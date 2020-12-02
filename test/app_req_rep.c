/*
 * APP_EXAMPLE.C
 *   Simple Request-Reply application using latest HAL API.
 *
 * December 2020, Perspecta Labs
 *
 * gcc -c app_req_rep.c -I ../log
 * gcc -o z app_req_rep.o ../api/xdcomms.o ../appgen/6month-demo/gma.o ../log/log.o ../appgen/6month-demo/float.o  -l zmq
 */

#include "../api/xdcomms.h"
#include "../appgen/6month-demo/gma.h"
#include <stdio.h>
#include <sys/time.h>

# define IPC_ADDR_MAX 50

/**********************************************************************/
/* Get options */
/*********t************************************************************/
/* Default option values */
int  mux_g2o = 1, sec_g2o = 1, typ_g2o = 1;    /* one-way request tag */
int  mux_o2g = 2, sec_o2g = 2, typ_o2g = 1;    /* one-way reply tag */
int  log_level            = 2;
int  receive_first        = 0;
int  loop_count           = 1;
int  sub_block_timeout_ms = -1;
char xdc_addr_in_green[IPC_ADDR_MAX]   = "ipc:///tmp/halsubbegreen";
char xdc_addr_out_green[IPC_ADDR_MAX]  = "ipc:///tmp/halpubbegreen";
char xdc_addr_in_orange[IPC_ADDR_MAX]  = "ipc:///tmp/halsubbeorange";
char xdc_addr_out_orange[IPC_ADDR_MAX] = "ipc:///tmp/halpubbeorange";

/* Print options */
void opts_print(void) {
  printf("Test program for sending requests and replies using the GAPS Hardware Abstraction Layer (HAL)\n");
  printf("Usage: ./app_req_rep [Options]\n");
  printf("[Options]:\n");
  printf(" -b : Subscriber Blocking timeout (in milliseconds): default = -1 (blocking) \n");
  printf(" -h : Print this message\n");
  printf(" -l : log level: 0=TRACE, 1=DEBUG, 2=INFO, 3=WARN, 4=ERROR, 5=FATAL (default = 2)\n");
  printf(" -n : Number of request-response loops: Default = 1\n");
  printf(" -r : Receive first (Responder): default = Send first (Requester)\n");

  printf(" -m : Green to Orange Multiplexing (mux) tag (int): Default = 1\n");
  printf(" -s : Green to Orange Security (sec)     tag (int): Default = 1\n");
  printf(" -t : Green to Orange Data type (typ)    tag (int): Default = 1\n");
  printf(" -M : Orange to Green Multiplexing (mux) tag (int): Default = 2\n");
  printf(" -S : Orange to Green Security (sec)     tag (int): Default = 2\n");
  printf(" -T : Orange to Green Data type (typ)    tag (int): Default = 1\n");
}

/* Parse the configuration file */
void opts_get(int argc, char **argv) {
  int opt;
  while((opt =  getopt(argc, argv, "b:c:hl:o:rm:s:t:M:S:T:")) != EOF)
  {
    switch (opt)
    {
      case 'b':
        sub_block_timeout_ms = atoi(optarg);
        break;
      case 'c':
        loop_count = atoi(optarg);
        break;
      case 'h':
        opts_print();
        exit(0);
      case 'l':
        log_level = atoi(optarg);;
        break;
      case 'r':
        receive_first = 1;
        break;
      case 'm':
        mux_g2o = atoi(optarg);
        break;
      case 's':
        sec_g2o = atoi(optarg);
        break;
      case 't':
        typ_g2o = atoi(optarg);
        break;
      case 'M':
        mux_o2g = atoi(optarg);
        break;
      case 'S':
        sec_o2g = atoi(optarg);
        break;
      case 'T':
        typ_o2g = atoi(optarg);
        break;
      case ':':
        fprintf(stderr, "\noption needs a value\n");
        opts_print();
        exit(0);
        break;
      default:
        fprintf(stderr, "\nSkipping undefined option (%c)\n", opt);
    }
  }
  fprintf(stderr, "send-tag = [%d, %d, %d] recv-tag = [%d, %d, %d] ", mux_g2o, sec_g2o, typ_g2o, mux_o2g, sec_o2g, typ_o2g);
  fprintf(stderr, "block_timeout=%d loop_count=%d, receive_first=%d xdc_log_level=%d\n", sub_block_timeout_ms, loop_count, receive_first, log_level);
}

/**********************************************************************/
/* Specify Application Data Unit (ADU) Information */
/*********t************************************************************/
void position_set (uint8_t *adu, size_t *len) {
  static int            n = 0;
  static double         z = 102;     /* changing value intitialized */
  position_datatype  *xyz = (position_datatype *) adu;
  trailer_datatype   *trl = &(xyz->trailer);
  
  xyz->x    = -74.574489;
  xyz->y    =  40.695545;
  xyz->z    =  z;
  trl->seq  =  n;
  trl->rqr  =  0;
  trl->oid  =  0;
  trl->mid  =  0;
  trl->crc  =  0;
  *len=(size_t) sizeof(*xyz);
  z += 0.1;                   /* changing value for next call */
  n += 1;
}

/**********************************************************************/
/* Send and receive (or receive and send) one ADU */
/*********t************************************************************/
/* Create, send and print one message */
void send_one(uint8_t *adu, size_t *adu_len, gaps_tag *tag_pub, void *socket) {
  xdc_asyn_send(socket, adu, tag_pub);
  fprintf(stderr, "app tx: ");   position_print((position_datatype *) adu);
}

/* Create, send and print one message */
void recv_one(uint8_t *adu, size_t *adu_len, gaps_tag *tag_pub, gaps_tag *tag_sub, void *socket_pub, void *socket_sub) {
  int rv=-1;
//  xdc_blocking_recv(socket_sub, adu, r_tag);
  rv = xdc_recv(socket_sub, adu, tag_sub);
  if (rv > 0) {
    fprintf(stderr, "app rx: "); position_print((position_datatype *) adu);
  }
  else {
    fprintf(stderr, "app rx failed: rv=%d (timeout=%d ms) ", rv, sub_block_timeout_ms);
    send_one(adu, adu_len, tag_pub, socket_pub);
    recv_one(adu, adu_len, tag_pub, tag_sub, socket_pub, socket_sub);
  }
}

/* One send & receive loop (or receive & send loop)  */
void send_and_recv(gaps_tag *tag_pub, gaps_tag *tag_sub, void *socket_pub, void *socket_sub) {
  uint8_t            adu[ADU_SIZE_MAX_C];
  size_t             adu_len;
  position_datatype  *xyz = (position_datatype *) adu;
  
  if (receive_first == 0) {
    position_set(adu, &adu_len);
    send_one(adu, &adu_len, tag_pub, socket_pub);
  }
  recv_one(adu, &adu_len, tag_pub, tag_sub, socket_pub, socket_sub);
  if (receive_first == 1) {
    xyz->x += 5.0;
    send_one(adu, &adu_len, tag_pub, socket_pub);
  }
}
         
/**********************************************************************/
/* Conifgure API, Run experiment and collect results */
/*********t************************************************************/
int main(int argc, char **argv) {
  int             i;
  gaps_tag        tag_pub, tag_sub;
  struct timeval  t0, t1;
  long            elapsed_us;
  void            *socket_pub, *socket_sub;
  
  opts_get (argc, argv);
  xdc_log_level(log_level);

  /* A) Configure TAGS and ADDRESSES (values depend on the enclave) */
  if (receive_first == 0) {
    tag_write(&tag_pub, mux_g2o, sec_g2o, typ_g2o);
    tag_write(&tag_sub, mux_o2g, sec_o2g, typ_o2g);
    xdc_set_in(xdc_addr_in_green);
    xdc_set_out(xdc_addr_out_green);
  }
  else {
    tag_write(&tag_sub, mux_g2o, sec_g2o, typ_g2o);
    tag_write(&tag_pub, mux_o2g, sec_o2g, typ_o2g);
    xdc_set_in(xdc_addr_in_orange);
    xdc_set_out(xdc_addr_out_orange);
  }
  
  /* B) Configure ZMQ Pub and Sub Interfaces (same for both enclaves) */
  xdc_register(position_data_encode, position_data_decode, DATA_TYP_POSITION);
  xdc_ctx();
  socket_pub = xdc_pub_socket();
//  socket_sub = xdc_sub_socket(tag_sub);
  socket_sub = xdc_sub_socket_non_blocking(tag_sub, sub_block_timeout_ms);
  
  // C) Run Experiment (Calculating delay and read-write loop rate)
  gettimeofday(&t0, NULL);
  for (i=0; i<loop_count; i++) send_and_recv(&tag_pub, &tag_sub, socket_pub, socket_sub);  // usleep(1111);
  gettimeofday(&t1, NULL);
  elapsed_us = ((t1.tv_sec-t0.tv_sec)*1000000) + t1.tv_usec-t0.tv_usec;
  fprintf(stderr, "Elapsed = %ld us, rate = %f rw/sec\n", elapsed_us, (double) 1000000*loop_count/elapsed_us);
  
  return (0);
}
