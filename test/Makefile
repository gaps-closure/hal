CC          ?= gcc
CFLAGS      ?= -O2 -Wall -Wstrict-prototypes

all: 	app_req_rep

app_req_rep:	app_req_rep.c app_req_rep.o
	$(CC) $(CFLAGS) -o $@ app_req_rep.o ../api/xdcomms.o ../appgen/6month-demo/gma.o ../log/log.o ../appgen/6month-demo/float.o  -l zmq

%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@ -I ../log 

clean:
	rm -f *.o app_req_rep
