.PHONY: clean help

IDIR	  := include
CC	    := gcc
CFLAGS	:= -g -I$(IDIR) -O3 -Wall -Werror -std=gnu99

all: 					memcpy_test

memcpy_test:	memcpy_test.c apex_memmove.o
							$(CC) -o $@ $^ $(CFLAGS)
							
%.o: %.c
							$(CC) -c -o $@ $< $(CFLAGS)

clean:
		rm -f *.o memcpy_test *.csv fig_*

help:
		@echo "usage: make <target>"
			@echo ""
				@echo "targets: $(APPS)"
					@echo ""

