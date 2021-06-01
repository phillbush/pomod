# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

CFLAGS = -g -O0 -Wall -Wextra ${INCS} ${CPPFLAGS}
LDFLAGS = ${LIBS}
CC = cc
