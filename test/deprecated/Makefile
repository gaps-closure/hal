CC          = gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes

LDLIBS      = -L../api -lxdcomms -lzmq -L../appgen -lxyz -lpnt 

all: app_test

app_test: app_test.o
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

app_test.o: app_test.c 
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f app_test *.o

