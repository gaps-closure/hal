CC          = gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes

#LDLIBS      = -L./codecs -lpnt -lxyz

all: libxdcomms.a libxdcomms.so

libxdcomms.a: xdcomms.o 
	ar rcs $@ $< 

xdcomms.o:  xdcomms.c
	$(CC) $(CFLAGS) -c $<

libxdcomms.so: xdcomms.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $< -lzmq

libs: libxdcomms.a libxdcomms.so

clean:
	rm -f *.o *.a *.so


