#include <stdio.h>
#include <unistd.h>

#define PATHSIZ     104
#define PATHPREFIX  "/tmp/pomodoro."

char *
getsockpath(void)
{
	static char buf[PATHSIZ];

	snprintf(buf, PATHSIZ, PATHPREFIX "%ld", (long)geteuid());
	return buf;
}
