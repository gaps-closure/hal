CC          = gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes

LDFLAGS     = -lconfig
LDLIBS      = -L../api -lxdcomms -lzmq   # -L../codecs -lpnt

all: hal

hal: config.o devices.o packetize.o packetize_ge.o packetize_ms.o hal.o 
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS) $(LDFLAGS)

%.o:	%.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o hal
