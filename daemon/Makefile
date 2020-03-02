CC          = gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes

LDFLAGS     = -lconfig
LDLIBS      = -L../api -lxdcomms -lzmq   # -L../codecs -lpnt

all: hal

hal: config.o devices.o packetize.o hal.o 
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS) $(LDFLAGS)

hal.o: hal.c 
	$(CC) $(CFLAGS) -c $<

config.o:  config.c
	$(CC) $(CFLAGS) -c $<

devices.o:  devices.c
	$(CC) $(CFLAGS) -c $<

packetize.o:  packetize.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o hal
