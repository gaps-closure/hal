#ifndef XDC_HEADER_FILE
#define XDC_HEADER_FILE

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <zmq.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <assert.h>
#include "log.h"

#define ADU_SIZE_MAX_C  1000000     /* 1 MB - Increased for ILIP payload mode*/
#define RX_FILTER_LEN   12
#define DATA_TYP_MAX    200
#define IPC_ADDR_DEFAULT_HALPUB "ipc:///tmp/halpub1"
#define IPC_ADDR_DEFAULT_HALSUB "ipc:///tmp/halsub1"
/* Non Legacy versions */
#define MY_DATA_TYP_MAX 200
#define MY_IPC_ADDR_DEFAULT_IN  "ipc:///tmp/halpub1"
#define MY_IPC_ADDR_DEFAULT_OUT "ipc:///tmp/halsub1"

/**********************************************************************/
/* LIB Structures */
/*********t************************************************************/
/* Closure tag structure */
typedef struct _tag {
  uint32_t    mux;      /* APP ID */
  uint32_t    sec;      /* Security tag */
  uint32_t    typ;      /* data type */
} gaps_tag;

/* CLOSURE packet (for comms between application and HAL) */
typedef struct _sdh_ha_v1 {
  gaps_tag  tag;
  uint32_t  data_len;               /* 0 = no immediate data */
  uint8_t   data[ADU_SIZE_MAX_C];   /* Immediate data */
} sdh_ha_v1;

/* Table of codec per data types (Max of DATA_TYP_MAX types) */
typedef void (*codec_func_ptr)(void *, void *, size_t *);
typedef struct _codec_map {
  int             valid;
  uint32_t        data_type;
  codec_func_ptr  encode;
  codec_func_ptr  decode;
} codec_map;
//extern codec_map  cmap[DATA_TYP_MAX];   /* declare for global use (where?) */

// 0) Minor exposed function prototypes
extern void tag_print     (gaps_tag *, FILE *);
extern void tag_write     (gaps_tag *, uint32_t,   uint32_t,   uint32_t);
extern void tag_read      (gaps_tag *, uint32_t *, uint32_t *, uint32_t *);
extern void tag_cp        (gaps_tag *tag_out, gaps_tag *tag_in);
extern void tag_encode    (gaps_tag *, gaps_tag *);
extern void tag_decode    (gaps_tag *, gaps_tag *);
extern void len_encode    (uint32_t *, size_t);
extern void len_decode    (size_t *, uint32_t);
extern void xdc_log_level (int new_level);
// 1) Configure the ADU coders and addresses
extern void xdc_register(codec_func_ptr encoder, codec_func_ptr decoder, int type);
extern char *xdc_set_in (char *addr);  /* addr = NULL returns value; else sets */
extern char *xdc_set_out(char *addr);
// 2) Initialize XDC sockets
extern void *xdc_ctx(void);
extern void *xdc_pub_socket(void);
extern void *xdc_sub_socket_non_blocking(gaps_tag tag, int timeout);
extern void *xdc_sub_socket(gaps_tag tag);
// 3) Send and recv ADUs
extern void xdc_asyn_send(void *socket, void *adu, gaps_tag *tag);
extern void xdc_blocking_recv(void *socket, void *adu, gaps_tag *tag);
extern int  xdc_recv(void *socket, void *adu, gaps_tag *tag);

/* 0m) Non-legacy Minor exposed function prototypes */
extern void my_tag_write (gaps_tag *tag, uint32_t mux, uint32_t sec, uint32_t typ);
extern void my_tag_encode(gaps_tag *tag_out, gaps_tag *tag_in);
extern void my_tag_decode (gaps_tag *tag_out, gaps_tag *tag_in);
extern void my_len_encode (uint32_t *out, size_t len);
extern void my_len_decode (size_t *out, uint32_t in);
// 1m) Configure the ADU coders and addresses
extern void my_type_check(uint32_t typ, codec_map *cmap);
extern void my_xdc_register(codec_func_ptr encode, codec_func_ptr decode, int typ, codec_map *cmap);
//extern void my_gaps_data_encode(sdh_ha_v1 *p, size_t *p_len, uint8_t *buff_in, size_t *len_out, gaps_tag *tag, codec_map *cmap);
//extern void my_gaps_data_decode(sdh_ha_v1 *p, size_t p_len, uint8_t *buff_out, size_t *len_out, gaps_tag *tag, codec_map *cmap);
// 2m) Initialize XDC sockets
extern void *my_xdc_pub_socket(void *ctx, const char *outuri);
extern void *my_xdc_sub_socket_non_blocking(gaps_tag tag, void *ctx, int timeout, const char *inuri);
extern void *my_xdc_sub_socket(gaps_tag tag, void *ctx, const char *inuri);
// 3) Send and recv ADUs
extern void my_xdc_asyn_send(void *socket, void *adu, gaps_tag *tag, codec_map *cmap);
extern void my_xdc_blocking_recv(void *socket, void *adu, gaps_tag *tag, codec_map *cmap);
extern int  my_xdc_recv(void *socket, void *adu, gaps_tag *tag, codec_map *cmap);

#endif
