CC          = gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes -L.
LIB_OBJ     =pnt.o gma.o float.o

all: libpnt.a libgma.a slibs

libpnt.a: $(LIB_OBJ)
	ar rcs $@ $(LIB_OBJ)

libgma.a: gma.o float.o
	ar rcs $@ $^

libpnt.so: pnt.c float.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $<

libgma.so: gma.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $< -lfloat

libfloat.so: float.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $<

%.o:	%.c
	$(CC) $(CFLAGS) -c $<

libs: libpnt.a libgma.a

slibs: libfloat.so libgma.so

clean:
	rm -f *.o *.a float *.so

