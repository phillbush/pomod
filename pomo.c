#include <sys/socket.h>
#include <sys/un.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "util.h"

static char *sockpath = NULL;

static void
usage(void)
{
	(void)fprintf(stderr, "usage: %s [-S socket] [start|stop|info]\n", getprogname());
	exit(1);
}

static int
parseargs(int argc, char *argv[])
{
	int ch;

	while ((ch = getopt(argc, argv, "S:")) != -1) {
		switch (ch) {
		case 'S':
			sockpath = optarg;
			break;
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;
	if (argc != 1)
		usage();
	if (sockpath == NULL)
		sockpath = getsockpath();
	if (strcasecmp(*argv, "stop") == 0)
		return STOP;
	else if (strcasecmp(*argv, "start") == 0)
		return START;
	else if (strcasecmp(*argv, "info") == 0)
		return INFO;
	usage();
	return 0;                       /* NOTREACHED */
}

static int
connectsocket(const char *path)
{
	struct sockaddr_un saddr;
	int fd;

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		err(1, "socket");
	memset(&saddr, 0, sizeof saddr);
	saddr.sun_family = AF_UNIX;
	strncpy(saddr.sun_path, path, (sizeof saddr.sun_path) - 1);
	if (connect(fd, (struct sockaddr *)&saddr, sizeof saddr) == -1)
		err(1, "connect");
	return fd;
}

static void
sendcommand(char cmd, int fd)
{
	if (write(fd, &cmd, 1) == -1) {
		err(1, "write");
	}
}

static void
printinfo(int fd)
{
	int n;
	char buf[INFOSIZ];

	if ((n = read(fd, buf, INFOSIZ)) != INFOSIZ)
		errx(1, "could not get info");
	buf[INFOSIZ - 1] = '\0';
	printf("%s\n", buf);
}

int
main(int argc, char *argv[])
{
	int fd;                         /* socket file descriptor */
	char cmd;

	setprogname(argv[0]);
	cmd = parseargs(argc, argv);
	fd = connectsocket(sockpath);
	sendcommand(cmd, fd);
	if (cmd == INFO)
		printinfo(fd);
	close(fd);
	return 0;
}
