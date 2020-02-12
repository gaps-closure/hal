/*
 * APP_TEST.C
 *   Sample program to call libclosure.a functions
 *
 * February 2019
 * A. McAuley, Perspecta Labs (amcauley@perspectalabs.com)
 *
 * For description and revision history see README.txt
 */

//#include "network.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <zmq.h>
#include <unistd.h>


#define ADU_SIZE_MAX 100

int verbose=1;

void exit_with_zmq_error(const char* where) {
    fprintf(stderr,"%s error %d: %s\n",where,errno,zmq_strerror(errno));
    exit(-1);
}

void gaps_asyn_send(void *socket, uint8_t *buff, size_t len) {
//    echo "$TAG $DATA" | zc/zc $ZC_COMMON_FLAGS -dEOF -n 1 pub $HALSUB
    for (int i = 0; i < len; i++)
    {
        fprintf(stderr, "%02X", buff[i]);
    }
    fprintf(stderr, "\n");
    
//    char buff [10];
    printf ("ZMQ Sending...\n");
    zmq_send (socket, buff, len, 0);
//    zmq_recv (socket, buff, 10, 0);
//    printf ("Received %s\n", buffer);
}

void encode(uint8_t *buff, size_t *len) {
    uint8_t     data1[] = {0x0, 0x82, 0x0, 0xa5, 0x0, 0x64, 0x80, 0x0, 0x43, 0xc0, 0x0, 0x0, 0x2, 0x0, 0x0};
    
    *len = sizeof(data1);
    memcpy (buff, data1, *len);
    return;
}

void * z_connect(int type, const char* endpoint) {
    int err;
    
    void *ctx = zmq_ctx_new ();
    if(ctx == NULL) exit_with_zmq_error("zmq_ctx_new");
    
    void *socket = zmq_socket(ctx, type);
    if(socket == NULL) exit_with_zmq_error("zmq_socket");
    
    err = zmq_connect(socket, endpoint);
    if(err) exit_with_zmq_error("zmq_connect");
    
    if(verbose) fprintf(stderr,"connected (%d) to %s\n", type, endpoint);
    
    usleep(10000); // let a chance for the connection to be established before sending a message
    
    return (socket);
}


/*
 * MAIN
 */
int main(int argc, char **argv) {
    uint8_t  adu[ADU_SIZE_MAX];
    size_t   len;
    void     *socket;
    
    socket = z_connect(ZMQ_PUB, "ipc://halsub");
    encode(adu, &len);
    gaps_asyn_send(socket, adu, len);
                   
    return (0);
}
