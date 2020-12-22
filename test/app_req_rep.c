/*
 * APP_REQ_REP.C
 *   Simple Request-Reply (client-server) application to test the new HAL API
 *   December 2020, Perspecta Labs
 *
 * 1) Compilation (with the necessary includes and libraries):
 *   cd ~/gaps/build/src/hal/test/
 *   make clean; make
 *
 * 2) View APP Options:
 *   ./app_req_rep -h
 *
 * 3) Run APP via HAL and Network (after HAL daemon is running and NET is up) in all enclaves
 *    (currently support green and orange enclaves). Defaults to BE (ILIP device) Networking;
 *    but can add -u option (see example [g] below) in order to run with BW (UDP/IP) Networking.
 *
 *    [a] Default Flow: Green enclave sends position <1,1,1>; Orange replies with posiiton <2,2,1>
 *      green:   ./app_req_rep
 *      oprange: ./app_req_rep -e o
 *
 *    [b] Reverse: Orange enclave sends position <2,2,1>; Green replies with posiiton <1,1,1>
 *      green:   ./app_req_rep -r
 *      oprange: ./app_req_rep -e o -r
 *
 *    [c] Raw Data: Green enclave sends raw data <1,1,3>; Orange replies with posiiton <2,2,1>
 *          The '-g 0' option, means orange expect to receive a request with raw data (of any size)
 *      orange:  ./app_req_rep -e o -g 0
 *          The Green APP sends raw data <1,1,3>, sending a buffer of sequenctial numbers
 *          whose size (in Bytes) is defined by the '-g' option.
 *      green:   ./app_req_rep -g 1000
 *
 *    [d] Big Tag: Green enclave sends position <1,1,0x01234567>; Orange replies with posiiton <2,2,1>
 *      green:   ./app_req_rep -G
 *      oprange: ./app_req_rep -e o -G
 *
 *    [e] Timeout: Green enclave sends posiiton <1,1,1>; Orange replies with raw data <2,2,3>
 *          The Orange APP replies to one request with raw data <2,2,3>, sending a buffer of
 *          sequenctial numbers whose size (in Bytes) is defined by the '-o' option.
 *      orange:  ./app_req_rep -e o -o 150
 *          The Green APP sends requests with position <1,1,1> information.
 *          The '-n 2' option configures the APP to send 2 sequential requests;
 *          Green will report timeouts (set to 3 seconds using the '-b' option) and uses the
 *          '-o' option, so it knows to expect raw data (without specifying the size).
 *      green:   ./app_req_rep -n 2 -b 3000 -o 0
 *          We can repeat the orange's command to respond to the second request:
 *      orange:  ./app_req_rep -e o -o 100
 *
 *    [f] Big: Green enclave sends raw data <1,1,3>; Orange replies with raw data <2,2,3>
 *             (HAL must be configured to use BE (ILIP device) Pyaload Mode in both directions)
 *      orange:  ./app_req_rep -e o -o 800 -g 0
 *      green:   ./app_req_rep -o 0 -o 0 -g 400
 *
 *    [g] Big UDP: Green enclave sends raw data <1,1,3>; Orange replies with raw data <2,2,3>
 *                 (HAL must be configured to use BW (UDP) in both directions)
 *      green:   ./app_req_rep -u -o 0 -g 700
 *      oprange: ./app_req_rep -e o -u -o 900 -g 0
 */

#include "../api/xdcomms.h"
#include "../appgen/6month-demo/gma.h"
#include <stdio.h>
#include <sys/time.h>

/**********************************************************************/
/* Get options */
/*********t************************************************************/
/* Default option values */
int  mux_g2o = 1, sec_g2o = 1, typ_g2o = DATA_TYP_POSITION;    /* one-way request tag */
int  mux_o2g = 2, sec_o2g = 2, typ_o2g = DATA_TYP_POSITION;    /* one-way reply tag */
int  log_level            = 2;
char enclave              = 'g';
int  reverse_flow         = 0;    // green client; orange server
int  receive_first        = 0;
int  loop_count           = 1;
int  sub_block_timeout_ms = -1;
int  copy_buf_size        = -1;
# define IPC_ADDR_MAX 50
char xdc_addr_sub_green[IPC_ADDR_MAX]  = "ipc:///tmp/halsubbegreen";
char xdc_addr_pub_green[IPC_ADDR_MAX]  = "ipc:///tmp/halpubbegreen";
char xdc_addr_sub_orange[IPC_ADDR_MAX] = "ipc:///tmp/halsubbeorange";
char xdc_addr_pub_orange[IPC_ADDR_MAX] = "ipc:///tmp/halpubbeorange";

/* Print options */
void opts_print(void) {
  printf("Test program for sending requests and replies using the GAPS Hardware Abstraction Layer (HAL)\n");
  printf("Usage: ./app_req_rep [Options]\n");
  printf("[Options]:\n");
  printf(" -b : Subscriber Blocking timeout (in milliseconds): default = -1 (blocking) \n");
  printf(" -e : Enclave (single char). Currently support 'g' or 'o': default = 'g'\n");
  printf(" -g : Green sends Raw data type (3) of specified size (in bytes): default = position type (1) \n");
  printf(" -G : Green sends BIG data type (0x01234567): default = position type (1) \n");
  printf(" -h : Print this message\n");
  printf(" -l : log level: 0=TRACE, 1=DEBUG, 2=INFO, 3=WARN, 4=ERROR, 5=FATAL (default = 2)\n");
  printf(" -n : Number of request-response loops: Default = 1\n");
  printf(" -o : Orange sends Raw data type (3) of specified size (in bytes): default = position type (1) \n");
  printf(" -r : Reverse Client and Server: default = Green client, orange server\n");
  printf(" -u : URLs for HAL use BW option (UDP/IP): Default = BE options\n");
}

/* Parse the configuration file */
void opts_get(int argc, char **argv) {
  int opt, v;
  while((opt =  getopt(argc, argv, "b:e:g:Ghl:n:o:ru")) != EOF)
  {
    switch (opt)
    {
      case 'b':
        sub_block_timeout_ms = atoi(optarg);
        break;
      case 'e':
        enclave = *optarg;      /* e.g.. green = 'g', orange = 'o' */
        break;
      case 'g':
        v = atoi(optarg);
        if (v > 0) copy_buf_size = v;
        typ_g2o = DATA_TYP_RAW;     /* ILIP ACM supports <1 1 3> */
        break;
      case 'G':
        typ_g2o = DATA_TYP_BIG;     /* Must have added <1 1 0x01234567> into ILIP ACM */
        break;
      case 'h':
        opts_print();
        exit(0);
      case 'l':
        log_level = atoi(optarg);;
        break;
      case 'n':
        loop_count = atoi(optarg);
        break;
      case 'o':
        v = atoi(optarg);
        if (v > 0) copy_buf_size = v;
        typ_o2g = DATA_TYP_RAW;
        sec_o2g = 3;               /* ILIP ACM supports <2 3 3> not <2 2 3> */
        break;
      case 'r':
        reverse_flow = 1;
        break;
      case 'u':
        strcpy(xdc_addr_sub_green,  "ipc:///tmp/halsubbwgreen");
        strcpy(xdc_addr_pub_green,  "ipc:///tmp/halpubbwgreen");
        strcpy(xdc_addr_sub_orange, "ipc:///tmp/halsubbworange");
        strcpy(xdc_addr_pub_orange, "ipc:///tmp/halpubbworange");
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
  fprintf(stderr, "%c channels: g2o-tag=[%d, %d, %d] o2g-tag=[%d, %d, %d]\n", enclave, mux_g2o, sec_g2o, typ_g2o, mux_o2g, sec_o2g, typ_o2g);
  fprintf(stderr, "Params: timeout=%d, loop_count=%d, reverse_flow=%d, copy_buf_size=%d, xdc_log_level=%d\n", sub_block_timeout_ms, loop_count, reverse_flow, copy_buf_size, log_level);
  fprintf(stderr, "API URIs: gsub=%s, gpub=%s, osub=%s, opub=%s\n", xdc_addr_sub_green, xdc_addr_pub_green, xdc_addr_sub_orange, xdc_addr_pub_orange);
}

/**********************************************************************/
/* Specify Application Data Unit (ADU) Information */
/*********t************************************************************/
void trailer_set (trailer_datatype *trl) {
  static int n = 1;

  trl->seq  =  n;
  trl->rqr  =  0;
  trl->oid  =  0;
  trl->mid  =  0;
  trl->crc  =  0;
  n += 1;
}

/* Create Position ADU and return its length (changes z value for each call) */
size_t position_set (uint8_t *adu) {
  static double         z = 102;     /* changing value intitialized */
  position_datatype  *out = (position_datatype *) adu;
  
  out->x    = -74.574489;
  out->y    =  40.695545;
  out->z    =  z;
  z += 0.1;                   /* changing value for next call */
  trailer_set(&(out->trailer));
  return((size_t) sizeof(*out));
}

/* Create Raw ADU and return its length  */
size_t raw_set (uint8_t *adu, size_t len) {
  uint32_t       i;
  raw_datatype  *out  = (raw_datatype *) adu;
  uint8_t       *data = (uint8_t *) (out + 1);
  
  out->data_len = len;
  trailer_set(&(out->trailer));
  for (i=0; i<len; i++) data[i] = (uint8_t) (i % 256);
  return((size_t) sizeof(*out) + len);
}

/**********************************************************************/
/* Send and receive (or receive and send) one ADU */
/*********t************************************************************/
/* Send and print one message */
void send_one(uint8_t *adu, size_t *adu_len, gaps_tag *tag_pub, void *socket, int new_flag) {
  fprintf(stderr, "app tx: ");
  switch (tag_pub->typ) {
    case DATA_TYP_POSITION:
    case DATA_TYP_BIG:
      if (new_flag == 1) *adu_len = position_set(adu);
      position_print((position_datatype *) adu);
      break;
    case DATA_TYP_RAW:
      if (new_flag == 1) *adu_len = raw_set(adu, copy_buf_size);
      raw_print((raw_datatype *) adu);
      break;
    default:
      fprintf(stderr, "Undefined Publish data type: %d\n", tag_pub->typ);
      exit(3);
  }
  xdc_asyn_send(socket, adu, tag_pub);
}


/* Receive and print one message */
void recv_one(uint8_t *adu, size_t *adu_len, gaps_tag *tag_pub, gaps_tag *tag_sub, void *socket_pub, void *socket_sub) {
  int rv=-1;
//  xdc_blocking_recv(socket_sub, adu, r_tag);     /* 6month API only supports blocking receive */
  rv = xdc_recv(socket_sub, adu, tag_sub);
  if (rv > 0) {
    fprintf(stderr, "app rx: ");
    switch (tag_sub->typ) {
      case DATA_TYP_POSITION:
      case DATA_TYP_BIG:
        position_print((position_datatype *) adu);
        break;
      case DATA_TYP_RAW:
        raw_print((raw_datatype *) adu);
        break;
      default:
        fprintf(stderr, "Undefined Subscribe data type: %d\n", tag_sub->typ);
        exit(3);
    }
  }
  else {
    fprintf(stderr, "app rx failed: rv=%d (timeout=%d ms) ", rv, sub_block_timeout_ms);
    send_one(adu, adu_len, tag_pub, socket_pub, 0);
    recv_one(adu, adu_len, tag_pub, tag_sub, socket_pub, socket_sub);
  }
}

/* One send & one receive (or one receive & one send)  */
void send_and_recv(gaps_tag *tag_pub, gaps_tag *tag_sub, void *socket_pub, void *socket_sub, int receive_first) {
  uint8_t   adu[ADU_SIZE_MAX_C];
  size_t    adu_len;
  
  if (receive_first == 0) send_one(adu, &adu_len, tag_pub, socket_pub, 1);
  recv_one(adu, &adu_len, tag_pub, tag_sub, socket_pub, socket_sub);
  if (receive_first == 1) send_one(adu, &adu_len, tag_pub, socket_pub, 1);
}
         
/**********************************************************************/
/* Conifgure API, Run experiment and collect results */
/*********t************************************************************/
int main(int argc, char **argv) {
  int             i, receive_first=0;
  gaps_tag        tag_pub, tag_sub;
  struct timeval  t0, t1;
  long            elapsed_us;
  void            *socket_pub, *socket_sub;
  
  opts_get (argc, argv);
  xdc_log_level(log_level);

  /* A) Configure TAGS and ADDRESSES (values depend on the enclave) */
  switch(enclave) {
    case 'g':
      tag_write(&tag_pub, mux_g2o, sec_g2o, typ_g2o);
      tag_write(&tag_sub, mux_o2g, sec_o2g, typ_o2g);
      xdc_set_in(xdc_addr_sub_green);
      xdc_set_out(xdc_addr_pub_green);
      if (reverse_flow == 1) receive_first = 1;
      break;
    case 'o':
      tag_write(&tag_sub, mux_g2o, sec_g2o, typ_g2o);
      tag_write(&tag_pub, mux_o2g, sec_o2g, typ_o2g);
      xdc_set_in(xdc_addr_sub_orange);
      xdc_set_out(xdc_addr_pub_orange);
      if (reverse_flow == 0) receive_first = 1;
      break;
    default:
      fprintf(stderr, "Undefined enclave: %c\n", enclave);
      exit(3);
  }
  
  /* B) Configure Encoders and Decoders for all data types (same for both enclaves) */
  if ((typ_g2o == DATA_TYP_POSITION) || (typ_o2g = DATA_TYP_POSITION))
    xdc_register(position_data_encode, position_data_decode, DATA_TYP_POSITION);
  if ((typ_g2o == DATA_TYP_RAW) || (typ_o2g = DATA_TYP_RAW))
    xdc_register(raw_data_encode, raw_data_decode, DATA_TYP_RAW);
  if ((typ_g2o == DATA_TYP_BIG) || (typ_o2g = DATA_TYP_BIG))
    xdc_register(position_data_encode, position_data_decode, DATA_TYP_BIG);

  /* C) Configure ZMQ Pub and Sub Interfaces (same for both enclaves) */
  xdc_ctx();
  socket_pub = xdc_pub_socket();
//  socket_sub = xdc_sub_socket(tag_sub);   /* 6month API does not support timeout parameter */
  socket_sub = xdc_sub_socket_non_blocking(tag_sub, sub_block_timeout_ms);
  
  // D) Run Experiment (Calculating delay and read-write loop rate)
  gettimeofday(&t0, NULL);
  for (i=0; i<loop_count; i++) send_and_recv(&tag_pub, &tag_sub, socket_pub, socket_sub, receive_first);
  gettimeofday(&t1, NULL);
  elapsed_us = ((t1.tv_sec-t0.tv_sec)*1000000) + t1.tv_usec-t0.tv_usec;
  fprintf(stderr, "Elapsed = %ld us, rate = %f rw/sec\n", elapsed_us, (double) 1000000*loop_count/elapsed_us);
  
  return (0);
}