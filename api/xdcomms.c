/*
 * Cross Domain (XD) Communication API between Applicaitons and GAP XD Guards
 *   March 2020, Perspecta Labs
 *
 * TODO:
 *  XXX: enclose data encode/decode API functions by passing data file description
 */

#include "xdcomms.h"

codec_map  cmap[DATA_TYP_MAX];

/**********************************************************************/
/* Tag processing (TODO, Use DFDL schema) */
/**********************************************************************/

void tag_print (gaps_tag *tag, FILE * fd) {
  fprintf(fd, "[mux=%02u sec=%02u typ=%02u] ", tag->mux, tag->sec, tag->typ);
}

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

/* copy tag_in to tag_out */
void tag_cp (gaps_tag *tag_out, gaps_tag *tag_in) {
    tag_out->mux = tag_in->mux;
    tag_out->sec = tag_in->sec;
    tag_out->typ = tag_in->typ;
}

/**********************************************************************/
/* Coding Functions for Application Data (TODO, Use DFDL schema) */
/**********************************************************************/
/*
 * Print Codec Table
 */
void xdc_maps_print(void) {
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
//  fprintf(stderr, "%s: typ=%d\n", __func__, typ);
  cmap[typ].valid=1;
  cmap[typ].encode=encode;
  cmap[typ].decode=decode;
  // xdc_maps_print();
}

// XXX: Additional Functions TBD
//  typ = xdc_generate(spec);  /* creates encode and decode functions and typ, then uses register to load them into the table */
// Also xdc_provision function(s)

void type_check(uint32_t typ) {
  if ( (typ >= DATA_TYP_MAX) || (cmap[typ].valid==0) ) {
    log_fatal("No encode function loaded for data typ=%d\n", typ);
    exit (1);
  }
}

/*
 * Create packet (serialize data and add header)
 */
void gaps_data_encode(sdh_ha_v1 *p, size_t *p_len, uint8_t *buff_in, size_t *buff_len, gaps_tag *tag) {
  uint32_t typ = tag->typ;
  
  /* a) serialize data into packet */
  type_check(typ);
  cmap[typ].encode (p->data, buff_in, buff_len);
  log_buf_trace("API <- raw app data:", buff_in, *buff_len);
  log_buf_trace("    -> encoded data:", p->data, *buff_len);

  /* b) Create CLOSURE packet header (TODO: remove NBO ha tag, len) */
  tag_cp(&(p->tag), tag);
  p->data_len = (uint32_t) *buff_len;
  /* TODO: preplace last two with  sizeof(*p) - ADU_SIZE_MAX_C  */
  *p_len = (*buff_len) + sizeof(p->tag) + sizeof(p->data_len);
}

/*
 * Decode data from packet
 */
void gaps_data_decode(sdh_ha_v1 *p, size_t p_len, uint8_t *buff_out, size_t *len_out, gaps_tag *tag) {
  uint32_t typ = tag->typ;

  /* a) deserialize data from packet (TODO: remove NBO ha tag, len) */
  type_check(typ);
  tag_cp(tag, &(p->tag));
  *len_out = (size_t) p->data_len;
  cmap[typ].decode (buff_out, p->data, &p_len);
  log_buf_trace("API -> raw app data:", p->data,  *len_out);
  log_buf_trace("    <- decoded data:", buff_out, *len_out);
}

/* TODO: new gaps_dma_payload_write/read functions to read/write payload packets */
//      0) API serializes into packet or memory (1 copy)
//      3) HAL calls sdh_v33 with packet pointting into serialized ha packet or HAL data
//         Do based on configured sdh (or if length is greater than limit?)
//  Can choose to have no encode or decode if want no copies (APP serializes)
//  ?/  Always pass poninter in ha packet ... reduce number of data copies

/**********************************************************************/
/* Set and Get APP-HAL API Addresses */
/**********************************************************************/
/* Init address to default; set if valid addr_in */
void set_address(char *xdc_addr, char *addr_in, const char *addr_default, int *do_once) {
    if (*do_once == 1) {
      if (strlen(addr_default) >= 255) {
        log_fatal("API IPC address default is too long: %s", addr_default);
        exit(1);
      }
      *do_once = 0;
      strcpy(xdc_addr, addr_default);
    }
    if (addr_in != NULL) {
      if (strlen(addr_in) >= 255) {
        log_warn("%s: Input too long, not changing", __func__);
      } else {
        strcpy(xdc_addr, addr_in);
      }
    }
}

/*
 * Set and Get IPC Addresses where APP client subscribes to HAL publisher
 * TODO: change name to xdc_addr_sub
 */
char *xdc_set_in(char *addr_in) {
  static int do_once = 1;
  static char xdc_addr[256];
  set_address(xdc_addr, addr_in, IPC_ADDR_DEFAULT_HALSUB, &do_once);
//  fprintf(stderr, "%s = %s\n", __func__, xdc_addr);
  return xdc_addr;
}

/*
 * Set and Get A IPC Addresses where APP publishes to HAL subscriber
 * TODO: change name to xdc_addr_pub
 */
char *xdc_set_out(char *addr_in) {
  static int do_once = 1;
  static char xdc_addr[256];
  set_address(xdc_addr, addr_in, IPC_ADDR_DEFAULT_HALPUB, &do_once);
// fprintf(stderr, "%s = %s\n", __func__, xdc_addr);
  return xdc_addr;
}

/**********************************************************************/
/* ZMQ-based Communication Setup */
/**********************************************************************/
/*
 * Exit with ZMQ error message
 */
void exit_with_zmq_error(const char* where) {
  log_fatal("HAL API exits after %s error %d: %s\n", where, errno, zmq_strerror(errno));
  exit(-1);
}

/*
 * Get zmq context (and create if not already created) - once per process
 */
void *xdc_ctx() {
    static void *ctx = NULL;
    if (ctx == NULL) {
        /* TODO - Pass logging requirements in as a parameter */
        log_set_quiet(0);
        log_set_level(LOG_INFO);
        
        ctx = zmq_ctx_new();
        if(ctx == NULL) exit_with_zmq_error("zmq_ctx_new");
    }
    return ctx;
}

/*
 * Open ZMQ socket and return socket handle: Publisher
 * Note:
 *   HAL subscriber binds to address (usually publisher would bind)
 *   The APP-API cannot send immediately after a connect, as there
 *   is a few msec before here is no outgoing pipe (so sleep 1)
 */
void *xdc_pub_socket()
{
    int      err;
    void    *socket;

    socket = zmq_socket(xdc_ctx(), ZMQ_PUB);
    if (socket == NULL) exit_with_zmq_error("zmq_socket");
    err = zmq_connect(socket, xdc_set_out(NULL));
    if (err) exit_with_zmq_error("zmq_connect");
    log_trace("API connects (s=%p t=%d) to %s\n", socket, ZMQ_PUB, xdc_set_out(NULL));
    sleep (1);
    return socket;
}

/*
 * Open ZMQ socket and return socket handle: Subscriber
 */
void *xdc_sub_socket(gaps_tag tag)
{
    int      err;
    void    *socket;
    gaps_tag tag4filter;

    socket = zmq_socket(xdc_ctx(), ZMQ_SUB);
    if (socket == NULL) exit_with_zmq_error("zmq_socket");
    err = zmq_connect(socket, xdc_set_in(NULL));
    if (err) exit_with_zmq_error("zmq_connect");
    log_trace("API connects (s=%p t=%d) to %s\n", socket, ZMQ_SUB, xdc_set_in(NULL));
    tag_cp(&tag4filter, &tag);
    err = zmq_setsockopt(socket, ZMQ_SUBSCRIBE, (void *) &tag4filter, RX_FILTER_LEN);
    assert(err == 0);
    return socket;
}

/**********************************************************************/
/* ZMQ Communication Send and Receive */
/**********************************************************************/
/*
 * Send ADU to HAL (which should be listening on the ZMQ subscriber socket)
 */
void xdc_asyn_send(void *socket, void *adu, gaps_tag *tag) {
  sdh_ha_v1    packet, *p=&packet;
  size_t       packet_len;
  
  size_t adu_len;         /* Size of ADU is calculated by encoder */
  gaps_data_encode(p, &packet_len, adu, &adu_len, tag);
  // fprintf(stderr, "API sends (on ZMQ s=%p): ", socket);
  // tag_print(&tag, stderr);
  // fprintf(stderr, "len=%ld ", adu_len);
  log_buf_trace("API sends Packet", (uint8_t *) p, packet_len);
  int bytes = zmq_send (socket, (void *) p, packet_len, 0);
  if (bytes <= 0) fprintf(stderr, "send error %s %d ", zmq_strerror(errno), bytes);
}

/*
 * Receive ADU from HAL (which should be listening on the ZMQ publisher socket)
 */
void xdc_blocking_recv(void *socket, void *adu, gaps_tag *tag)
{
    sdh_ha_v1 packet;
    void *p = &packet;

    log_trace("API waiting to recv (using len=%d filter)", RX_FILTER_LEN);
    int size = zmq_recv(socket, p, sizeof(sdh_ha_v1), 0);
    log_buf_trace("API recv packet", (uint8_t *) p, size);
    size_t adu_len;
    gaps_data_decode(p, size, adu, &adu_len, tag);
}
