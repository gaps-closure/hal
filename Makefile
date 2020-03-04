CC          = gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes

LDLIBS      = -L./api -lxdcomms -lzmq -L./appgen -lpnt 

all: sub_zc sub_api sub_appgen sub_daemon app_test

app_test: app_test.o
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

app_test.o: app_test.c 
	$(CC) $(CFLAGS) -c $<

sub_api:
	make CC=$(CC) -C ./api

sub_appgen:
	make CC=$(CC) -C ./appgen

sub_daemon:
	make CC=$(CC) -C ./daemon

sub_zc: 
	make CC=$(CC) -C ./zc

clean:
	rm -f app_test *.o fifo* *_log.txt halsub* halpub*
	rm -f zc/zc zc/*.o api/*.a api/*.o codecs/*.a codecs/*.o daemon/hal daemon/*.o 

install: 
	mkdir -p $(INSTALLPATH)
	install -m 755 zc/zc $(INSTALLPATH)
	install -m 755 hal   $(INSTALLPATH)

