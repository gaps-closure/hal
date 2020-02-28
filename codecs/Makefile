CC          = gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes

all: libpnt.a

libpnt.a: pnt.o
	ar rcs $@ $<

pnt.o:  pnt.c
	$(CC) $(CFLAGS) -c $< 

libs: libpnt.a

clean:
	rm -f *.o *.a


