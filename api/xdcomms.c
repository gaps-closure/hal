/*
 * XDCOMMS.C
 *   CLOSURE API functions
 *
 * February 2020
 * A. McAuley, Perspecta Labs (amcauley@perspectalabs.com)
 *
 * For description and revision history see README.txt
 */

#include "xdcomms.h"

int clo_verbose=1;

/**********************************************************************/
/* LIB Printing Functions */
/**********************************************************************/

void tag_print (gaps_tag *tag) {
  fprintf(stderr, "[mux=%u sec=%u typ=%u] ", tag->mux, tag->sec, tag->typ);
}

/* Print raw data of specified length */
void data_print(const char *str, uint8_t *data, size_t data_len) {
  fprintf(stderr, "%s (len=%ld)", str, data_len);
  for (int i = 0; i < data_len; i++) {
    if ((i%4)==0) fprintf(stderr, " ");
    fprintf(stderr, "%02X", data[i]);
  }
  fprintf(stderr, "\n");
}

/**********************************************************************/
/* LIB Coding Functions for Header (TODO, Use DFDL schema) */
/**********************************************************************/

void tag_write (gaps_tag *tag, uint32_t mux, uint32_t sec, uint32_t typ) {
  tag->mux = mux;
  tag->sec = sec;
  tag->typ = typ;
}

void tag_read (gaps_tag *tag, uint32_t *mux, uint32_t *sec, uint32_t *typ) {
  *mux = tag->mux;
  *sec = tag->sec;
  *typ = tag->typ;
}

/* Serialize tag onto wire (TODO, Use DFDL schema) */
void tag_encode (gaps_tag *tag_out, gaps_tag *tag_in) {
  tag_out->mux = htonl(tag_in->mux);
  tag_out->sec = htonl(tag_in->sec);
  tag_out->typ = htonl(tag_in->typ);
}

/* Convert tag to local host format (TODO, Use DFDL schema) */
void tag_decode (gaps_tag *tag_out, gaps_tag *tag_in) {
  tag_out->mux = ntohl(tag_in->mux);
  tag_out->sec = ntohl(tag_in->sec);
  tag_out->typ = ntohl(tag_in->typ);
}

/* Convert tag to local host format (TODO, Use DFDL schema) */
void len_encode (uint32_t *out, size_t len) {
  *out = ntohl((uint32_t) len);
}

/* Convert tag to local host format (TODO, Use DFDL schema) */
void len_decode (size_t *out, uint32_t in) {
  *out = (uint32_t) htonl(in);
}

/**********************************************************************/
/* LIB Coding Functions for Data (TODO, Use DFDL schema) */
/**********************************************************************/
/*
 * Encode data of type dtype using DFDL schema
 */
void gaps_data_encode(uint8_t *buff_out, size_t *len_out, uint8_t *buff_in, size_t *len_in, int dtype) {
    
  switch (dtype) {
    case DATA_TYP_PNT:
      pnt_data_encode (buff_out, len_out, buff_in, len_in);
      break;
    default:
      fprintf(stderr, "Do not yet support data type = %d\n", dtype);
      exit (1);
  }
  if(clo_verbose) data_print("CLOSURE -- raw app data:", buff_in, *len_in);
  if(clo_verbose) data_print("        -> encoded data:", buff_out, *len_out);
}

/*
 * Encode data of type dtype using DFDL schema
 */
void gaps_data_decode(uint8_t *buff_out, size_t *len_out, uint8_t *buff_in, size_t *len_in, int dtype) {
    
  switch (dtype) {
    case DATA_TYP_PNT:
      pnt_data_decode (buff_out, len_out, buff_in, len_in);
      break;
    default:
      fprintf(stderr, "Do not yet support data type = %d\n", dtype);
      exit (1);
    }
  if(clo_verbose) data_print("CLOSURE -> raw app data:", buff_in, *len_in);
  if(clo_verbose) data_print("        -- encoded data:", buff_out, *len_out);
}

/**********************************************************************/
/* LIB Network (ZMQ-based) Functions */
/**********************************************************************/

void exit_with_zmq_error(const char* where) {
  fprintf(stderr,"%s error %d: %s\n",where,errno,zmq_strerror(errno));
  exit(-1);
}

/*
 * Connect to ZMQ socket
 */
void * z_connect(int type, const char *dest) {
  void   *socket, *ctx;
  int     err;

  ctx = zmq_ctx_new ();
  if(ctx == NULL) exit_with_zmq_error("zmq_ctx_new");
  socket = zmq_socket(ctx, type);
  if(socket == NULL) exit_with_zmq_error("zmq_socket");
  err = zmq_connect(socket, dest);
  if(err) exit_with_zmq_error("zmq_connect");

  if(clo_verbose) fprintf(stderr,"CLOSURE connected (s=%p t=%d) to %s\n", socket, type, dest);
  usleep(10000); // let connection establish before sending a message
  return (socket);
}

/*
 * Send ADU to HAL (which should be listening on the ZMQ subscriber socket)
 */
void gaps_asyn_send(uint8_t *adu, size_t adu_len, gaps_tag tag) {
  static int   do_once = 1;
  static void *socket;
  pkt_c         packet;
  pkt_c         *p=&packet;
  size_t        packet_len;
  
  /* a) Open connection with HAL ZMQ subscriber */
  if (do_once == 1) {
    socket = z_connect(ZMQ_PUB, HAL_IPC_SUB);
    do_once = 0;
  }
  
  /* b) Encode information into packet */
  tag_encode(&(p->tag), &tag);
  len_encode(&(p->data_len), adu_len);
  memcpy(p->data, adu, adu_len);
  packet_len = adu_len + sizeof(p->tag) + sizeof(p->data_len);
  
  /* c) Send a packet to HAL ZMQ subscriber */
  if(clo_verbose) {
    fprintf(stderr, "CLOSURE sending (on ZMQ s=%p): ", socket);
    tag_print(&tag);
    fprintf(stderr, "len=%ld ", adu_len);
    data_print("Packet", (uint8_t *) p, packet_len);
  }
  zmq_send (socket, (void *) p, packet_len, 0);
}

/*
 * Send ADU to HAL (which should be listening on the ZMQ publisher socket)
 */
void gaps_asyn_recv(uint8_t *adu, size_t *adu_len, gaps_tag *tag) {
  static int   do_once = 1;
  static void *socket;
  int          err;
  pkt_c         packet;
  pkt_c        *p=&packet;
  int          size;
  gaps_tag     tag4filter;
  
  /* a) Open connection with HAL ZMQ publisher */
  if (do_once == 1) {
//    socket = z_connect(ZMQ_SUB, HAL_IPC_PUB);
// Why can't I replace next 7 lines with above call????
    void *ctx = zmq_ctx_new ();
    if(ctx == NULL) exit_with_zmq_error("zmq_ctx_new");
    socket = zmq_socket(ctx, ZMQ_SUB);
    if(socket == NULL) exit_with_zmq_error("zmq_socket");
    err = zmq_connect(socket, HAL_IPC_PUB);
    if(err) exit_with_zmq_error("zmq_connect");
    if(clo_verbose) fprintf(stderr,"CLOSURE connects (s=%p t=%d) to %s\n", socket, ZMQ_SUB, HAL_IPC_PUB);

    tag_encode(&tag4filter, tag);
    err = zmq_setsockopt (socket, ZMQ_SUBSCRIBE, (void *) &tag4filter, RX_FILTER_LEN);
    assert (err == 0);
    do_once = 0;
  }
   
  /* b) Get a packet from HAL ZMQ publisher */
  if(clo_verbose) {
    fprintf(stderr, "CLOSURE waiting to recv (using len=%d filter ", RX_FILTER_LEN);
    uint8_t *f = (uint8_t *) &tag4filter;
    for (int i=0; i < RX_FILTER_LEN; i++) fprintf(stderr, "%.02x", *(f++));
    fprintf(stderr, ")\n");
  }
  size = zmq_recv (socket, (uint8_t *) p, PACKET_MAX, 0);
  if(clo_verbose) data_print("CLOSURE recv packet", (uint8_t *) p, size);

  /* c) Decode information from packet */
  tag_decode(tag, &(p->tag));
  len_decode(adu_len, p->data_len);
  memcpy(adu, p->data, *adu_len);
  if(clo_verbose) {
    fprintf(stderr, "CLOSURE extracts packet info: ");
    tag_print(tag);
    fprintf(stderr, "data_len=%lu\n", *adu_len);
  }
}
