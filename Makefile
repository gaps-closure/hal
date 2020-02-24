CC          = gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes

LDFLAGS     = -lconfig
LDLIBS      = -L./api -lclosure -lzmq 

all: zcbin libapi hal app_test

app_test: app_test.o
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

hal: hal.o
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS) $(LDFLAGS)

app_test.o: app_test.c 
	$(CC) $(CFLAGS) -c $<

hal.o: hal.c 
	$(CC) $(CFLAGS) -c $<

zcbin: 
	make CC=$(CC) -C ./zc

libapi:
	make CC=$(CC) -C ./api


clean:
	rm -f *.o zc/*.o
	rm -f app_test hal zc/zc  api/*.o api/*.a fifo* *_log.txt halsub* halpub*

install: 
	mkdir -p $(INSTALLPATH)
	install -m 755 zc/zc $(INSTALLPATH)
	install -m 755 hal   $(INSTALLPATH)

