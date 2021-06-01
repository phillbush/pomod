enum Command {
	BYE   = 0,
	STOP  = 'x',
	START = 's',
	INFO  = 'i'
};

enum Cycle {
	STOPPED    = 'x',
	POMODORO   = 'p',
	SHORTBREAK = 's',
	LONGBREAK  = 'l'
};

enum Info {
	CYCLE   = 0,
	MIN     = 1,
	SEC     = 2,
	INFOSIZ = 3,
};

char *getsockpath(void);
char *getcyclename(char);
