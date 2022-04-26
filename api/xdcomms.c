/*
 * Cross Domain (XD) Communication API between Applicaitons and GAP XD Guards
 *   April 2022, Peraton Labs
 *
 * This merged API supports two versions:
 * 1) Legacy Version:
 *     Supports single-threaded receiver per Cross Doamin Function (XD-func).
 *     Socket is opened once per XD-func, minimizing protoocl delay and overhead.
 * 2) Non-Legacy Version ('my_' API functions):
 *     Supports multi-threaded receiver per XD-func.
 *     Socket is opened and closed within each thread: less efficent, but thread-safe
 *     (Orinally part of the CLOSURE RPC generator, but moved here in April 2022)
 *       a) API passes the HAL URI into PUB and SUB socket creation functions
 *       b) API passes each XD-func's codec map into the send and receive calls\
 *       c) 'codec_map' directly maps data type to its data encode + decode functions
 *          Simplifies opersation, but only supports type indexes up to DATA_TYP_MAX
 *          (may change to use LEGACY method if need arbitary indexes)
 *       d) Does not curretly support logging
 */

#include "xdcomms.h"

codec_map  cmap[DATA_TYP_MAX];    /* maps data type to its data encode + decode functions */

/**********************************************************************/
/* A) Set API Logging to a new level */
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
    log_trace("User sets API log level: %d", new_level);
    do_once = 0;
  }
  else {
    log_warn("Cannot change API to log level %d (min=%d max=%d)\n", __func__, new_level, LOG_TRACE, LOG_FATAL);
  }
}

/**********************************************************************/
/* B) Legacy Tag processing */
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
/* Bm) Non-Legacy Tag processing (all just aliases) */
/**********************************************************************/
void my_tag_write (gaps_tag *tag, uint32_t mux, uint32_t sec, uint32_t typ) {
  tag_write (tag, mux, sec, typ);
}
void my_tag_encode (gaps_tag *tag_out, gaps_tag *tag_in) {
  tag_encode (tag_out, tag_in);
}
void my_tag_decode (gaps_tag *tag_out, gaps_tag *tag_in) {
  tag_decode (tag_out, tag_in);
}
void my_len_encode (uint32_t *out, size_t len) {
  len_encode (out, len);
}
void my_len_decode (size_t *out, uint32_t in) {
  len_decode (out, in);
}

/**********************************************************************/
/* C) Legacy CMAP table to store encoding and decoding function pointers */
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
/* Cm) Non-Legacy CMAP table to store encoding and decoding function pointers */
/**********************************************************************/
/* Non-legacy tag data_type used as direct index, so must be less than MY_DATA_TYP_MAX */
void my_type_check(uint32_t typ, codec_map *cmap) {
    if ( (typ >= MY_DATA_TYP_MAX) || (cmap[typ].valid==0) ) {
        exit (1);
    }
}

/* tag data_type used as direct index (Legacy uses first available as index and stored data type) */
void my_xdc_register(codec_func_ptr encode, codec_func_ptr decode, int typ, codec_map *cmap) {
    cmap[typ].valid=1;
    cmap[typ].encode=encode;
    cmap[typ].decode=decode;
}

/**********************************************************************/
/* D) Legacy Encocde/decode data into/from a HAL packet */
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

  /* b) Create CLOSURE packet header */
  tag_encode(&(p->tag), tag);

// tag_print(tag, stderr);
// fprintf(stderr, "%s: mux = %d = %d\n", __func__, tag->mux, *((uint32_t *) p));

  len_encode(&(p->data_len), *buff_len);
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
  tag_decode(tag, &(p->tag));
  len_decode(len_out, p->data_len);
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
/* Dm) Non-Legacy Encocde/decode data into/from a HAL packet */
/**********************************************************************/
void my_gaps_data_encode(sdh_ha_v1 *p, size_t *p_len, uint8_t *buff_in, size_t *len_out, gaps_tag *tag, codec_map *cmap) {
    uint32_t typ = tag->typ;
    my_type_check(typ, cmap);
    cmap[typ].encode (p->data, buff_in, len_out);
    log_buf_trace("my API <- raw app data:", buff_in, *len_out);
    log_buf_trace("       -> encoded data:", p->data, *len_out);
    my_tag_encode(&(p->tag), tag);
    log_trace("A");
    my_len_encode(&(p->data_len), *len_out);
    log_trace("B");
    *p_len = (*len_out) + sizeof(p->tag) + sizeof(p->data_len);
    log_trace("my_gaps_data_encode: typ=%d p_len=%d", typ, p_len);
}
void my_gaps_data_decode(sdh_ha_v1 *p, size_t p_len, uint8_t *buff_out, size_t *len_out, gaps_tag *tag, codec_map *cmap) {
    uint32_t typ = tag->typ;
    my_type_check(typ, cmap);
    my_tag_decode(tag, &(p->tag));
    my_len_decode(len_out, p->data_len);
    cmap[typ].decode (buff_out, p->data, &p_len);
    }

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
/* F) Legacy ZMQ-based Communication Setup */
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
void *xdc_ctx(void) {
  static void *ctx = NULL;
  if (ctx == NULL) {
    xdc_log_level(-1);            /* set logging level to default (if not set) */
    ctx = zmq_ctx_new();
    if(ctx == NULL) exit_with_zmq_error("zmq_ctx_new");
    log_trace("API creates a new ZMQ context (p=%p)", ctx);
  }
  return ctx;
}

/*
 * Open ZMQ Publisher socket, connecting to HAL subscriber listening at addreess set by xdc_set_out:
 */
void *xdc_pub_socket(void) {
    int      err;
    void    *socket;

    socket = zmq_socket(xdc_ctx(), ZMQ_PUB);
    if (socket == NULL) exit_with_zmq_error("zmq_socket");
    err = zmq_connect(socket, xdc_set_out(NULL));
    if (err) exit_with_zmq_error("zmq_connect");
    log_trace("API connects (spck=%p t=%d) to %s", socket, ZMQ_PUB, xdc_set_out(NULL));
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
void *xdc_sub_socket_non_blocking(gaps_tag tag, int timeout) {
    int      err, len;
    void    *socket;
    gaps_tag tag4filter;
    void    *filter;

    socket = zmq_socket(xdc_ctx(), ZMQ_SUB);
    if (socket == NULL) exit_with_zmq_error("zmq_socket");
  
    if (timeout>=0) {
        err = zmq_setsockopt(socket, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
        assert(err == 0);
    }

    err = zmq_connect(socket, xdc_set_in(NULL));
    if (err) exit_with_zmq_error("zmq_connect");
    
    if ((tag.mux) != 0) {
        len = RX_FILTER_LEN;
        tag_encode(&tag4filter, &tag);
        filter = (void *) &tag4filter;
    } else {
        len = 0;
        filter = (void *) "";
    }
      
    log_trace("API connects to %s (sock=%p timeout(ms)=%d match Len=%d)", xdc_set_in(NULL), socket, timeout, len);
    
    err = zmq_setsockopt(socket, ZMQ_SUBSCRIBE, filter, len);
    assert(err == 0);
    return socket;
}

/*
 * Open ZMQ Subscriber socket, connecting to HAL publisher listening at addreess set by xdc_set_in:
 */
void *xdc_sub_socket(gaps_tag tag) {
  return (xdc_sub_socket_non_blocking(tag, -1));      /* Recv call blocks forever */
//  return (xdc_sub_socket_non_blocking(tag, 10000));       /* Receive call block for 10 seconds */
}

/**********************************************************************/
/* Fm) Non-Legacy ZMQ-based Communication Setup (URI per thread) */
/**********************************************************************/
void *my_xdc_pub_socket(void *ctx, const char *outuri) {
    int err;
    void *socket;
    socket = zmq_socket(ctx, ZMQ_PUB);
    err = zmq_connect(socket, outuri);
    assert(err == 0);
    return socket;
}

void *my_xdc_sub_socket(gaps_tag tag, void *ctx, const char *inuri) {
    int err;
    gaps_tag tag4filter;
    void *socket;
    socket = zmq_socket(ctx, ZMQ_SUB);
    err = zmq_connect(socket, inuri);
    my_tag_encode(&tag4filter, &tag);
    err = zmq_setsockopt(socket, ZMQ_SUBSCRIBE, (void *) &tag4filter, RX_FILTER_LEN);
    assert(err == 0);
    return socket;
}

void *my_xdc_sub_socket_non_blocking(gaps_tag tag, void *ctx, int timeout, const char *inuri) {
    int  err, len;
    void    *socket;
    gaps_tag tag4filter;
    void    *filter;
    socket = zmq_socket(ctx, ZMQ_SUB);
    if (timeout>=0) {
        err = zmq_setsockopt(socket, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
        assert(err == 0);
    }
    err = zmq_connect(socket, inuri);
    if ((tag.mux) != 0) {
        len = RX_FILTER_LEN;
        my_tag_encode(&tag4filter, &tag);
        filter = (void *) &tag4filter;
    } else {
        len = 0;
        filter = (void *) "";
    }
    err = zmq_setsockopt(socket, ZMQ_SUBSCRIBE, filter, len);
    assert(err == 0);
    return socket;
}

/**********************************************************************/
/* G) Legacy ZMQ Communication Send and Receive */
/**********************************************************************/
/*
 * Send ADU to HAL (HAL is the ZMQ subscriber) in a sdh_ha_v1 packet
 */
void xdc_asyn_send(void *socket, void *adu, gaps_tag *tag) {
  int          bytes;
  sdh_ha_v1    packet, *p=&packet;
  size_t       packet_len;
  size_t       adu_len;         /* Size of ADU is calculated by encoder */

  gaps_data_encode(p, &packet_len, adu, &adu_len, tag);
  log_buf_trace("API sends Packet", (uint8_t *) p, packet_len);
  // Test 1) two packets in one HAL receive buf using by sending same packet back-to-back:
  //         for (i=0; i<2; i++)
  // Test 2) one Packet split into two HAL receive bufs by sending 1/2 packet, then sleep before sending next half
  //         size_t split_1_len = packet_len / 2;
  //         size_t split_2_len = packet_len - split_1_len;
  //         bytes = zmq_send (socket, (void *) p, split_1_len, 0);
  //         sleep(1);
  //         bytes = zmq_send (socket, (void *) p, split_2_len, 0);
  //         log_buf_warn("API SPLITS Packet into two: len=%d + len=%d (%d)", split_1_len, split_2_len, bytes);

  bytes = zmq_send (socket, (void *) p, packet_len, 0);
  if (bytes <= 0) log_error("RCV ERROR on ZMQ socket %d: size=%d err=%s", socket, bytes, zmq_strerror(errno));
}

/*
 * Receive ADU from HAL (HAL is ZMQ publisher) from a sdh_ha_v1 packet
 * Returs size of packet received (timeout/error if < 0)
 */
int xdc_recv(void *socket, void *adu, gaps_tag *tag) {
  sdh_ha_v1   packet;
  void *p =  &packet;
  size_t      adu_len;
  
  log_trace("API waiting to recv packet");
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
 * Receive ADU from HAL (HAL is the ZMQ publisher) - Blocks until it gets a valid adu
 */
void xdc_blocking_recv(void *socket, void *adu, gaps_tag *tag) {
  while (xdc_recv(socket, adu, tag) < 0);
}

/**********************************************************************/
/* Gm) Non-Legacy ZMQ Communication Send and Receive */
/**********************************************************************/
void my_xdc_asyn_send(void *socket, void *adu, gaps_tag *tag, codec_map *cmap) {
    sdh_ha_v1    packet, *p=&packet;
    size_t       packet_len;
    size_t adu_len;         /* Size of ADU is calculated by encoder */
    log_trace("my API send encodes ");
    my_gaps_data_encode(p, &packet_len, adu, &adu_len, tag, cmap);
    log_buf_trace("my API sends Packet", (uint8_t *) p, packet_len);
    int bytes = zmq_send (socket, (void *) p, packet_len, 0);
    if (bytes <= 0) fprintf(stderr, "send error %s %d ", zmq_strerror(errno), bytes);
}

void my_xdc_blocking_recv(void *socket, void *adu, gaps_tag *tag, codec_map *cmap) {
    sdh_ha_v1 packet;
    void *p = &packet;
    log_trace("my API waiting to recv packet");
    int size = zmq_recv(socket, p, sizeof(sdh_ha_v1), 0);
    size_t adu_len;
    my_gaps_data_decode(p, size, adu, &adu_len, tag, cmap);
}

int my_xdc_recv(void *socket, void *adu, gaps_tag *tag, codec_map *cmap) {
    sdh_ha_v1 packet;
    void *p = &packet;
    int size = zmq_recv(socket, p, sizeof(sdh_ha_v1), 0);
    size_t adu_len;
    my_gaps_data_decode(p, size, adu, &adu_len, tag, cmap);
    return size;
}
