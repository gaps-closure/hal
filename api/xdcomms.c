/*
 * Cross Domain (XD) Communication API between Applicaitons and GAP XD Guards
 *   March 2020, Perspecta Labs
 *
 * TODO:
 *  XXX: enclose data encode/decode API functions by passing data file description
 */

#include "xdcomms.h"

int xdc_verbose=0;

/**********************************************************************/
/* LIB Printing Functions */
/**********************************************************************/

void tag_print (gaps_tag *tag) {
  fprintf(stderr, "[mux=%02u sec=%02u typ=%02u] ", tag->mux, tag->sec, tag->typ);
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
void type_check(uint32_t typ) {
  if ( (typ >= DATA_TYP_MAX) || (cmap[typ].valid==0) ) {
    fprintf(stderr, "No encode function loaded for data typ=%d\n", typ);
    exit (1);
  }
}

/*
 * Create packet (serialize data and adding header)
 */
void gaps_data_encode(sdh_ha_v1 *p, size_t *p_len, uint8_t *buff_in, size_t *len_in, gaps_tag *tag) {
  size_t adu_len;
  uint32_t typ = tag->typ;
  
  /* a) serialize data into packet */
  type_check(typ);
  cmap[typ].encode (p->data, &adu_len, buff_in, len_in);
  if(xdc_verbose) data_print("API <- raw app data:", buff_in, *len_in);
  if(xdc_verbose) data_print("    -> encoded data:", p->data, adu_len);
  
  /* b) Create CLOSURE packet header */
  tag_encode(&(p->tag), tag);
  len_encode(&(p->data_len), adu_len);
  *p_len = adu_len + sizeof(p->tag) + sizeof(p->data_len);
}

/*
 * Decode data from packet
 */
void gaps_data_decode(sdh_ha_v1 *p, size_t p_len, uint8_t *buff_out, size_t *len_out, gaps_tag *tag) {
  uint32_t typ = tag->typ;

  /* a) deserialize data from packet */
  type_check(typ);
  tag_decode(tag, &(p->tag));
  len_decode(len_out, p->data_len);
  cmap[typ].decode (buff_out, len_out, p->data, &p_len);
  if(xdc_verbose) {
    data_print("API -> raw app data:", p->data,  *len_out);
    data_print("    <- encoded data:", buff_out, *len_out);
    tag_print(tag);
    fprintf(stderr, "data_len=%lu\n", *len_out);
  }
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

  if(xdc_verbose) fprintf(stderr,"API connects (s=%p t=%d) to %s\n", socket, type, dest);
  usleep(10000); // let connection establish before sending a message
  return (socket);
}

/*
 * Send ADU to HAL (which should be listening on the ZMQ subscriber socket)
 */
void xdc_asyn_send(uint8_t *adu, size_t adu_len, gaps_tag tag) {
  static int   do_once = 1;
  static void *socket;
  sdh_ha_v1    packet, *p=&packet;
  size_t       packet_len;
  
  if (do_once == 1) {    /* a) Open connection with HAL ZMQ subscriber */
    socket = z_connect(ZMQ_PUB, HAL_IPC_SUB);
    do_once = 0;
  }
  gaps_data_encode(p, &packet_len, adu, &adu_len, &tag);
  if(xdc_verbose) {
    fprintf(stderr, "API sends (on ZMQ s=%p): ", socket);
    tag_print(&tag);
    fprintf(stderr, "len=%ld ", adu_len);
    data_print("Packet", (uint8_t *) p, packet_len);
  }
  zmq_send (socket, (void *) p, packet_len, 0);
}

/*
 * Send ADU to HAL (which should be listening on the ZMQ publisher socket)
 */
void xdc_blocking_recv(uint8_t *adu, size_t *adu_len, gaps_tag *tag) {
  static int   do_once = 1;
  static void *socket;
  int          err;
  sdh_ha_v1    packet, *p=&packet;
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
    if(xdc_verbose) fprintf(stderr,"API connects (s=%p t=%d) to %s\n", socket, ZMQ_SUB, HAL_IPC_PUB);

    tag_encode(&tag4filter, tag);
    err = zmq_setsockopt (socket, ZMQ_SUBSCRIBE, (void *) &tag4filter, RX_FILTER_LEN);
    assert (err == 0);
    do_once = 0;
  }
   
  /* b) Get a packet from HAL ZMQ publisher */
  if(xdc_verbose) {
    fprintf(stderr, "API waiting to recv (using len=%d filter ", RX_FILTER_LEN);
    uint8_t *f = (uint8_t *) &tag4filter;
    for (int i=0; i < RX_FILTER_LEN; i++) fprintf(stderr, "%.02x", *(f++));
    fprintf(stderr, ")\n");
  }
  size = zmq_recv (socket, (uint8_t *) p, PACKET_MAX, 0);
  if(xdc_verbose) data_print("API recv packet", (uint8_t *) p, size);

  /* c) Decode information from packet */
  gaps_data_decode(p, size, adu, adu_len, tag);
}

/*
 * Print Codec Table
 */
void codec_maps_print(void) {
  fprintf(stderr, "%s: ", __func__);
  for (int i=0; i < DATA_TYP_MAX; i++) {
    if (cmap[i].valid != 0) {
      fprintf(stderr, "[typ=%d ", i);
      fprintf(stderr, "e=%p ", cmap[i].encode);
      fprintf(stderr, "d=%p] ", cmap[i].decode);
    }
  }
  fprintf(stderr, "\n");
}

/*
 * Load Codec Table with ADU encode and decode functions
 */
void xdc_register(codec_func_ptr encode, codec_func_ptr decode, int typ) {
  static int   do_once = 1;
  
  if (do_once == 1) {
    for (int i=0; i < DATA_TYP_MAX; i++)  cmap[i].valid=0;
    do_once = 0;
  }
  cmap[typ].valid=1;
  cmap[typ].encode=encode;
  cmap[typ].decode=decode;
  if(xdc_verbose) codec_maps_print();
}

// XXX: Additional Functions TBD
//  typ = xdc_generate(spec);  /* creates encode and decode functions and typ, then uses register to load them into the table */
// Also xdc_provision function(s)
