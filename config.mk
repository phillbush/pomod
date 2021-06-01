# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

LOCALINC = /usr/local/include
LOCALLIB = /usr/local/lib

# includes and libs
INCS = -I${LOCALINC}
LIBS = -L${LOCALLIB} -lpthread

# flags
CFLAGS = -g -O0 -Wall -Wextra ${INCS} ${CPPFLAGS}
LDFLAGS = ${LIBS}

# compiler and linker
CC = cc
