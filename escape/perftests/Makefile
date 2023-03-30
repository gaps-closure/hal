.PHONY: clean help

IDIR	  := include
CC	    := gcc
CFLAGS	:= -g -I$(IDIR) -O3 -Wall -Werror -std=gnu99

all:	memcpy_test thread_eg

memcpy_test:	memcpy_test.c apex_memmove.o thread_pool.o
		$(CC) -o $@ $^ $(CFLAGS) -lpthread

thread_eg:	thread_eg.c thread_pool.o
		$(CC) $(CFLAGS) -o $@ $^ -lpthread
							
%.o: %.c
		$(CC) -c -o $@ $< $(CFLAGS)

clean:
		rm -f *.o memcpy_test *.csv fig_* thread_eg

help:
		@echo "usage: make"
