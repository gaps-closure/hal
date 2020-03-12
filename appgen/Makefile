CC          = gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes

all: libpnt.a float

libpnt.a: pnt.o
	ar rcs $@ $<

float:  float.c
	$(CC) $(CFLAGS) -o $@ $^

%.o:	%.c
	$(CC) $(CFLAGS) -c $<

libs: libpnt.a

clean:
	rm -f *.o *.a


