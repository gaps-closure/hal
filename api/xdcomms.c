/*
 * Cross Domain (XD) Communication API between Applicaitons and GAP XD Guards
 *   December 2020, Perspecta Labs
 */

#include "xdcomms.h"

codec_map  cmap[DATA_TYP_MAX];

/**********************************************************************/
/* A) Tag processing */
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
/* D) Set API Logging to a new level */
/**********************************************************************/
void xdc_log_level(int new_level) {
  static int do_once = 1;
  
  // set to default if User has not already set
  if (new_level == -1) {
    if (do_once == 1) {
      log_set_quiet(0);               /* not quiet */
      log_set_level(LOG_INFO);        /* default level */
//      log_set_level(LOG_TRACE);       /* test */
    }
    return;
  }
  if ((new_level >= LOG_TRACE) && (new_level <= LOG_FATAL)) {
    log_set_quiet(0);
    log_set_level(new_level);
    log_trace("User sets API log level to %d)", new_level);
    do_once = 0;
  }
  else {
    log_warn("Cannot change API to log level %d (min=%d max=%d)\n", __func__, new_level, LOG_TRACE, LOG_FATAL);
  }
}

/**********************************************************************/
/* B) CMAP table to store encoding and decoding function pointers */
/**********************************************************************/
/*
 * Print Codec Table entry
 */
void cmap_print_one(codec_map *cm) {
  fprintf(stderr, "[typ=%d ", cm->data_type);
  fprintf(stderr, "e=%p ",    cm->encode);
  fprintf(stderr, "d=%p] ",    cm->decode);
//  fprintf(stderr, "[valid=%d] ",   cm->valid);
}

/*
 * Print entire Codec Table
 */
void cmap_print(void) {
  codec_map  *cm;
  
  fprintf(stderr, "%s: ", __func__);
  for(cm = cmap; cm->valid != 0; cm++) cmap_print_one(cm);
  fprintf(stderr, "\n");
}

/*
 * Find Codec Table entry for a given data type
 */
codec_map *cmap_find(int data_type) {
  codec_map  *cm;
  
//  fprintf(stderr, "%s: typ=%d\n", __func__, data_type); cmap_print();
  for(cm = cmap; cm->valid != 0; cm++) {
    if (cm->data_type == data_type) return (cm);
  }
  log_warn("Could not find registered data typ = %d\n", data_type);
  return (NULL);
}

/*
 * Initialize Codec Table
 */
void cmap_init(void) {
  int         i;
  static int  do_once = 1;
  
  if (do_once == 1) {
    for (i=0; i < DATA_TYP_MAX; i++) cmap[i].valid=0;
    do_once = 0;
  }
}

/*
 * Load Codec Table with ADU encode and decode functions
 */
void xdc_register(codec_func_ptr encode, codec_func_ptr decode, int typ) {
  int i;

  xdc_log_level(-1);            /* set logging level to default (if not set) */
  cmap_init();
  for (i=0; i < DATA_TYP_MAX; i++) {
    if (cmap[i].data_type == typ) break;
    if (cmap[i].valid == 0) break;
  }
  if (i >= DATA_TYP_MAX) log_fatal("CMAP table is full (DATA_TYP_MAX=%d)\n", i);
  cmap[i].data_type = typ;
  cmap[i].valid     = 1;
  cmap[i].encode    = encode;
  cmap[i].decode    = decode;
  log_debug("API registered new data typ = %d (index=%d)", typ, i);
// cmap_print();
}

/**********************************************************************/
/* C) Encocde/decode data into/from a HAL packet */
/* TODO, Use DFDL schema */
/**********************************************************************/
/*
 * Create packet (serialize data and add header)
 */
void gaps_data_encode(sdh_ha_v1 *p, size_t *p_len, uint8_t *buff_in, size_t *buff_len, gaps_tag *tag) {
  codec_map  *cm = cmap_find(tag->typ);
  
  xdc_log_level(-1);            /* set logging level to default (if not set) */

  /* a) serialize data into packet */
  cm->encode (p->data, buff_in, buff_len);
  log_buf_trace("API <- raw app data:", buff_in, *buff_len);
  log_buf_trace("    -> encoded data:", p->data, *buff_len);

  /* b) Create CLOSURE packet header (TODO: remove NBO ha tag, len) */
  tag_cp(&(p->tag), tag);

// fprintf(stderr, "%s: mux = %d = %d\n", __func__, tag->mux, *((uint32_t *) p));

  p->data_len = (uint32_t) *buff_len;
  /* TODO: preplace last two with  sizeof(*p) - ADU_SIZE_MAX_C  */
  *p_len = (*buff_len) + sizeof(p->tag) + sizeof(p->data_len);
  // TODO - return value to indicate an error
}

/*
 * Decode data from packet
 */
void gaps_data_decode(sdh_ha_v1 *p, size_t p_len, uint8_t *buff_out, size_t *len_out, gaps_tag *tag) {
  codec_map  *cm = cmap_find(tag->typ);
  
  xdc_log_level(-1);            /* set logging level to default (if not set) */

  /* a) deserialize data from packet (TODO: remove NBO ha tag, len) */
  tag_cp(tag, &(p->tag));
  *len_out = (size_t) p->data_len;
//  fprintf(stderr, "%s\n", __func__); cmap_print();
  cm->decode (buff_out, p->data, &p_len);
  log_buf_trace("API -> raw app data:", p->data,  *len_out);
  log_buf_trace("    <- decoded data:", buff_out, *len_out);
  // TODO - return value to indicate an error
}

// XXX: Additional Functions TBD
//  typ = xdc_generate(spec);  /* creates encode and decode functions and typ, then use register to load them into the table */
// Also xdc_provision function(s)

/**********************************************************************/
/* E) Set and Get APP-HAL API Addresses */
/**********************************************************************/
/*
 * Set static address (xdc_addr) if addr_in != NULL
 *   and initialize to addr_default when do_once == 1
 */
void set_address(char *xdc_addr, char *addr_in, const char *addr_default, int *do_once) {
    if (*do_once == 1) {
      xdc_log_level(-1);            /* set logging level to default (if not set) */
      if (strlen(addr_default) >= 255) {
        log_fatal("API IPC address default is too long: %s", addr_default);
        exit(1);
      }
      *do_once = 0;
      strcpy(xdc_addr, addr_default);
    }
    if (addr_in != NULL) {
      if (strlen(addr_in) >= 255) {
        log_warn("Input too long (%d), not changing: %s", strlen(addr_in), addr_in);
      } else {
        strcpy(xdc_addr, addr_in);
        log_debug("HAL API address set to: %s", xdc_addr);
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
/* F) ZMQ-based Communication Setup */
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
    xdc_log_level(-1);            /* set logging level to default (if not set) */
    ctx = zmq_ctx_new();
    if(ctx == NULL) exit_with_zmq_error("zmq_ctx_new");
  }
  return ctx;
}

/*
 * Open ZMQ Publisher socket, connecting to HAL subscriber listening at addreess set by xdc_set_out:
 */
void *xdc_pub_socket()
{
    int      err;
    void    *socket;

    socket = zmq_socket(xdc_ctx(), ZMQ_PUB);
    if (socket == NULL) exit_with_zmq_error("zmq_socket");
    err = zmq_connect(socket, xdc_set_out(NULL));
    if (err) exit_with_zmq_error("zmq_connect");
    log_trace("API connects (s=%p t=%d) to %s", socket, ZMQ_PUB, xdc_set_out(NULL));
    /*
     * HAL subscriber binds to address (usually publisher would bind).
     * The APP-API cannot send immediately after a connect, as there
     * is a few msec before ZMQ creates outgoing pipe (so sleep 1)
     */
    sleep (1);
    return socket;
}

/*
 * Open non-blocking ZMQ Subscriber socket, with timeout specified in milliseconds
 * (HAL is a ZMQ Publisher listening on the address configured by xdc_set_in)
 */
void *xdc_sub_socket_non_blocking(gaps_tag tag, int timeout)
{
    int      err;
    void    *socket;
    gaps_tag tag4filter;

    socket = zmq_socket(xdc_ctx(), ZMQ_SUB);
    if (socket == NULL) exit_with_zmq_error("zmq_socket");
  
    if (timeout>=0) {
        err = zmq_setsockopt(socket, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
        assert(err == 0);
    }
  
    err = zmq_connect(socket, xdc_set_in(NULL));
    if (err) exit_with_zmq_error("zmq_connect");
    log_trace("API connects to %s (sock=%p timeout(ms)=%d matchLen=%d)", xdc_set_in(NULL), socket, timeout, RX_FILTER_LEN);
    tag_cp(&tag4filter, &tag);
    err = zmq_setsockopt(socket, ZMQ_SUBSCRIBE, (void *) &tag4filter, RX_FILTER_LEN);
    assert(err == 0);
    return socket;
}

/*
 * Open ZMQ Subscriber socket, connecting to HAL publisher listening at addreess set by xdc_set_in:
 */
void *xdc_sub_socket(gaps_tag tag)
{
  return (xdc_sub_socket_non_blocking(tag, -1));      /* Recv call blocks forever */
//  return (xdc_sub_socket_non_blocking(tag, 10000));       /* Receive call block for 10 seconds */
}

/**********************************************************************/
/* G) ZMQ Communication Send and Receive */
/**********************************************************************/
/*
 * Send ADU to HAL (HAL is the ZMQ subscriber) in a sdh_ha_v1 packet
 */
void xdc_asyn_send(void *socket, void *adu, gaps_tag *tag) {
  sdh_ha_v1    packet, *p=&packet;
  size_t       packet_len;
  size_t       adu_len;         /* Size of ADU is calculated by encoder */

  gaps_data_encode(p, &packet_len, adu, &adu_len, tag);
  // fprintf(stderr, "API sends (on ZMQ s=%p): ", socket);
  // tag_print(&tag, stderr);
  // fprintf(stderr, "len=%ld ", adu_len);
  log_buf_trace("API sends Packet", (uint8_t *) p, packet_len);
  int bytes = zmq_send (socket, (void *) p, packet_len, 0);
  if (bytes <= 0) log_error("RCV ERROR on ZMQ socket %d: size=%d err=%s", socket, bytes, zmq_strerror(errno));
}

/*
 * Receive ADU from HAL (HAL is the ZMQ publisher) from a sdh_ha_v1 packet
 * Returs size of packet received (timeout/error if < 0)
 */
int xdc_recv(void *socket, void *adu, gaps_tag *tag)
{
  sdh_ha_v1   packet;
  void *p =  &packet;
  size_t      adu_len;
  
  log_trace("API waiting to recv (using len=%d filter)", RX_FILTER_LEN);
  int size = zmq_recv(socket, p, sizeof(sdh_ha_v1), 0);
  if (size <= 0) {
    if (errno == EAGAIN) log_trace("Timeout on socket (%p): rv=%d error = %s", socket, size, zmq_strerror(errno));
    else                 log_error("RCV ERROR on ZMQ socket (%p): rv=%d error = %s", socket, size, zmq_strerror(errno));
  }
  else {
    log_buf_trace("API recv packet", (uint8_t *) p, size);
//fprintf(stderr, "%s: xxxxx %d\n", __func__, size);
    gaps_data_decode(p, size, adu, &adu_len, tag);
  }
  return (size);
}

/*
 * Receive ADU from HAL (HAL is the ZMQ publisher)
 * Blocks until it gets a valid adu
 */
void xdc_blocking_recv(void *socket, void *adu, gaps_tag *tag)
{
  while (xdc_recv(socket, adu, tag) < 0);
}
