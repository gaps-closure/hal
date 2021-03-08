/*
 * APP_REQ_REP.C
 *   Simple Request-Reply (client-server) application to test the new HAL API
 *   February 2021, Perspecta Labs
 *
 * 1) Compilation (with the necessary includes and libraries):
 *   cd ~/gaps/build/src/hal/test/
 *   make clean; make
 *
 * 2) View APP Options:
 *   ./app_req_rep -h
 *
 * 3) Run APP via HAL and Network (after HAL daemon is running and NET is up) in all enclaves
 *
 *    [a] Default Flow: Green enclave-1 sends position <1,1,1>; Orange replies with posiiton <2,2,1>
 *      green:   ./app_req_rep
 *      oprange: ./app_req_rep -e 2
 *
 *    [b] Reverse: Orange enclave-2 sends position <2,2,1>; Green replies with posiiton <1,1,1>
 *      green:   ./app_req_rep -r
 *      oprange: ./app_req_rep -e 2 -r
 *
 *    [c] Raw Data: Green enclave-1 sends raw data <1,1,3>; Orange replies with posiiton <2,2,1>
 *          The '-g 0' option, means orange expect to receive a request with raw data (of any size)
 *      orange:  ./app_req_rep -e 2 -g 0
 *          The Green APP sends raw data <1,1,3>, sending a buffer of sequenctial numbers
 *          whose size (in Bytes) is defined by the '-g' option.
 *      green:   ./app_req_rep -g 1000
 *
 *    [d] Big Tag: Green enclave sends position <1,1,0x01234567>; Orange replies with posiiton <2,2,1>
 *      green:   ./app_req_rep -G
 *      oprange: ./app_req_rep -e 2 -G
 *
 *    [e] Timeout: Green enclave sends posiiton <1,1,1>; Orange replies with raw data <2,2,3>
 *          The Orange APP replies to one request with raw data <2,2,3>, sending a buffer of
 *          sequenctial numbers whose size (in Bytes) is defined by the '-o' option.
 *      orange:  ./app_req_rep -e 2 -o 150
 *          The Green APP sends requests with position <1,1,1> information.
 *          The '-n 2' option configures the APP to send 2 sequential requests;
 *          Green will report timeouts (set to 3 seconds using the '-t' option) and uses the
 *          '-o' option, so it knows to expect raw data (without specifying the size).
 *      green:   ./app_req_rep -n 2 -t 3000 -o 0
 *          We can repeat the orange's command to respond to the second request:
 *      orange:  ./app_req_rep -e 2 -o 100
 *
 *    [f] Big: Green enclave sends raw data <1,1,3>; Orange replies with raw data <2,2,3>
 *             (HAL must be configured to use BE (ILIP device) Pyaload Mode in both directions)
 *      orange:  ./app_req_rep -e 2 -o 800 -g 0
 *      green:   ./app_req_rep -o 0 -o 0 -g 400
 *
 *    [g] Big UDP: Green enclave sends raw data <1,1,3>; Orange replies with raw data <2,2,3>
 *                 (HAL is configured to 'be' ZMQ URIs)
 *      green:   ./app_req_rep -z 1 -o 0 -g 700
 *      orange:  ./app_req_rep -e 2 -z 1 -o 900 -g 0
 
 *    [x] EOP: Orange enclave sends HB/position <12,12,13>; Green replies with HB/position <111,111,113>
 *      green:   ./app_req_rep -r -q
 *      orange:  ./app_req_rep -r -q -e 2
 */

#include "../api/xdcomms.h"
#include "../appgen/6month-demo/gma.h"
#include <stdio.h>
#include <sys/time.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>

#define MODE_SERVER 0     /* Receive first, then send */
#define MODE_CLIENT 1     /* send first, then receive */
#define MODE_SOURCE 2     /* Send only */
#define MODE_SINK   3     /* Recv only */

/**********************************************************************/
/* Get options */
/*********t************************************************************/
/* Default option values */
uint32_t  mux_1_2 = 1, sec_1_2 = 1, typ_1_2 = DATA_TYP_POSITION;    /* request tag */
uint32_t  mux_2_1 = 2, sec_2_1 = 2, typ_2_1 = DATA_TYP_POSITION;    /* reply tag */
int  rx_count             = 0;
int  burst_size           = 1;
int  log_level            = 2;
int  enclave              = 1;
int  reverse_flow         = 0;    // normally enclave1 = client; enclave-2 = server
int  mode_uni             = 0;    // 0=RPC e1=client, e2=server   (1=UNI: e1=source, e2=sink)
int  loop_count           = 1;
int  sleep_nano           = 0;
bool verbose              = false;
int  sub_block_timeout_ms = -1;
int  copy_buf_size        = -1;
int  receive_first        = 0;

# define IPC_ADDR_MAX 128
char log_filename[IPC_ADDR_MAX]        = "z_";
char experimental_mode[IPC_ADDR_MAX]   = "m1";
char xdc_addr_sub_enc1[IPC_ADDR_MAX]   = "ipc:///tmp/halsubgreen";
char xdc_addr_pub_enc1[IPC_ADDR_MAX]   = "ipc:///tmp/halpubgreen";
char xdc_addr_sub_enc2[IPC_ADDR_MAX]   = "ipc:///tmp/halsuborange";
char xdc_addr_pub_enc2[IPC_ADDR_MAX]   = "ipc:///tmp/halpuborange";

/* Print options */
void opts_print(void) {
  printf("Test program for sending requests and replies using the GAPS Hardware Abstraction Layer (HAL)\n");
  printf("Usage: ./app_req_rep [Options]\n");
  printf("[Options]:\n");
  printf(" -b : Burst size: default = 1 (send and receive one message per request-response loop)\n");
  printf(" -e : Enclave index. Currently support 1 or 2: default = 1\n");
  printf(" -f : Log filename prefix: default = %s\n", log_filename);
  printf(" -g : Raw (3) data type from enclave 1 to 2 (enclave 1 specified size (in bytes); enclave 2 size must be 0: default = position (1)\n");
  printf(" -G : BIG data type (0x01234567) sent from enclave 1 to 2 (both sides must specify): default = position type (1) \n");
  printf(" -h : Print this message\n");
  printf(" -l : log level: 0=TRACE, 1=DEBUG, 2=INFO, 3=WARN, 4=ERROR, 5=FATAL (default = 2)\n");
  printf(" -m : Experimental Mode: default = %s\n", experimental_mode);
  printf(" -n : Number of request-response loops: Default = 1\n");
  printf(" -o : Raw (3) data type from enclave 1 to 2. Both enclaves must set and enclave 1 specified size (in bytes): default = position (1)\n");
  printf(" -p : HAL Loopback test <mux,sec,typ>: Send and Receive <1,1,1>\n");
  printf(" -q : EOP heartbeat test <mux,sec,typ>: Send <11,11,DATA_TYP_HB_ORANGE>, Receive <12,12,DATA_TYP_HB_GREEN>\n");
  printf(" -r : Reverse enclave1 and enclave 2 roles (default is enclave 1 is RPC Client or One-way source)\n");
  printf(" -s : Sleep (nanoseconds) before a send: default = 0 \n");
  printf(" -t : Timeout for subscriber receive (in milliseconds): default = -1 (blocking) \n");
  printf(" -u : Unidirectional mode (Enclave 1 = source, Enclave 2 = sink): default = RPC mode (Enclave 1 = client, Enclave 2 = server), \n");
  printf(" -z : URL index for HAL API : Default = ipc:///tmp/halsubgreen, 1=ipc:///tmp/example1suborange 2=ipc:///tmp/sock_suborange 5=ipc:///tmp/halsubbegreen 6=ipc:///tmp/halsubbwgreen\n");
  printf(" -v : Verbose mode\n");
}

/* Parse the configuration file */
void opts_get(int argc, char **argv) {
  int opt, value;
  while((opt =  getopt(argc, argv, "b:e:f:g:Ghl:n:o:pqrs:t:uvz:")) != EOF)
  {
    switch (opt)
    {
      case 'b':
        burst_size = atoi(optarg);
        break;
      case 'e':
        enclave = atoi(optarg);      /* default = 1 */
        break;
      case 'f':
        strcpy(log_filename, optarg);
        break;
      case 'g':
        value = atoi(optarg);
        if (value > 0) copy_buf_size = value;
        typ_1_2 = DATA_TYP_RAW;     /* ILIP ACM supports <1 1 3> */
        break;
      case 'G':
        typ_1_2 = DATA_TYP_BIG;     /* Must have added <1 1 0x01234567> into ILIP ACM */
        break;
      case 'h':
        opts_print();
        exit(0);
      case 'l':
        log_level = atoi(optarg);;
        break;
      case 'm':
        strcpy(experimental_mode, optarg);
        break;
      case 'n':
        loop_count = atoi(optarg);
        break;
      case 'o':
        value = atoi(optarg);
        if (value > 0) copy_buf_size = value;
        typ_2_1 = DATA_TYP_RAW;
        sec_2_1 = 3;               /* ILIP ACM supports <2 3 3> not <2 2 3> */
        break;
      case 'p':
        mux_2_1 = 1;
        sec_2_1 = 1;
        typ_2_1 = DATA_TYP_POSITION;
        mux_1_2 = 1;
        sec_1_2 = 1;
        typ_1_2 = DATA_TYP_POSITION;
        break;
      case 'Q':
        mux_2_1 = 12;
        sec_2_1 = 12;
        typ_2_1 = DATA_TYP_HB_ORANGE;
        mux_1_2 = 111;
        sec_1_2 = 111;
        typ_1_2 = DATA_TYP_HB_GREEN;
        break;
      case 'q':
        mux_2_1 = 3221225484;
        sec_2_1 = 12;
        typ_2_1 = DATA_TYP_HB_ORANGE;
        mux_1_2 = 1073741935;
        sec_1_2 = 111;
        typ_1_2 = DATA_TYP_HB_GREEN;
        break;
      case 'r':
        reverse_flow = 1;
        break;
      case 's':
        sleep_nano = atoi(optarg);
        break;
      case 't':
        sub_block_timeout_ms = atoi(optarg);
        break;
      case 'u':
        mode_uni = 1;
        break;
      case 'v':
        verbose = true;
        break;
      case 'z':
        value = atoi(optarg);
        if (value==1) {
          strcpy(xdc_addr_sub_enc1, "ipc:///tmp/example1suborange");
          strcpy(xdc_addr_pub_enc1, "ipc:///tmp/example1puborange");
          strcpy(xdc_addr_sub_enc2, "ipc:///tmp/example1subpurple");
          strcpy(xdc_addr_pub_enc2, "ipc:///tmp/example1pubpurple");
        }
        if (value==2) {
          strcpy(xdc_addr_sub_enc1, "ipc:///tmp/sock_subgreen");
          strcpy(xdc_addr_pub_enc1, "ipc:///tmp/sock_pubgreen");
          strcpy(xdc_addr_sub_enc2, "ipc:///tmp/sock_suborange");
          strcpy(xdc_addr_pub_enc2, "ipc:///tmp/sock_puborange");
//          mux_1_2 = 11; sec_2_1 = 11; typ_2_1 = 12;
//          mux_2_1 = 12; sec_1_2 = 12; typ_1_2 = 11;
        }
        if (value==5) {
          strcpy(xdc_addr_sub_enc1, "ipc:///tmp/halsubbegreen");
          strcpy(xdc_addr_pub_enc1, "ipc:///tmp/halpubbegreen");
          strcpy(xdc_addr_sub_enc2, "ipc:///tmp/halsubbeorange");
          strcpy(xdc_addr_pub_enc2, "ipc:///tmp/halpubbeorange");
        }
        if (value==6) {
          strcpy(xdc_addr_sub_enc1, "ipc:///tmp/halsubbwgreen");
          strcpy(xdc_addr_pub_enc1, "ipc:///tmp/halpubbwgreen");
          strcpy(xdc_addr_sub_enc2, "ipc:///tmp/halsubbworange");
          strcpy(xdc_addr_pub_enc2, "ipc:///tmp/halpubbworange");
        }
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
  fprintf(stderr, "Enclave-%d channels: 1-to-2-tag=[%u, %u, %u] 2-to-1-tag=[%u, %u, %u]\n", enclave, mux_1_2, sec_1_2, typ_1_2, mux_2_1, sec_2_1, typ_2_1);
  fprintf(stderr, "Params: timeout=%d, loops=%d, burst=%d, uni=%d reverse=%d, sleep=%dns, buf_size=%d, API-log=%d verbose=%s\n", sub_block_timeout_ms, loop_count, burst_size, mode_uni, reverse_flow, sleep_nano, copy_buf_size, log_level, verbose ? "true" : "false");
  fprintf(stderr, "API URIs: ");
  switch(enclave) {
    case 1:
      fprintf(stderr, "enc1_sub=%s, enc1_pub=%s", xdc_addr_sub_enc1, xdc_addr_pub_enc1);
      break;
    case 2:
      fprintf(stderr, "enc2_sub=%s, enc2_pub=%s", xdc_addr_sub_enc2, xdc_addr_pub_enc2);
      break;
  }
  fprintf(stderr, "\nExperiment Mode=%s, Log prefix=%s\n", experimental_mode, log_filename);
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
/* Send and print one message (do not optimize the sleep loop) */
#pragma GCC push_options
#pragma GCC optimize ("O0")
void send_one_burst(uint8_t *adu, size_t *adu_len, gaps_tag *tag_pub, void *socket, int new_flag, struct timespec *sleep_req) {
  int i, j;
  
  if (verbose) fprintf(stderr, "app tx: ");
  switch (tag_pub->typ) {
    case DATA_TYP_POSITION:
    case DATA_TYP_BIG:
    case DATA_TYP_HB_ORANGE:
    case DATA_TYP_HB_GREEN:
      if (new_flag == 1) *adu_len = position_set(adu);
      if (verbose) position_print((position_datatype *) adu);
      break;
    case DATA_TYP_RAW:
      if (new_flag == 1) *adu_len = raw_set(adu, copy_buf_size);
      if (verbose) raw_print((raw_datatype *) adu);
      break;
    default:
      fprintf(stderr, "Undefined Publish data type: %d\n", tag_pub->typ);
      exit(3);
  }
  for (i=0; i<burst_size; i++) {
    if (sleep_req != NULL) {
//    nanosleep has a minimum, as it is rounded up to the next multiple of the underlying unix clock
//      nanosleep(sleep_req, sleep_req);
      for (j=0; j<sleep_req->tv_nsec; j++) sleep_req->tv_sec += j;
    }
    xdc_asyn_send(socket, adu, tag_pub);
  }
}
#pragma GCC pop_options


/* Receive and print one message */
void recv_one_burst(uint8_t *adu, size_t *adu_len, gaps_tag *tag_pub, gaps_tag *tag_sub, void *socket_pub, void *socket_sub) {
  int rv=-1, i;
//  xdc_blocking_recv(socket_sub, adu, r_tag);     /* 6month API only supports blocking receive */
  for (i=0; i<burst_size; i++) { rv = xdc_recv(socket_sub, adu, tag_sub); rx_count++; }
  if (rv > 0) {
    if (verbose) fprintf(stderr, "app rx: ");
    switch (tag_sub->typ) {
      case DATA_TYP_POSITION:
      case DATA_TYP_BIG:
      case DATA_TYP_HB_ORANGE:
      case DATA_TYP_HB_GREEN:
        if (verbose) position_print((position_datatype *) adu);
        break;
      case DATA_TYP_RAW:
        if (verbose) raw_print((raw_datatype *) adu);
        break;
      default:
        fprintf(stderr, "Undefined Subscribe data type: %d\n", tag_sub->typ);
        exit(3);
    }
  }
  else {
    fprintf(stderr, "app rx failed: rv=%d (after a timeout=%d ms) ", rv, sub_block_timeout_ms);
    send_one_burst(adu, adu_len, tag_pub, socket_pub, 0, (struct timespec *) NULL);
    recv_one_burst(adu, adu_len, tag_pub, tag_sub, socket_pub, socket_sub);
  }
}

/* One send & one receive (or one receive & one send)  */
void send_and_recv(gaps_tag *tag_pub, gaps_tag *tag_sub, void *socket_pub, void *socket_sub, struct timespec *sleep_req) {
  uint8_t         adu[ADU_SIZE_MAX_C];
  size_t          adu_len;
  
  if (mode_uni == 0) {      /* 2-way flow (RPC - alternates read and write) */
    if (receive_first == 0) send_one_burst(adu, &adu_len, tag_pub, socket_pub, 1, sleep_req);
    recv_one_burst(adu, &adu_len, tag_pub, tag_sub, socket_pub, socket_sub);
    if (receive_first == 1) send_one_burst(adu, &adu_len, tag_pub, socket_pub, 1, sleep_req);
  }
  else {                    /* 1-way flow (unidirection flow) ) */
    if (receive_first == 0) send_one_burst(adu, &adu_len, tag_pub, socket_pub, 1, sleep_req);
    else                    recv_one_burst(adu, &adu_len, tag_pub, tag_sub, socket_pub, socket_sub);
  }
}
         
/**********************************************************************/
/* Conifgure API, Run experiment and collect results */
/*********t************************************************************/
void log_results(long elapsed_us) {
  FILE             *fp;
  char              header_str[IPC_ADDR_MAX], enclave_str[IPC_ADDR_MAX];
  int               send_count, expected_count=0;
  double            duration_seconds;
  
  send_count       = loop_count * burst_size;
  duration_seconds = (double) elapsed_us/1000000;
  if ( (mode_uni == 0) || (receive_first == 1) )  expected_count = send_count;
  fprintf(stderr, "Elapsed = %ld us, rate = %f msgs/sec (%d of %d expected)\n", elapsed_us, (double) 1000000*send_count/elapsed_us, rx_count, expected_count);
  
  /* Write results to log file */
  sprintf(enclave_str, "%d", enclave);
  strcat(log_filename, enclave_str);
  strcpy(header_str, "");
  if( access(log_filename, F_OK) != 0 ) {
    sprintf(header_str, "%lu\nMode_%d, Tx_Count_%d, Tx_Burst_%d, Rx_Count_%d, Loss_%d %%, Sleep_%d ns, Duration_%d sec, Rate_%d msgs/sec\n", (unsigned long)time(NULL), enclave, enclave, enclave, enclave, enclave, enclave, enclave, enclave);
  }
  fp = fopen(log_filename, "a");
  fprintf(fp, "%s", header_str);
  fprintf(fp, "%s, %d, %d, %d, %.3f, %d, %.3f, %.3f\n", experimental_mode, send_count, burst_size, rx_count, (double) 100*rx_count/send_count, sleep_nano, duration_seconds, (double) send_count/duration_seconds);
  fclose(fp);
}


/* Signal Handler for SIGINT - print statistics */
void sigintHandler(int sig_num)
{
  log_results((long) 0);
  exit(0);
}

// Run Experiment (Calculating delay and read-write loop rate) and collect results
void run_experiment(gaps_tag *tag_pub, gaps_tag *tag_sub, void *socket_pub, void *socket_sub) {
  int               i;
  long              elapsed_us;
  struct timeval    t0, t1;
  struct timespec   sleep_req = {0}, *sleep_req_ptr=NULL;
  
  sleep_req.tv_sec = 0;
  sleep_req.tv_nsec = sleep_nano;
  if (sleep_nano > 0) sleep_req_ptr = &sleep_req;

  gettimeofday(&t0, NULL);
  for (i=0; i<loop_count; i++) send_and_recv(tag_pub, tag_sub, socket_pub, socket_sub, sleep_req_ptr);
  gettimeofday(&t1, NULL);
  elapsed_us = ((t1.tv_sec-t0.tv_sec)*1000000) + t1.tv_usec-t0.tv_usec;

  log_results(elapsed_us);
}

/* Set HAL API parameters */
int main(int argc, char **argv) {
  gaps_tag   tag_pub, tag_sub;
  void      *socket_pub, *socket_sub;
  
  signal(SIGINT, sigintHandler);    /* Initialize signal handler (for counting rx packet if not complete) */
  opts_get (argc, argv);
  xdc_log_level(log_level);
  
  /* A) Configure TAGS and ADDRESSES (values depend on the enclave) */
  switch(enclave) {
    case 1:
      tag_write(&tag_pub, mux_1_2, sec_1_2, typ_1_2);
      tag_write(&tag_sub, mux_2_1, sec_2_1, typ_2_1);
      xdc_set_in(xdc_addr_sub_enc1);
      xdc_set_out(xdc_addr_pub_enc1);
      if (reverse_flow == 1) receive_first = 1;
      break;
    case 2:
      tag_write(&tag_sub, mux_1_2, sec_1_2, typ_1_2);
      tag_write(&tag_pub, mux_2_1, sec_2_1, typ_2_1);
      xdc_set_in(xdc_addr_sub_enc2);
      xdc_set_out(xdc_addr_pub_enc2);
      if (reverse_flow == 0) receive_first = 1;
      break;
    default:
      fprintf(stderr, "Undefined enclave index: %d\n", enclave);
      exit(3);
  }
  
  /* B) Configure Encoders and Decoders for all data types (same for both enclaves) */
  if ((typ_1_2 == DATA_TYP_POSITION) || (typ_2_1 = DATA_TYP_POSITION))
    xdc_register(position_data_encode, position_data_decode, DATA_TYP_POSITION);
  if ((typ_1_2 == DATA_TYP_RAW) || (typ_2_1 = DATA_TYP_RAW))
    xdc_register(raw_data_encode, raw_data_decode, DATA_TYP_RAW);
  if ((typ_1_2 == DATA_TYP_BIG) || (typ_2_1 = DATA_TYP_BIG))
    xdc_register(position_data_encode, position_data_decode, DATA_TYP_BIG);
  if ((typ_1_2 == DATA_TYP_HB_ORANGE) || (typ_2_1 = DATA_TYP_HB_ORANGE))
    xdc_register(position_data_encode, position_data_decode, DATA_TYP_HB_ORANGE);
  if ((typ_1_2 == DATA_TYP_HB_GREEN)  || (typ_2_1 = DATA_TYP_HB_GREEN))
    xdc_register(position_data_encode, position_data_decode, DATA_TYP_HB_GREEN);

  /* C) Configure ZMQ Pub and Sub Interfaces (same for both enclaves) */
  xdc_ctx();
  socket_pub = xdc_pub_socket();
//  socket_sub = xdc_sub_socket(tag_sub);   /* 6month API does not support timeout parameter */
  socket_sub = xdc_sub_socket_non_blocking(tag_sub, sub_block_timeout_ms);
  
  run_experiment(&tag_pub, &tag_sub, socket_pub, socket_sub);
  return (0);
}
