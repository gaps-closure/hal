CC          = gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes

#LDLIBS      = -L./codecs -lpnt -lxyz

all: libxdcomms.a libxdcomms.so

libxdcomms.a: ../log/log.o xdcomms.o 
	ar rcs $@ $^ 

libxdcomms.so: ../log/log.o xdcomms.o
	$(CC) $(CFLAGS) -shared -o $@ $^ -lzmq

xdcomms.o:  xdcomms.c
	$(CC) $(CFLAGS) -fpic -c $<

libs: libxdcomms.a libxdcomms.so

clean:
	rm -f *.o *.a *.so


