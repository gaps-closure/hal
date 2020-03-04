CC          = gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes

LDLIBS      = -lzmq -L./api -lxdcomms -L./appgen -lpnt 

all: sub_zc sub_api sub_appgen sub_daemon sub_test

sub_api:
	make CC=$(CC) -C ./api

sub_appgen:
	make CC=$(CC) -C ./appgen

sub_daemon:
	make CC=$(CC) -C ./daemon

sub_test: 
	make CC=$(CC) -C ./test

sub_zc: 
	make CC=$(CC) -C ./zc

clean:
	rm -f fifo* *_log.txt halsub* halpub*
	rm -f zc/zc zc/*.o 
	rm -f api/*.a api/*.o 
	rm -f appgen/*.a appgen/*.o 
	rm -f daemon/hal daemon/*.o 
	rm -f test/app_test test/*.o 

install: 
	mkdir -p $(INSTALLPATH)
	install -m 755 zc/zc $(INSTALLPATH)
	install -m 755 hal   $(INSTALLPATH)

