CC          = gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes -lzmq

LDFLAGS     = -lconfig
LDLIBS      = -lzmq

# INSTALLPATH = /usr/local/bin
INSTALLPATH = ./bin

all: app_test zcbin hal

app_test: app_test.o 
	$(CC) -o $@ $< $(CFLAGS) 

app_test.o: app_test.c 
	$(CC) $(CFLAGS) -c $<

hal: hal.o 
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

zcbin: 
	make CC=$(CC) -C ./zc

clean:
	rm -f *.o zc/*.o
	rm -f app_test hal zc/zc

install: 
	mkdir -p $(INSTALLPATH)
	install -m 755 zc/zc $(INSTALLPATH)
	install -m 755 hal   $(INSTALLPATH)

