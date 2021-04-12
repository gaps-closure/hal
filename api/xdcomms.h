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
#define VERSION_LOCAL   0
#define VERSION_REMOTE  0x72ca

#define IPC_ADDR_DEFAULT_HALPUB "ipc:///tmp/halpub1"
#define IPC_ADDR_DEFAULT_HALSUB "ipc:///tmp/halsub1"
/**********************************************************************/
/* LIB Structures */
/*********t************************************************************/
/* Closure tag structure */
typedef struct _tag {
  uint32_t    mux;      /* APP ID */
  uint32_t    sec;      /* Security tag */
  uint32_t    typ;      /* data type */
} gaps_tag;

/*
 * CLOSURE packet for local comms between application and HAL
 * Also between HAL's with a p2p link: e.g., emulator
 * Only the latter uses framing (crc, delim) and hton/ntoh conversions
 */
typedef struct _sdh_ha_v1 {
  gaps_tag  tag;                   /* must come first, so API can select its packets */
  uint16_t  version;               /* VERSION_LOCAL = local API; VERSION_REMOTE = remote API */
  uint16_t  crc16;                 /* CRC frame check */
  uint32_t  data_len;              /* Data length */
  uint8_t   data[ADU_SIZE_MAX_C];  /* Data (up to ADU_SIZE_MAX_C bytes) */
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

/* Minor exposed function prototypes */
extern void tag_print     (gaps_tag *, FILE *);
extern void tag_write     (gaps_tag *, uint32_t,   uint32_t,   uint32_t);
extern void tag_read      (gaps_tag *, uint32_t *, uint32_t *, uint32_t *);
extern void tag_cp        (gaps_tag *tag_out, gaps_tag *tag_in);
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

#endif
