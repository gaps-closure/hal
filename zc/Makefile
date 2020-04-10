LDLIBS=-lzmq
COMPATLIBS  = -static ../x86_64_prebuild/libzmq.a -Wl,--allow-multiple-definition -Wl,-Bstatic -lsodium -lunwind -llzma  -lc -lstdc++ -lpthread -ldl


zc: zc.o
ifeq ($(STATIC_BUILD),true)
	$(CC) -o $@ zc.o $(COMPATLIBS)
else

endif

