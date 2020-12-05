/* Common headers for all HAL components */

#ifndef HAL_HEADER_FILE
#define HAL_HEADER_FILE

#include "../api/xdcomms.h"

#include <getopt.h>
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

/* HAL interface device (to the application or to the network) parameters */
typedef struct _dev {
  /* A) external parameters for this device (from HAL config file) */
  int         enabled;     /* enable (1) or disable (0) use of device  */
  int         init_enable; /* initialize device (on 'path') */
  const char *id;          /* device name (locally unique) */
  const char *path;        /* path to comms device  */
  const char *path_r;      /* path to read-only device */
  const char *path_w;      /* path to write-only device */
  const char *model;       /* packet type sent/received */
  const char *comms;       /* protocol used for communication */
  const char *addr_in;     /* address HAL listens to on this device */
  const char *addr_out;    /* address HAL connects to from this device */
  const char *mode_in;     /* HAL-ZMQ-API mode (pub, sub, req, ...)  */
  const char *mode_out;    /* HAL-ZMQ-API mode (pub, sub, req, ...)  */
  int         port_in;     /* port HAL listens to on this device */
  int         port_out;    /* port HAL connects to from this device */
  int         from_mux;    /* tag mux value for ilip device */
  /* B) internal structures and parameters for this device */
  struct sockaddr_in socaddr_in;
  struct sockaddr_in socaddr_out;
  int         readfd;       /* file descriptors to HAL-ZMQ-API process */
  int         writefd;
  int         count_r;      /* total packet counts */
  int         count_w;
  int         pid_in;       /* HAL-ZMQ-API process ids */
  int         pid_out;
  struct _dev *next;        /* Deices saved as a linked list */
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
  uint8_t   data[ADU_SIZE_MAX_C];   /* TODO_PDU_PTR */
} pdu;

#endif
