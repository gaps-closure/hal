CC          = gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes

#LDLIBS      = -L./codecs -lpnt -lxyz

all: libxdcomms.a

libxdcomms.a: xdcomms.o 
	ar rcs $@ $< 

xdcomms.o:  xdcomms.c
	$(CC) $(CFLAGS) -c $<

libs: libxdcomms.a

clean:
	rm -f *.o *.a


