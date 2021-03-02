CC          ?= gcc    # GMON_OUT_PREFIX=mygmon; gcc
CFLAGS      ?= -O2 -Wall -Wstrict-prototypes # -g -pg

LOCAL_LIBS  = -lconfig
LIBS 	    = $(OBJDIR)/../api/libxdcomms.a
INCL = -I ../log -I ../api

OBJDIR ?= .

LDLIBS      = -lzmq -lpthread  # -L../codecs -lpnt
LIBS        = $(OBJDIR)/../api/libxdcomms.a
HAL_OBJECT_LIST = $(OBJDIR)/../log/log.o $(OBJDIR)/config.o $(OBJDIR)/device_open.o $(OBJDIR)/device_read_write.o $(OBJDIR)/map.o $(OBJDIR)/time.o $(OBJDIR)/packetize.o $(OBJDIR)/packetize_sdh_be_v1.o $(OBJDIR)/packetize_sdh_be_v3.o $(OBJDIR)/packetize_sdh_be_v2.o $(OBJDIR)/packetize_sdh_bw_v1.o $(OBJDIR)/packetize_sdh_ha_v1.o $(OBJDIR)/crc.o $(OBJDIR)/hal.o

#static version of LDLIBS for compatible binaries
COMPATLIBS  = ../x86_64_prebuild/libzmq.a -static -Wl,--allow-multiple-definition -Wl,-Bstatic -lsodium -lunwind -llzma $(LOCAL_LIBS) -ldl -lc -lstdc++ -lpthread

all: $(OBJDIR)/hal

$(OBJDIR)/../log/log.o: ../log/log.c
	$(CC) $(CFLAGS) $(INCL) -fpic -c ../log/log.c -o $@

$(OBJDIR)/hal: $(HAL_OBJECT_LIST)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) $(LDLIBS) $(LDFLAGS) $(LOCAL_LIBS)

static: $(HAL_OBJECT_LIST) 
	$(CC) $(CFLAGS) $(INCL) -o $(OBJDIR)/hal $^ $(LIBS) $(COMPATLIBS)

$(OBJDIR)/%.o:	%.c
	$(CC) $(CFLAGS) $(INCL) -c $< -o $@

clean:
	rm -f $(OBJDIR)/*.o $(OBJDIR)/hal $(OBJDIR)/hal-compat
