CC          = gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes -L.
LIB_OBJ     =pnt.o gma.o float.o

all: libfloat.a libgma.a slibs

static: libs

libgma.a: gma.o float.o
	ar rcs $@ $^

libfloat.a: float.o
	ar rcs $@ $^

libgma.so: gma.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $< -lfloat

libfloat.so: float.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $<

%.o:	%.c
	$(CC) $(CFLAGS) -c $<

libs: libgma.a libfloat.a

slibs: libfloat.so libgma.so

clean:
	rm -f *.o *.a float *.so *~

