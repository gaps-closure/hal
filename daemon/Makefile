CC          = gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes

LDFLAGS     = -lconfig
LDLIBS      = -L../api -lxdcomms -lzmq   # -L../codecs -lpnt

all: hal

hal: config.o devices.o packetize.o packetize_sdh-be-v1.o packetize_sdh-bw-v1.o crc.o hal.o 
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS) $(LDFLAGS)

%.o:	%.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o hal
