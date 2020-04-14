CC          = gcc    # GMON_OUT_PREFIX=mygmon; gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes # -g -pg

LDFLAGS     = -lconfig
LDLIBS      = -lzmq   # -L../codecs -lpnt
LIBS 	    = ../api/libxdcomms.a

#static version of LDLIBS for compatible binaries
COMPATLIBS  = ../x86_64_prebuild/libzmq.a -static -Wl,--allow-multiple-definition -Wl,-Bstatic -lsodium -lunwind -llzma $(LDFLAGS) -ldl -lc -lstdc++ -lpthread

all: hal

hal: 	../log/log.o config.o device_open.o device_read_write.o map.o packetize.o packetize_sdh_be_v1.o packetize_sdh_be_v2.o packetize_sdh_bw_v1.o crc.o hal.o 
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) $(LDLIBS) $(LDFLAGS)

static: ../log/log.o config.o device_open.o device_read_write.o map.o packetize.o packetize_sdh_be_v1.o packetize_sdh_be_v2.o packetize_sdh_bw_v1.o crc.o hal.o 
	$(CC) $(CFLAGS) -o hal $^ $(LIBS) $(COMPATLIBS)

%.o:	%.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o hal hal-compat
