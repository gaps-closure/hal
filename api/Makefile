CC          = gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes

LDLIBS      = -L./codecs -lpnt

all: libxdcomms.a

libxdcomms.a: xdcomms.o 
	ar rcs $@ $< 

xdcomms.o:  xdcomms.c
	$(CC) $(CFLAGS) -c $< $(LDLIBS)

libs: libxdcomms.a

clean:
	rm -f *.o *.a


