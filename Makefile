CC          = gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes

LDLIBS      = -lzmq -L./api -lxdcomms -L./appgen -lpnt 

all: sub_zc sub_log sub_api sub_appgen sub_daemon  

sub_api:
	make CC=$(CC) STATIC_BUILD=false -C ./api

sub_appgen:
	make CC=$(CC) STATIC_BUILD=false -C ./appgen

sub_daemon:
	make CC=$(CC) STATIC_BUILD=false -C ./daemon

sub_log: 
	make CC=$(CC) STATIC_BUILD=false -C ./log

sub_zc: 
	make CC=$(CC) STATIC_BUILD=false -C ./zc

static:
	#build with static flag
	make CC=$(CC) STATIC_BUILD=true -C ./zc
	make CC=$(CC) STATIC_BUILD=true -C ./log
	make CC=$(CC) STATIC_BUILD=true -C ./api
	make CC=$(CC) STATIC_BUILD=true -C ./appgen
	make CC=$(CC) STATIC_BUILD=true -C ./daemon

clean:
#	rm -f /tmp/halsub* /tmp/halpub*
	rm -f zc/zc zc/zc-compat zc/*.o 
	rm -f api/*.a api/*.o api/*.so
	rm -f appgen/*.a appgen/*.o appgen/float appgen/*.so
	rm -f daemon/hal daemon/*.o
	rm -f daemon/hal-compat
	rm -f log/*.o 

install: 
	mkdir -p $(INSTALLPATH)
	install -m 755 zc/zc $(INSTALLPATH)
	install -m 755 hal   $(INSTALLPATH)

