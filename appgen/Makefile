CC          = gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes -L.
LIB_OBJ     =pnt.o gma.o float.o

all: libpnt.a libxyz.a libgma.a slibs

libpnt.a: $(LIB_OBJ)
	ar rcs $@ $(LIB_OBJ)

libxyz.a: xyz.o float.o
	ar rcs $@ $^

libgma.a: gma.o float.o
	ar rcs $@ $^

libpnt.so: pnt.c float.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $<

libxyz.so: xyz.c float.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $<

libgma.so: gma.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $< -lfloat

libfloat.so: float.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $<

#float:  float.c
#	$(CC) $(CFLAGS) -o $@ $^

%.o:	%.c
	$(CC) $(CFLAGS) -c $<

libs: libpnt.a libxyz.a libgma.a

slibs: libfloat.so libgma.so

clean:
	rm -f *.o *.a float *.so


