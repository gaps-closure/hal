all: zcbin hal

hal: hal.o 
	cc -o hal hal.o -lconfig

zcbin: 
	make -C ./zc

clean:
	rm -f *.o zc/*.o
	rm -f hal zc/zc

#install: 
#	install -m 755 zc/zc /usr/local/bin
#	install -m 755 hal /usr/local/bin

