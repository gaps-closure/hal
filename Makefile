CC          = gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes

LDFLAGS     = -lconfig
LDLIBS      = -L. -lclosure -lzmq 

all: zcbin libclosure.a hal app_test

app_test: app_test.o libclosure.a
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

hal: hal.o libclosure.a
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS) $(LDFLAGS)

app_test.o: app_test.c 
	$(CC) $(CFLAGS) -c $<

closure.o:  closure.c
	$(CC) $(CFLAGS) -c $< 

hal.o: hal.c 
	$(CC) $(CFLAGS) -c $<

zcbin: 
	make CC=$(CC) -C ./zc

libclosure.a: closure.o   # link library files into a static library
	ar rcs libclosure.a closure.o

libs: libclosure.a

clean:
	rm -f *.o zc/*.o
	rm -f app_test hal zc/zc

install: 
	mkdir -p $(INSTALLPATH)
	install -m 755 zc/zc $(INSTALLPATH)
	install -m 755 hal   $(INSTALLPATH)

