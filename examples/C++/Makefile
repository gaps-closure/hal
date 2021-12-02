CC = clang++ 
CFLAGS = -g -Wall
LIBS = -lzmq -lzmqpp
all: sender receiver 

sender: sender.o message.o
	$(CC) $(CFLAGS) $(LIBS) message.o sender.o -o sender 

receiver: receiver.o message.o
	$(CC) $(CFLAGS) $(LIBS) message.o receiver.o -o receiver 

message.o: message.cpp message.h
	$(CC) $(CFLAGS) -c message.cpp 

clean:
	rm -rf sender receiver *.o

