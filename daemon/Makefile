CC          = gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes

LDFLAGS     = -lconfig
LDLIBS      = -lzmq   # -L../codecs -lpnt
LIBS 	    = ../api/libxdcomms.a
all: hal

hal: config.o device_open.o device_read_write.o map.o packetize.o packetize_sdh_be_v1.o packetize_sdh_be_v2.o packetize_sdh_bw_v1.o crc.o hal.o 
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) $(LDLIBS) $(LDFLAGS)

%.o:	%.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o hal
