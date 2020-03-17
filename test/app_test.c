/*
 * APP_TEST.C
 *   Sample test application using HAL API to communicate with GAPS hardware.
 *
 * March 2020, Perspecta Labs
 */

#include "../api/xdcomms.h"
#include "../appgen/pnt.h"
#include "../appgen/xyz.h"
#include <stdio.h>
#include <sys/time.h>

/**********************************************************************/
/* Get options */
/*********t************************************************************/
/* Default option values */
int s_mux = 1, s_sec = 1, s_typ = 1;    /* send tag */
int r_mux = 1, r_sec = 1, r_typ = 1;    /* recv tag */
int receive_first    = 0;
int loop_pause_us    = 0;
int loop_count       = 1;
int demo_number      = 0;

/* Print options */
void opts_print(void) {
  printf("Test program for sending/receiving data to/from the GAPS Hardware Abstraction Layer (HAL)\n");
  printf("Usage: app_test [OPTIONS]\n");
  printf(" -c : Number of send & receive loops: Default = 1\n");
  printf(" -h : Print this message\n");
  printf(" -p : Pause time (in microseconds) after send & receive loop: Default = 0\n");
  printf(" -r : Receive first (for each send & receive loop) \n");
  printf(" -m : Send multiplexing (mux) tag (integer): Default = 1\n");
  printf(" -s : Send security (sec)     tag (integer): Default = 1\n");
  printf(" -t : Send data type (typ)    tag (integer): Default = 1\n");
  printf(" -M : Recv multiplexing (mux) tag (integer): Default = 1\n");
  printf(" -S : Recv security (sec)     tag (integer): Default = 1\n");
  printf(" -T : Recv data type (typ)    tag (integer): Default = 1\n");
}

/* Parse the configuration file */
void opts_get(int argc, char **argv) {
  int opt;
//  char  *file_name= NULL;
  while((opt =  getopt(argc, argv, "c:hp:rm:s:t:M:S:T:")) != EOF)
  {
    switch (opt)
    {
      case 'c':
        loop_count = atoi(optarg);
        break;
      case 'h':
        opts_print();
        exit(0);
      case 'p':
        loop_pause_us = atoi(optarg);
        break;
      case 'r':
        receive_first = 1;
        break;
      case 'm':
        s_mux = atoi(optarg);
        break;
      case 's':
        s_sec = atoi(optarg);
        break;
      case 't':
        s_typ = atoi(optarg);
        break;
      case 'M':
        r_mux = atoi(optarg);
        break;
      case 'S':
        r_sec = atoi(optarg);
        break;
      case 'T':
        r_typ = atoi(optarg);
        break;
      case ':':
        fprintf(stderr, "\noption needs a value\n");
        opts_print();
        exit(0);
        break;
      default:
        fprintf(stderr, "\nSkipping undefined option (%d)\n", opt);
    }
  }
  if(optind<argc) {
    demo_number = atoi(argv[optind++]);
    fprintf(stderr, "demo_num=%d\n", demo_number);
    switch (demo_number) {
      case 1:
        s_mux = 1; s_sec = 1; s_typ = 1; r_mux = 1; r_sec = 1; r_typ = 1;
        break;
      case 2:
        s_mux = 2; s_sec = 2; s_typ = 2; r_mux = 2; r_sec = 2; r_typ = 2;
        break;
      case 5:
        s_mux = 5; s_sec = 5; s_typ = 1; r_mux = 6; r_sec = 6; r_typ = 1;
        break;
      case 11:
        s_mux = 11; s_sec = 11; s_typ = 1; r_mux = 12; r_sec = 12; r_typ = 1;
        break;
      case 13:
        s_mux = 13; s_sec = 13; s_typ = 1; r_mux = 14; r_sec = 14; r_typ = 1;
        break;
      default:
        fprintf(stderr, "\nSkipping undefined demo-number (%d)\n", opt);
    }
  }

  fprintf(stderr, "Read options: send-tag = [%d, %d, %d] recv-tag = [%d, %d, %d] ", s_mux, s_sec, s_typ, r_mux, r_sec, r_typ);
  fprintf(stderr, "loop_count=%d, loop_pause_us=%d, receive_first=%d\n", loop_count, loop_pause_us, receive_first);
//  return (file_name);
}

/* Data type 1 */
void pnt_set (uint8_t *adu, size_t *len) {
  static int  AltFrac=0;
  pnt_datatype  *pnt = (pnt_datatype *) adu;
  
  pnt->MessageID  = 130;
  pnt->TrackIndex = 165;
  pnt->Lon        = 100;
  pnt->LonFrac    = 32768;
  pnt->Lat        = 67;
  pnt->LatFrac    = 49152;
  pnt->Alt        = 2;
  pnt->AltFrac    = 0;
  *len=(size_t) sizeof(*pnt);
  pnt_print(pnt);
  AltFrac++;
}

/* Data type 2 */
void xyz_set (uint8_t *adu, size_t *len) {
  static double  z=3.3;
  xyz_datatype  *xyz = (xyz_datatype *) adu;

  xyz->x = 1.1;
  xyz->y = 2.2;
  xyz->z = z;
  *len=(size_t) sizeof(*xyz);
  xyz_print(xyz);
  z += 1;
}

/* Create, send and print one message */
void send_one(uint8_t *adu, size_t *adu_len, gaps_tag *s_tag) {
  fprintf(stderr, "app tx ");
  tag_print(s_tag);
  switch (s_tag->typ) {
    case DATA_TYP_PNT:
      pnt_set(adu, adu_len);
      break;
    case DATA_TYP_XYZ:
      xyz_set(adu, adu_len);
      break;
    default:
      fprintf(stderr, "\nUndefined data type (%d)\n", s_tag->typ);
      exit(2);
  }
  xdc_asyn_send(adu, *adu_len, *s_tag);
}

/* Create, send and print one message */
void recv_one(uint8_t *adu, size_t *adu_len, gaps_tag *r_tag) {
  xdc_blocking_recv(adu, adu_len, r_tag);
  fprintf(stderr, "app rx ");
  tag_print(r_tag);
  switch (r_tag->typ) {
    case DATA_TYP_PNT:
      pnt_print((pnt_datatype *) adu);
      break;
    case DATA_TYP_XYZ:
      xyz_print((xyz_datatype *) adu);
      break;
    default:
      fprintf(stderr, "\nUndefined data type (%d)\n", r_tag->typ);
      exit(2);
  }
}


/* One send & receive loop (using xdc API)  */
void send_and_recv(gaps_tag *s_tag, gaps_tag *r_tag) {
  uint8_t       adu[ADU_SIZE_MAX_C];
  size_t        adu_len;

  if (receive_first == 0) send_one(adu, &adu_len, s_tag);
  recv_one(adu, &adu_len, r_tag);
  if (receive_first == 1) send_one(adu, &adu_len, s_tag);
  usleep (loop_pause_us);
}
         
int main(int argc, char **argv) {
  int             i;
  gaps_tag        s_tag, r_tag;
  struct timeval  t0, t1;
  long            elapsed_us;
          
  opts_get (argc, argv);
  tag_write(&s_tag, s_mux, s_sec, s_typ);  tag_write(&r_tag, r_mux, r_sec, r_typ);
  /* Low level API registers a manually created encode and decode function per */
  /* data type. This will be replaced by an xdc_generate function */
  xdc_register(pnt_data_encode, pnt_data_decode, DATA_TYP_PNT);
  xdc_register(xyz_data_encode, xyz_data_decode, DATA_TYP_XYZ);
  gettimeofday(&t0, NULL);
  for (i=0; i<loop_count; i++) send_and_recv(&s_tag, &r_tag);  // usleep(1111);
  
  /* Calculate delay and read-write loop rate */
  gettimeofday(&t1, NULL);
  elapsed_us = ((t1.tv_sec-t0.tv_sec)*1000000) + t1.tv_usec-t0.tv_usec;
  fprintf(stderr, "Elapsed = %ld us, LoopPause = %d us, rate = %f rw/sec\n", elapsed_us, loop_pause_us, (double) 1000000*loop_count/elapsed_us);
  return (0);
}
