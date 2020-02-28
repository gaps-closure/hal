#ifndef HAL_HEADER_FILE
#define HAL_HEADER_FILE

#include "../api/xdcomms.h"
#include "../codecs/pnt.h"

#include <unistd.h>	
#include <sys/time.h>
#include <sys/types.h>
#include <libconfig.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <errno.h>


#define PARENT_READ  read_pipe[0]
#define PARENT_WRITE write_pipe[1]
#define CHILD_WRITE  read_pipe[1]
#define CHILD_READ   write_pipe[0]

#define PKT_M1_MAX_TLVS         4
#define PKT_M1_ADU_SIZE_MAX   236
#define PKT_G1_ADU_SIZE_MAX  1000


/**********************************************************************/
/* HAL Structures */
/*********t************************************************************/
/* HAL Interface parameters (linked list) */
typedef struct _dev {
  int         enabled;
  const char *id;
  const char *path;
  const char *model;
  const char *comms;
  const char *addr_in;
  const char *addr_out;
  const char *mode_in;
  const char *mode_out;
  int         port_in;
  int         port_out;
  struct sockaddr_in socaddr_in;
  struct sockaddr_in socaddr_out;
  int         readfd;
  int         writefd;
  struct _dev *next;
} device;

/* HAL Selector (used in HAL map entries and PDUs) */
typedef struct _sel {
  const char *dev;
  gaps_tag    tag;
} selector;

/* HAL map entry (linked list) */
typedef struct _hal {
  selector    from;
  selector    to;
  const char  *codec;
  struct _hal *next;
} halmap;

/* HAL PDU */
typedef struct _pdu {
  selector  psel;                   /* Input device and tag info */
  size_t    data_len;
  uint8_t   data[ADU_SIZE_MAX_C];   /* opaque to HAL - serialized by APP */
} pdu;

/* Mercury TLV */
typedef struct _tlv_m1 {
  uint32_t  data_tag;     /* Type (e.g., DATA_PAYLOAD_1) */
  uint32_t  data_len;     /* Length (in bytes) */
  uint8_t   data[PKT_M1_ADU_SIZE_MAX];    /* Value (up to 236 (256 - 5*4) bytes of payload) */
} tlv_m1;

/* Mercury packet */
typedef struct _pkt_m1 {
  uint32_t  session_tag;           /* App Mux */
  uint32_t  message_tag;           /* Security */
  uint32_t  message_tlv_count;     /* TLV count (1 for demo) */
  tlv_m1    tlv[PKT_M1_MAX_TLVS];   /* TLV */
} pkt_m1;

/* GE Compressed Mode packet */
typedef struct _pkt_g1 {
  uint32_t  message_tag_ID;        /* App Mux */
  uint16_t  data_len;              /* Length (in bytes) */
  uint16_t  crc16;                 /* XXX: what is the generator polynomial? */
  uint8_t   data[PKT_G1_ADU_SIZE_MAX];
} pkt_g1;

#include "config.h"
#include "devices.h"
#include "packetize.h"

#endif
