#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <zmq.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <assert.h>

#define ADU_SIZE_MAX   100
#define PACKET_MAX     (ADU_SIZE_MAX + 30)
#define DATA_TYP_PNT   1
#define DATA_TYP_XYZ   2
#define RX_FILTER_LEN  4
#define HAL_IPC_SUB "ipc://halsub_am"
#define HAL_IPC_PUB "ipc://halpub_am"

/**********************************************************************/
/* LIB Structures */
/*********t************************************************************/

/* Closure tag structure */
typedef struct _tag {
  uint32_t    mux;      /* APP ID */
  uint32_t    sec;      /* Security tag */
  uint32_t    typ;      /* data type */
} gaps_tag;

/* CLOSURE packet */
typedef struct _cpkt {
  gaps_tag  tag;
  uint32_t  data_len;
  uint8_t   data[ADU_SIZE_MAX];
} cpkt;

/* Data structure: PNT */
typedef struct _pnt_datatype {
    uint16_t    message_id;
    uint16_t    track_index;
    int16_t     lon;
    uint16_t    lon_frac;
    int16_t     lat;
    uint16_t    latfrac;
    int16_t     alt;
    uint16_t    altfrac;
} pnt_datatype;


extern void tag_print (gaps_tag *);
extern void data_print(const char *, uint8_t *, size_t);
extern void pnt_print (pnt_datatype *);
extern void tag_write (gaps_tag *, uint32_t,   uint32_t,   uint32_t);
extern void tag_read  (gaps_tag *, uint32_t *, uint32_t *, uint32_t *);
extern void tag_encode (gaps_tag *, gaps_tag *);
extern void tag_decode (gaps_tag *, gaps_tag *);
extern void len_encode (uint32_t *, size_t);
extern void len_decode (size_t *, uint32_t);
extern void gaps_data_encode(uint8_t *, size_t *, uint8_t *, size_t *, int);
extern void gaps_data_decode(uint8_t *, size_t *, uint8_t *, size_t *, int);
extern void gaps_asyn_send(uint8_t *, size_t,   gaps_tag);
extern void gaps_asyn_recv(uint8_t *, size_t *, gaps_tag *);
