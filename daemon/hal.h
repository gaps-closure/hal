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

#endif
