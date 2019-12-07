CC          = gcc
CFLAGS      = -O2
LDFLAGS     = -lconfig
# INSTALLPATH = /usr/local/bin
INSTALLPATH = ./bin

all: zcbin hal

hal: hal.o 
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

zcbin: 
	make CC=$(CC) -C ./zc

clean:
	rm -f *.o zc/*.o
	rm -f hal zc/zc

install: 
	mkdir -p $(INSTALLPATH)
	install -m 755 zc/zc $(INSTALLPATH)
	install -m 755 hal   $(INSTALLPATH)

