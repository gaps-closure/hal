CC          = gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes #-g -pg

all:  log.o

static: all

%.o:	%.c
	$(CC) $(CFLAGS) -fpic -c $<

clean:
	rm -f *.o
