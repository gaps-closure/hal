LDLIBS=-lzmq
COMPATLIBS  = -static ../x86_64_prebuild/libzmq.a -Wl,--allow-multiple-definition -Wl,-Bstatic -lsodium -lunwind -llzma  -lc -lstdc++ -lpthread -ldl

all: zc

zc: zc.o

static: zc.o
	$(CC) -o zc zc.o $(COMPATLIBS)

clean:
	rm -f zc *.o
