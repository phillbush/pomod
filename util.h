#define INFOSIZ 128

enum Command {
	BYE   = 0,
	STOP  = 'p',
	START = 's',
	INFO  = 'i',
};

char *getsockpath(void);
