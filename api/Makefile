CC          = gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes

all: libxdcomms.a libxdcomms.so

static: libxdcomms.a

libxdcomms.a: ../log/log.o xdcomms.o 
	ar rcs $@ $^ 

libxdcomms.so: ../log/log.o xdcomms.o
	$(CC) $(CFLAGS) -shared -o $@ $^ -lzmq

xdcomms.o:  xdcomms.c
	$(CC) $(CFLAGS) -fpic -c $<

libs: libxdcomms.a libxdcomms.so

clean:
	rm -f *.o *.a *.so


