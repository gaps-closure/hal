/* Common headers for all HAL components */

#ifndef HAL_HEADER_FILE
#define HAL_HEADER_FILE

#include "../api/xdcomms.h"

#include <unistd.h>	
#include <sys/time.h>
#include <sys/types.h>
#include <libconfig.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include "../log/log.h"

/**********************************************************************/
/* HAL Daemon Linked List Device and Halmap Databases */
/*********t************************************************************/
// #define SN_LIST_WINDOW_SIZE 1000

/* HAL Interface parameters (linked list) */
typedef struct _dev {
  int         enabled;
  int         init_enable;
  const char *id;
  const char *path;
  const char *path_r;
  const char *path_w;
  const char *model;
  const char *comms;
  const char *addr_in;
  const char *addr_out;
  const char *mode_in;
  const char *mode_out;
  int         port_in;
  int         port_out;
  int         from_mux;
  struct sockaddr_in socaddr_in;
  struct sockaddr_in socaddr_out;
  int         readfd;
  int         writefd;
  int         count_r;
  int         count_w;
//  int         sn_list_r[SN_LIST_WINDOW_SIZE];
//  int         sn_list_w[SN_LIST_WINDOW_SIZE];
  struct _dev *next;
} device;

/* HAL Selector (used in HAL map entries and PDUs) */
typedef struct _sel {
  const char *dev;      /* points to ID of device */
  int         ctag;     /* Compressed message tag (0 = use gaps_tag ) */
  gaps_tag    tag;
} selector;

/* HAL map entry (linked list) */
typedef struct _hal {
  selector    from;
  selector    to;
  const char  *codec;
  struct _hal *next;
} halmap;

/**********************************************************************/
/* HAL Packet Storage (in PDU) */
/*********t************************************************************/
/* HAL PDU */
typedef struct _pdu {
  selector  psel;                   /* Input device and tag info */
  size_t    data_len;
  uint8_t   data[ADU_SIZE_MAX_C];   /* opaque to HAL - serialized by APP */
} pdu;

#endif
