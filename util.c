#include <stdio.h>
#include <unistd.h>
#include "util.h"

#define PATHSIZ     104
#define PATHPREFIX  "/tmp/pomodoro."

char *
getsockpath(void)
{
	static char buf[PATHSIZ];

	snprintf(buf, PATHSIZ, PATHPREFIX "%ld", (long)geteuid());
	return buf;
}

char *
getcyclename(char cycle)
{
	switch (cycle) {
	case STOPPED:
		return "stopped";
	case POMODORO:
		return "pomodoro";
	case SHORTBREAK:
		return "shortbreak";
	case LONGBREAK:
		return "longbreak";
	}
	return "";
}
