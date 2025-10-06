CC := gcc

ifdef CROSS_COMPILE
	CC := aarch64-none-linux-gnu-${CC}
endif

build : writer

writer : writer.c

	$(CC) writer.c $(CFLAGS) $(CPPFLAGS) -o writer

clean :
	rm -f *.o writer
    