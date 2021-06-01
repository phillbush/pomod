include config.mk

SRCS = pomod.c pomo.c util.c
OBJS = ${SRCS:.c=.o}

all: pomod pomo

pomod: pomod.o util.o
	${CC} -o $@ pomod.o util.o ${LDFLAGS}

pomo: pomo.o util.o
	${CC} -o $@ pomo.o util.o ${LDFLAGS}

pomo.o: util.h
pomod.o: util.h

.c.o:
	${CC} ${CFLAGS} -c $<

clean:
	-rm ${OBJS} pomod pomo

.PHONY: all clean install uninstall
