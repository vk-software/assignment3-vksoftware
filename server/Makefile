CC := gcc

ifdef CROSS_COMPILE
	CC := aarch64-none-linux-gnu-${CC}
endif

all : server
build : server

server : server.c

	$(CC) server.c $(CFLAGS) $(CPPFLAGS) -g3 -Wall -Werror -o aesdsocket

clean :
	rm -f *.o server

