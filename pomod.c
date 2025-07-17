#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>

#include <err.h>
#include <limits.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "util.h"

#define BACKLOG     5
#define SECONDS     60          /* seconds in a minute */
#define MINUTES     60          /* minutes in a hour */
#define MILISECONDS 1000        /* miliseconds in a second */
#define NANOPERMILI 1000000
#define MAXCLIENTS  10

enum Duration {
	POMODORO_SECS   = SECONDS * 25,
	SHORTBREAK_SECS = SECONDS * 5,
	LONGBREAK_SECS  = SECONDS * 30
};

static char *sockpath;
static struct timespec pomodoro = {.tv_sec = POMODORO_SECS};
static struct timespec shortbreak = {.tv_sec = SHORTBREAK_SECS};
static struct timespec longbreak = {.tv_sec = LONGBREAK_SECS};

static void
usage(void)
{
	(void)fprintf(stderr, "usage: %s [-S socket] [-l time] [-p time] [-s time]\n", getprogname());
	exit(1);
}

static int
gettime(char *s)
{
	char *ep;
	long l;

	l = strtol(s, &ep, 10);
	if (*s == '\0' || *ep != '\0' || l <= 0 || l >= INT_MAX / SECONDS)
		goto error;
	return SECONDS * (int)l;
error:
	errx(1, "%s: invalid time", s);
}

/* parse arguments and set global variables */
static void
parseargs(int argc, char *argv[])
{
	int ch;

	while ((ch = getopt(argc, argv, "l:p:S:s:")) != -1) {
		switch (ch) {
		case 'S':
			sockpath = optarg;
			break;
		case 'l':
			longbreak.tv_sec = gettime(optarg);
			break;
		case 'p':
			pomodoro.tv_sec = gettime(optarg);
			break;
		case 's':
			shortbreak.tv_sec = gettime(optarg);
			break;
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;
	if (argc > 0) {
		usage();
	}
	if (sockpath == NULL) {
		sockpath = getsockpath();
	}
}

static int
createsocket(const char *path, int backlog)
{
	struct sockaddr_un saddr;
	int sd;

	memset(&saddr, 0, sizeof saddr);
	saddr.sun_family = AF_UNIX;
	strncpy(saddr.sun_path, path, (sizeof saddr.sun_path) - 1);
	unlink(path);
	if ((sd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		goto error;
	if (bind(sd, (struct sockaddr *)&saddr, sizeof saddr) == -1)
		goto error;
	if (listen(sd, backlog) == -1)
		goto error;
	return sd;
error:
	err(1, "%s", path);
}

/* return 1 if we handle client */
static int
acceptclient(struct pollfd *pfds, size_t n)
{
	struct sockaddr_un caddr;
	socklen_t len;
	size_t i;
	int cd;                         /* client file descriptor */

	len = sizeof caddr;
	if ((cd = accept(pfds[0].fd, (struct sockaddr *)&caddr, &len)) == -1)
		err(1, "accept");
	for (i = 1; i <= n; i++) {
		if (pfds[i].fd <= 0) {
			pfds[i].fd = cd;
			pfds[i].events = POLLIN;
			return 1;
		}
	}
	close(cd);      /* ignore if we have MAXCLIENTS or more */
	return 0;
}

/* return 1 if we do not handle client anymore */
static int 
handleclient(int fd)
{
	char cmd;
	int n;

	if ((n = read(fd, &cmd, 1)) != 1)
		return BYE;
	return cmd;
}

static void
gettimespec(struct timespec *ts)
{
	if (clock_gettime(CLOCK_REALTIME, ts) == -1) {
		err(1, "time");
	}
}

static void
notify(char cycle)
{
	printf("%s\n", getcyclename(cycle));
	fflush(stdout);
}

static void
timesub(struct timespec *a, struct timespec *b, struct timespec *c)
{
	timespecsub(a, b, c);
	if (c->tv_sec < 0) {
		c->tv_sec = 0;
	}
	if (c->tv_nsec < 0) {
		c->tv_nsec = 0;
	}
}

static void
info(int fd, struct timespec *stoptime, char cycle)
{
	struct timespec now, diff;
	uint8_t buf[INFOSIZ];

	gettimespec(&now);
	timesub(stoptime, &now, &diff);
	buf[CYCLE] = cycle;
	buf[MIN] = buf[SEC] = 0;
	switch (cycle) {
	case POMODORO:
		buf[MIN] = (uint8_t)((pomodoro.tv_sec - diff.tv_sec) / SECONDS);
		buf[SEC] = (uint8_t)((pomodoro.tv_sec - diff.tv_sec) % SECONDS);
		break;
	case SHORTBREAK:
		buf[MIN] = (uint8_t)((shortbreak.tv_sec - diff.tv_sec) / SECONDS);
		buf[SEC] = (uint8_t)((shortbreak.tv_sec - diff.tv_sec) % SECONDS);
		break;
	case LONGBREAK:
		buf[MIN] = (uint8_t)((longbreak.tv_sec - diff.tv_sec) / SECONDS);
		buf[SEC] = (uint8_t)((longbreak.tv_sec - diff.tv_sec) % SECONDS);
		break;
	}
	write(fd, buf, INFOSIZ);
}

static int
gettimeout(struct timespec *stoptime)
{
	struct timespec now, diff;

	gettimespec(&now);
	timesub(stoptime, &now, &diff);
	return MILISECONDS * diff.tv_sec + diff.tv_nsec / NANOPERMILI;
}

static void
run(int sd)
{
	struct pollfd pfds[MAXCLIENTS + 1];
	struct timespec now, stoptime;
	size_t i;
	int timeout;
	int pomocount;
	int n;
	char cycle;

	timeout = -1;
	pfds[0].fd = sd;
	pfds[0].events = POLLIN;
	for (i = 1; i <= MAXCLIENTS; i++)
		pfds[i].fd = -1;
	cycle = STOPPED;
	pomocount = 0;
	for (;;) {
		if ((n = poll(pfds, MAXCLIENTS + 1, timeout)) == -1)
			err(1, "poll");
		if (n > 0) {
			if (pfds[0].revents & POLLHUP)          /* socket has been disconnected */
				return;
			if (pfds[0].revents & POLLERR) {        /* an error occurred */
				err(1, "poll");
				return;
			}
			if (pfds[0].revents & POLLIN) {          /* handle new client */
				if (acceptclient(pfds, MAXCLIENTS) < 1)
					warn("too many clients");
			}

			for (i = 1; i <= MAXCLIENTS; i++) {     /* handle existing client */
				if (pfds[i].fd <= 0 || !(pfds[i].events & (POLLIN|POLLHUP)))
					continue;
				if (pfds[i].events & POLLHUP) {
					close(pfds[i].fd);
					pfds[i].fd = -1;
					continue;
				}

				switch (handleclient(pfds[i].fd)) {
				case BYE:
					close(pfds[i].fd);
					pfds[i].fd = -1;
					break;
				case START:
					pomocount = 0;
					gettimespec(&stoptime);
					stoptime.tv_sec += pomodoro.tv_sec;
					notify(cycle = POMODORO);
					break;
				case STOP:
					cycle = STOPPED;
					break;
				case INFO:
					info(pfds[i].fd, &stoptime, cycle);
					break;
				}
			}
		}
		gettimespec(&now);
		switch (cycle) {
		case STOPPED:
			timeout = -1;
			break;
		case POMODORO:
			if (timespeccmp(&now, &stoptime, >=)) {
				pomocount++;
				gettimespec(&stoptime);
				if (pomocount < 4) {
					notify(cycle = SHORTBREAK);
					stoptime.tv_sec += shortbreak.tv_sec;
				} else {
					pomocount = 0;
					notify(cycle = LONGBREAK);
					stoptime.tv_sec += longbreak.tv_sec;
				}
			}
			timeout = gettimeout(&stoptime);
			break;
		case SHORTBREAK:
			if (timespeccmp(&now, &stoptime, >)) {
				notify(cycle = POMODORO);
				stoptime.tv_sec += pomodoro.tv_sec;
			}
			timeout = gettimeout(&stoptime);
			break;
		case LONGBREAK:
			pomocount = 0;
			if (timespeccmp(&now, &stoptime, >)) {
				notify(cycle = POMODORO);
				stoptime.tv_sec += pomodoro.tv_sec;
			}
			timeout = gettimeout(&stoptime);
			break;
		}
	}
}

int
main(int argc, char *argv[])
{
	int sd;                         /* socket file descriptor */

	setprogname(argv[0]);
	parseargs(argc, argv);
	sd = createsocket(sockpath, BACKLOG);
	run(sd);
	close(sd);
	unlink(sockpath);
	return 0;
}
