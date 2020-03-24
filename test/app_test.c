/*
 * APP_TEST.C
 *   Sample test application using HAL API to communicate with GAPS hardware.
 *
 * March 2020, Perspecta Labs
 */

#include "../api/xdcomms.h"
#include "../appgen/pnt.h"
#include "../appgen/xyz.h"
#include "../appgen/gma.h"
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
int experiment_num   = 0;

/* Print options */
void opts_print(void) {
  printf("Test program for sending and receiving data to and from the GAPS Hardware Abstraction Layer (HAL)\n");
  printf("Usage: ./app_test [Options] [Experiment Number]\n");
  printf("[Options]:\n");
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
  printf("[Experiment Number]: Optional override of send and recv tags (-m -s -t -M -S -T options)\n");
  
  printf(" With HAL config file sample.cfg, the numbers map to the following send/recv network tags:");
  printf("\n  [6xxx] are for BE devices xdd6 (mux=1) & xdd7 (mux=2) with send/recv network tags:\n    ");
  printf("6111 → <1,1,1>, ");
  printf("6113 → <1,1,3>, ");
  printf("6221 → <2,2,1>, ");
  printf("6222 → <2,2,2>, ");
  printf("6233 → <2,3,3>. ");
  printf("\n  [3xxx] are for a BW device xdd3:\n    ");
  printf("3111 → <1,1,1>, ");
  printf("3221 → <2,2,1>, ");
  printf("3222 → <2,2,2>. ");
  printf("\n  [xxxx] are for other devices:\n    ");
  printf("1553 → <5,5,3> on xdd1, ");
  printf("3221 → <6,6,4> on xdd4. ");
  printf("\n");
}

/* get precanned experiment number for command line */
void set_precanned_tags(int experiment_num) {
  fprintf(stderr, "exp=%d: ", experiment_num);
  switch (experiment_num) {
    /* Unidirectional flows for BE devices; xdd6 (mux=1) and xdd7 (mux=2) */
    /* Note: typ=1 sent by both enclaves; typ=2 is sent by one, typ=3 (maps from 101) for testing */
    case 6111:
      s_mux = 1; s_sec = 1; s_typ = 1;        /* Position data */
      break;
    case 6113:
      s_mux = 1; s_sec = 1; s_typ = 101;      /* PNT data */
      break;
    case 6221:
      s_mux = 2; s_sec = 2; s_typ = 1;        /* Position data */
      break;
    case 6222:
      s_mux = 2; s_sec = 2; s_typ = 2;        /* Distance data */
      break;
    case 6233:
      s_mux = 2; s_sec = 3; s_typ = 101;      /* PNT data */
      break;
    /* 3 unidirectional flows (same as above), but for device xdd3 (BW) - */
    case 3111:
      s_mux = 11; s_sec = 11; s_typ = 1;      /* Position data */
      break;
    case 3221:
      s_mux = 12; s_sec = 12; s_typ = 1;      /* Position data */
      break;
    case 3222:
      s_mux = 12; s_sec = 12; s_typ = 2;      /* Distance data */
      break;
    /* Additional cases */
    case 1553:
      s_mux = 5; s_sec = 5; s_typ = 101;      /* PNT data on xdd1 */
      break;
    case 4664:
      s_mux = 6; s_sec = 6; s_typ = 102;      /* XYZ data on xdd4 */
      break;
    case 0:
      break;
    default:
      fprintf(stderr, "\nExiting: Undefined demo-number (%d)\n", experiment_num);
      opts_print();
      exit(2);
  }
  r_mux = s_mux; r_sec = s_sec; r_typ = s_typ;  /* test app acts as sender & receiver */
}

/* Parse the configuration file */
void opts_get(int argc, char **argv) {
  int opt;
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
        fprintf(stderr, "\nSkipping undefined option (%c)\n", opt);
    }
  }
  if(optind<argc) set_precanned_tags(atoi(argv[optind++]));

  fprintf(stderr, "send-tag = [%d, %d, %d] recv-tag = [%d, %d, %d] ", s_mux, s_sec, s_typ, r_mux, r_sec, r_typ);
  fprintf(stderr, "loop_count=%d, loop_pause_us=%d, receive_first=%d\n", loop_count, loop_pause_us, receive_first);
}

/**********************************************************************/
/* Specify Application Data Unit (ADU) Information */
/*********t************************************************************/
void position_set (uint8_t *adu, size_t *len) {
  static double  z = 102;     /* changing value intitialized */
  position_datatype  *xyz = (position_datatype *) adu;
  trailer_datatype   *trl = &(xyz->trailer);
  
  xyz->x    = -74.574489;
  xyz->y    =  40.695545;
  xyz->z    =  z;
  trl->seq  =  0;
  trl->rqr  =  0;
  trl->oid  =  0;
  trl->mid  =  0;
  trl->crc  =  0;
  *len=(size_t) sizeof(*xyz);
  position_print(xyz);
  z += 0.1;                   /* changing value for next call */
}

void distance_set (uint8_t *adu, size_t *len) {
  static double  z = 0.5;    /* changing value intitialized */
  distance_datatype  *xyz = (distance_datatype *) adu;
  trailer_datatype   *trl = &(xyz->trailer);

  xyz->x    = -1.021;
  xyz->y    =  2.334;
  xyz->z    =  z;
  trl->seq  =  0;
  trl->rqr  =  0;
  trl->oid  =  0;
  trl->mid  =  0;
  trl->crc  =  0;
  *len=(size_t) sizeof(*xyz);
  distance_print(xyz);
  z += 0.1;                   /* changing value for next call */
}

void pnt_set (uint8_t *adu, size_t *len) {
  static int  AltFrac=0;    /* changing value intitialized */
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
  AltFrac++;                   /* changing value for next call */
}

void xyz_set (uint8_t *adu, size_t *len) {
  static double  z = 3.3;    /* changing value intitialized */
  xyz_datatype  *xyz = (xyz_datatype *) adu;

  xyz->x = 1.1;
  xyz->y = 2.2;
  xyz->z = z;
  *len=(size_t) sizeof(*xyz);
  xyz_print(xyz);
  z += 0.1;                   /* changing value for next call */
}

/**********************************************************************/
/* Send and receive (or receive and send) one ADU */
/*********t************************************************************/
/* Create, send and print one message */
void send_one(uint8_t *adu, size_t *adu_len, gaps_tag *s_tag) {
  fprintf(stderr, "app tx ");
  tag_print(s_tag);
  switch (s_tag->typ) {
    case DATA_TYP_POSITION:
      position_set(adu, adu_len);
      break;
    case DATA_TYP_DISTANCE:
      distance_set(adu, adu_len);
      break;
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
  xdc_asyn_send(adu, *s_tag);
}

/* Create, send and print one message */
void recv_one(uint8_t *adu, size_t *adu_len, gaps_tag *r_tag) {
  xdc_blocking_recv(adu, r_tag);
  fprintf(stderr, "app rx ");
  tag_print(r_tag);
  switch (r_tag->typ) {
    case DATA_TYP_POSITION:
      position_print((position_datatype *) adu);
      break;
    case DATA_TYP_DISTANCE:
      distance_print((distance_datatype *) adu);
      break;
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
         
/**********************************************************************/
/* Conifgure API, Run experiment and collect results */
/*********t************************************************************/
int main(int argc, char **argv) {
  int             i;
  gaps_tag        s_tag, r_tag;
  struct timeval  t0, t1;
  long            elapsed_us;
  
  /* A) Conigure API */
  opts_get (argc, argv);
  tag_write(&s_tag, s_mux, s_sec, s_typ);  tag_write(&r_tag, r_mux, r_sec, r_typ);
  /* Low level API registers a manually created encode and decode function per */
  /* data type. This will be replaced by an xdc_generate function */
  xdc_register(position_data_encode, position_data_decode, DATA_TYP_POSITION);
  xdc_register(distance_data_encode, distance_data_decode, DATA_TYP_DISTANCE);
  xdc_register(pnt_data_encode,      pnt_data_decode,      DATA_TYP_PNT);
  xdc_register(xyz_data_encode,      xyz_data_decode,      DATA_TYP_XYZ);
  
  /* B) Run Experiment (Calculating delay and read-write loop rate) */
  gettimeofday(&t0, NULL);
  for (i=0; i<loop_count; i++) send_and_recv(&s_tag, &r_tag);  // usleep(1111);
  gettimeofday(&t1, NULL);
  elapsed_us = ((t1.tv_sec-t0.tv_sec)*1000000) + t1.tv_usec-t0.tv_usec;
  fprintf(stderr, "Elapsed = %ld us, LoopPause = %d us, rate = %f rw/sec\n", elapsed_us, loop_pause_us, (double) 1000000*loop_count/elapsed_us);
  
  return (0);
}
