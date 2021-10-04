/* Common headers for all HAL components */

#ifndef HAL_HEADER_FILE
#define HAL_HEADER_FILE

#define HAVE_ARCH_STRUCT_FLOCK 

/* Control Information about one way connection (fixed for now) */
#define PAGE_LEN_MAX 4096       /* Max page length in Bytes (B) */
//#define PAGES_MAX    3          /* Number of available pages (N) */
#define PAGES_MAX    128          /* Number of available pages (N) */
//#define PAGES_MAX    256          /* Number of available pages (N) */
#include "../api/xdcomms.h"

#include <getopt.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <libconfig.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fcntl.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include "../log/log.h"
#include <zmq.h>


/**********************************************************************/
/* HAL Daemon Linked List Device and Halmap Databases */
/*********t************************************************************/
// #define SN_LIST_WINDOW_SIZE 1000

typedef uint8_t (*page)[PAGE_LEN_MAX];  /* page is message ptr (up tp PAGE_LEN_MAX bytes) */

/* pointer to shared memory block */
typedef struct _dev_shm_ptrs {
  uint32_t  *shm_r;             /* Start of Read Index */
  uint32_t  *shm_w;             /* ext Write Index, in Shared Memory N*/
  uint32_t  *shm_l;             /* Length array address (page 0) */
  gaps_tag  *tag;               /* Tag (mux, sec, typ) array address (page 0) */
  page       shm_d;             /* Message Queue address (page 0) */
} dev_shm_ptrs;

typedef struct _dev_shm_local {
  uint64_t   shm_t[PAGES_MAX];  /* Page create/delete time in ns, local to writer */
  int        shm_v[PAGES_MAX];  /* Page valid flag, local to writer) */
} dev_shm_local;

/* HAL interface device (to the application or to the network) parameters */
typedef struct _dev {
  /* A) external parameters for this device (from HAL config file) */
  int         enabled;        /* enable (1) or disable (0) use of device  */
  int         init_enable;    /* initialize device (on 'path') */
  const char *id;             /* device name (locally unique) */
  const char *path;           /* path to comms device  */
  const char *path_r;         /* path to read-only device */
  const char *path_w;         /* path to write-only device */
  const char *model;          /* packet type sent/received */
  const char *comms;          /* protocol used for communication */
  const char *addr_in;        /* address HAL listens to on this device */
  const char *addr_out;       /* address HAL connects to from this device */
  const char *mode_in;        /* HAL-ZMQ-API mode (pub, sub, req, ...)  */
  const char *mode_out;       /* HAL-ZMQ-API mode (pub, sub, req, ...)  */
  int         port_in;        /* port HAL listens to on this device */
  int         port_out;       /* port HAL connects to from this device */
  int         from_mux;       /* tag mux value for ilip device */
  int         addr_off_r;     /* SHM address offset when reading (receiver) */
  int         addr_off_w;     /* SHM address offset when writing (sender) */
  int         guard_time_aw;  /* SHM Guard time Before wrtie hold time in ns */
  int         guard_time_bw;  /* SHM Guard time After wrtie hold time in ns */
  int         shm_reset_r;    /* 1 = Reset SHM Write; 0 = do not flush old values */
  int         shm_reset_w;    /* 1 = Reset SHM Read;  0 = do not flush old values */
  int         shm_poll_time;  /* How often to check whether new message in SHM in ms */
  /* B) internal structures and parameters for this device */
  struct sockaddr_in socaddr_in;
  struct sockaddr_in socaddr_out;
  int           read_fd;      /* I/O handles = file descriptors */
  int           write_fd;
  void         *read_soc;     /* I/O handles - ZMQ sockets */
  void         *write_soc;
  int           count_r;      /* total packet counts */
  int           count_w;
  int           pid_in;       /* HAL-ZMQ-API process ids */
  int           pid_out;
  dev_shm_ptrs  block_r;      /* read block, in Shared Memory */
  dev_shm_ptrs  block_w;      /* write block, in Shared Memory */
  dev_shm_local local_w;      /* arrays to keep status (time, validity) of messages */
  struct _dev *next;          /* Deices saved as a linked list */
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
//  uint8_t   data[ADU_SIZE_MAX_C];   /* TODO_PDU_PTR */
  uint8_t   *data;                  /* TODO_PDU_PTR */
} pdu;

#endif
