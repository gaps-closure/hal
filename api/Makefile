CC          = gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes

LDLIBS      = -L. -lclosure -lzmq 

all: libclosure.a

closure.o:  closure.c
	$(CC) $(CFLAGS) -c $< 

libclosure.a: closure.o   # link library files into a static library
	ar rcs libclosure.a closure.o

libs: libclosure.a

clean:
	rm -f *.o *.a


