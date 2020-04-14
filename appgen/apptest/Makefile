CC          = gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes -L.
LIB_OBJ     = pnt.o 

all: libpnt.a libpnt.so

static: libpnt.a

libpnt.a: pnt.o
	ar rcs $@ $^

libpnt.so: pnt.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $< 

%.o:	%.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o *.a *.so *~

