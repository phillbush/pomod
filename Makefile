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

install: all
	install -D -m 755 pomod ${DESTDIR}${PREFIX}/bin/${PROG}
	install -D -m 755 pomo ${DESTDIR}${PREFIX}/bin/${PROG}

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/pomod
	rm -f ${DESTDIR}${PREFIX}/bin/pomo

clean:
	-rm ${OBJS} pomod pomo

.PHONY: all clean install uninstall
