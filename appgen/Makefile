CC          = gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes

all: libpnt.a libxyz.a  #float 

LIB_OBJ=pnt.o gma.o float.o

libpnt.a: $(LIB_OBJ)
	ar rcs $@ $(LIB_OBJ)

libxyz.a: xyz.o float.o
	ar rcs $@ $^

#float:  float.c
#	$(CC) $(CFLAGS) -o $@ $^

%.o:	%.c
	$(CC) $(CFLAGS) -c $<

libs: libpnt.a libxyz.a

clean:
	rm -f *.o *.a float


