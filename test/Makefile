CC          ?= gcc
CFLAGS      ?= -O2 -Wall -Wstrict-prototypes

all: 	app_req_rep

app_req_rep:	app_req_rep.c app_req_rep.o
# HAL veriosn:
#	$(CC) $(CFLAGS) -o $@ app_req_rep.o ../api/xdcomms.o ../appgen/6month-demo/gma.o ../log/log.o ../appgen/6month-demo/float.o  -l zmq
# DMA version:
	$(CC) $(CFLAGS) -o $@ app_req_rep.o ~/gaps/xdcomms-dma/api/xdcomms_dma.o ~/gaps/xdcomms-dma/log/log.o ~/gaps/xdcomms-dma/crc/crc.o ../appgen/6month-demo/gma.o ../appgen/6month-demo/float.o -pthread

%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@ -I ../log 

clean:
	rm -f *.o app_req_rep
