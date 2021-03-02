/*
 * HAL device read and write (loop)
 *   January 2021, Perspecta Labs
 */

/**********************************************************************/
/* HAL Library Includes and Deinitions */
/**********************************************************************/
#include "hal.h"
#include "map.h"
#include "device_open.h"
#include "packetize.h"

#define MAX_POLL_ITEMS 16
#define DATA_ALIGNMENT 32       /* Must be power of 2 */
// PACKET MAX covers max data (ADU_SIZE_MAX_C) + max header (256), and it is multiple of DATA_ALIGNMENT
#define PACKET_MAX ((ADU_SIZE_MAX_C + 255 + DATA_ALIGNMENT) - ((ADU_SIZE_MAX_C + 255) % DATA_ALIGNMENT))
#define PACKET_BUFFERS_MAX 2    /* increasing gives (payload mode) driver more time to read data */

// #define MTHREAD     // Multithreaded version disabled until fix input buffer to use malloc?
#ifdef MTHREAD
#include <pthread.h>
typedef struct _thread_args {
  int      buf_len;
  uint8_t *buf;
  device  *idev;
  halmap  *map;
  device  *devs;
} thread_args;
#endif

int sel_verbose=0;      /* help debug of device saying it is ready when it is not */

/**********************************************************************/
/* HAL Applicaiton Data Unit (ADU) Transformation */
/**********************************************************************/
/* Transform ADU data  */
pdu *codec(halmap *h, pdu *ipdu) {
  pdu *opdu = NULL;

  if (strcmp(h->codec,"hal2be") == 0) {
    // opdu = hal2be(ipdu); 
  } 
  else if (strcmp(h->codec,"be2hal") == 0) {
    // opdu = be2hal(ipdu);
  }
  else {
    // XXX: Error message codec unknown 
    // return
  }

  // XXX: must apply appropriate transformation based on codec specified in mapentry
  // XXX: Doing null transform for now
  opdu = ipdu;  // XXX: remove once codec functions are available

  return opdu;
}

/**********************************************************************/
/* HAL Device Read and Write  */
/**********************************************************************/
void devs_stat_print(device *devs) {
  char   s[256]=""; char  str_new[64];
  
  for(device *d = devs; d != NULL; d = d->next) {
    if (d->enabled != 0) {
      sprintf(str_new, "[%s r=%d w=%d] ", d->id, d->count_r, d->count_w);
      strcat(s, str_new);
    }
  }
  log_debug("%s", s);
}

/* Read device and return buffer pointer and length */
/* Uses idev to determines how to parse, then extracts selector info and fill psel */
uint8_t *read_input_dev_into_buffer(device *idev, int *buf_len) {
  int                 fd;
  const char         *com_type, *com_model;
  struct sockaddr_in  socaddr_in;
  socklen_t           sock_len = sizeof(socaddr_in);
  
  static int          buf_index=0, buf_index_current;              /* Multiple buffers to keep data until read */
  static uint8_t      buf[PACKET_BUFFERS_MAX][PACKET_MAX]  __attribute__((aligned(DATA_ALIGNMENT)));   /* Input Packet buffers */

//  assert((PACKET_MAX%DATA_ALIGNMENT) == 0); /*
//  log_fatal("DATA_ALIGNMENT = %d, PACKET_MAX = %d, PACKET_MAX % DATA_ALIGNMENT = %d, b0=%p b1=%p", DATA_ALIGNMENT, PACKET_MAX, (PACKET_MAX % DATA_ALIGNMENT), (void *) buf[0], (void *) buf[1]);
  
  /* a) read input into buf and get its length (with input dev_id and fd) */
  fd        = idev->read_fd;
  com_type  = idev->comms;
  com_model = idev->model;
//exit (21);
  if (sel_verbose) log_trace("HAL reading using comms type %s (model=%s)", com_type, com_model);
  if (   (strcmp(com_type, "ipc") == 0)
      || (strcmp(com_type, "tty") == 0)
      || (strcmp(com_type, "ilp") == 0)
      || (strcmp(com_type, "tcp") == 0)
      ) {
      
    // Temporary HACK (NOV 2020) to give ILIP buffer of right sizes for sdh_be_v2 and sdh_be_v3
    if (strcmp(com_model, "sdh_be_v2") == 0) {
//      log_fatal("Temporary HACK to give ILIP buffer of 256 on fd=%d: buf=%p len=%d max=%d->256 err=%d\n", fd, buf, *buf_len, PACKET_MAX, errno);
      *buf_len = read(fd, buf[buf_index], 256);     /* HACK, max to exact size of of ILIP packet */
    }
    else if (strcmp(com_model, "sdh_be_v3") == 0) {
      //      *buf_len = read(fd, buf[buf_index], 512);     /* HACK to packet (256) + data (?); but not too high (e.g., 1350 fails) */
      *buf_len = read(fd, buf[buf_index], 2304);    /* v12  bigger packets (buffer big en */
      *buf_len = 256;                    /* HACK to packet (256) - actually gets 512 */
    }
    // TODOz - read from ZMQ socket instread of file descriptor
    else {
      *buf_len = read(fd, buf[buf_index], PACKET_MAX);     /* read = recv for tcp with no flags */
    }
          
    if (*buf_len < 0) {
      if (sel_verbose) log_trace("read error on fd=%d: rv=%d errno=%d", fd, *buf_len, errno);
      if (errno == EAGAIN) {
        if (sel_verbose) log_trace("EAGAIN: device unavaiable or read would block");
        return (NULL);
      }
      log_fatal("read error on fd=%d: rv=%d errno=%d", fd, *buf_len, errno);
      exit(EXIT_FAILURE);
    }
  }
  else if (strcmp(com_type, "udp") == 0) {
    *buf_len = recvfrom(fd, buf[buf_index], PACKET_MAX, PACKET_MAX, (struct sockaddr *) &socaddr_in, &sock_len);
    if (*buf_len < 0) {
      log_fatal("recvfrom errno code: %d", errno);
      exit(EXIT_FAILURE);
    }
  }
  else if (strcmp(com_type, "zmq") == 0) {
    *buf_len = zmq_recv (idev->read_soc, buf[buf_index], PACKET_MAX, 0);
    if (*buf_len < 0) {
      log_fatal("ZMQ reeive errno code: %d", errno);
      exit(EXIT_FAILURE);
    }
  }
  else {log_fatal("unknown comms type %s", com_type); exit(EXIT_FAILURE);}

  (idev->count_r)++;

  log_debug("HAL reads  (comms=%s, format=%s) from %s into buffer (ptr=%p, idx=%d): len=%d", com_type, com_model, idev->id, (void *) buf[buf_index], buf_index, *buf_len);
  log_buf_trace("Read Packet", buf[buf_index], *buf_len);
  
//log_trace("mux=%d", *((uint32_t *) buf[buf_index]));

  buf_index_current = buf_index;
  buf_index = (buf_index + 1) % PACKET_BUFFERS_MAX;
  return(buf[buf_index_current]);
}

/* Read input buffer into internal PDU (and return packet length) */
pdu *read_pdu_from_buffer(device *idev, uint8_t *buf, int buf_len, int *pkt_len) {
  pdu *pdu_ptr;
  
  pdu_ptr = malloc(sizeof(pdu));
  *pkt_len = pdu_from_packet(pdu_ptr, buf, buf_len, idev);
  if  (*pkt_len <= 0) return(NULL);
  log_trace("HAL extracted packet of len=%d from Input buf (%p) of len=%d", *pkt_len, (void *) buf, buf_len);
  log_pdu_trace(pdu_ptr, __func__);
  return(pdu_ptr);
}

/* Write pdu to specified fd */
void write_pdu(device *odev, selector *selector_to, pdu *p) {
  int             rv=-1;
  int             fd;
  int             pkt_len=0;
  const char     *com_type;
  static uint8_t  buf[PACKET_MAX];        /* Packet buffer when writing */

  // TODOz - write to ZMQ socket
  log_trace("HAL writing to %s (using buf=%p)", odev->id, (void *) buf);
  /* a) Convert into packet based on interface packet model  */
//  log_pdu_trace(p, __func__);
  pdu_into_packet(buf, p, &pkt_len, selector_to, odev->model);
//  log_buf_trace("Packet", buf, pkt_len);
  if (pkt_len == 0) return;      // do not write if bad length
    
  /* b) Write to interface based on interface comms type */
  fd = odev->write_fd;
  com_type = odev->comms;

  log_trace("HAL writing using comms type %s (len=%d buf=%p)", com_type, pkt_len, (void *)buf);
  if (   (strcmp(com_type, "ipc") == 0)
      || (strcmp(com_type, "tty") == 0)
      || (strcmp(com_type, "ilp") == 0)
      || (strcmp(com_type, "tcp") == 0)
     ) {
    rv = write(fd, buf, pkt_len);     /* write = send for tcp with no flags */
  }
  else if (strcmp(com_type, "udp") == 0) {
//    fprintf(stderr, "XXX: Write udp mode to %s interface\n", com_type);
    rv = sendto(fd, buf, pkt_len, MSG_CONFIRM, (const struct sockaddr *) &(odev->socaddr_out), sizeof(odev->socaddr_out));
  }
  else if (strcmp(com_type, "zmq") == 0) {
    rv = zmq_send (odev->write_soc, buf, pkt_len, 0);
    if (rv <= 0) log_error("RCV ERROR on ZMQ socket %d: size=%d err=%s", odev->write_soc, rv, zmq_strerror(errno));
  }
  else {
    log_fatal("%s unknown comms type %s", __func__, com_type);
    exit(EXIT_FAILURE);
  }
  (odev->count_w)++;

  (void)rv;     /* do nothing, so compiler sees rv is used if logging not enabled  */
  log_debug("HAL writes (comms=%s, format=%s) onto %s: len=%d", com_type, odev->model, odev->id, rv);
  log_buf_trace("Packet", buf, pkt_len);
}

/**********************************************************************/
/* HAL Device Process Chain  */
/**********************************************************************/
/* Free memory allocated for PDU */
void pdu_delete(pdu *pdu) {
  free((char *) pdu->psel.dev);  /* Device ID string (from strdup in 'pdu_from_packet')*/
  free(pdu);
}

/* Extract input packets from input buffer */
int route_packets(uint8_t *buf, int buf_len, device *idev, halmap *map, device *devs) {
  pdu     *ipdu;
  device  *odev;
  halmap  *h;
  int      pkt_len=0;

  if(buf_len <= 0) {
    log_trace("==================== No data from %s ====================\n", idev->id);
//    if (sel_verbose==1) fprintf(stderr, "%s: Input PDU is NULL\n", __func__);
    return (1);
  }
  
  /* Process one or more packets: input buffer -> internal PDU -> output device */
  while (buf_len > 0) {
    ipdu = read_pdu_from_buffer(idev, buf, buf_len, &pkt_len);
    if(ipdu == NULL) {
      log_trace("==================== No packet in Input Buffer from %s ====================\n", idev->id);
      return (0);
    }
    
    h = halmap_find(ipdu, map);
    if(h == NULL) {
      log_trace("==================== No matching HAL map entry from %s ====================\n", idev->id);
      log_pdu_trace(ipdu, __func__);
      pdu_delete(ipdu);
      return (0);
    }
    
    odev = find_device_by_id(devs, h->to.dev);
    if(odev == NULL) {
      log_warn("==================== Device %s not found for output ====================\n", h->to.dev);
      pdu_delete(ipdu);
      return (0);
    }

    /* Avoid race between send and receive */
    if (strcmp(idev->id, h->to.dev) == 0) {
      log_trace("%s: Loopback (%s -> %s) sleep = 50ms", __func__, idev->id, h->to.dev);
      usleep(50000);
    }
    
    write_pdu(odev, &(h->to), ipdu);
    pdu_delete(ipdu);
    buf      += pkt_len;
    buf_len  -= pkt_len;
// log_trace("length remaining in buffer after one packet removed = %d bytes", buf_len);
  }
  log_trace("==================== Successfully processed input from %s to %s =============\n", idev->id, odev->id);
  return (0);
}

#ifdef MTHREAD
/* Process input buffer in a separate thread */
void *route_packets_thread(void *vargp) {
//  printf("********* Inside route_packets thread *********\n");
  thread_args *p=vargp;
  route_packets(p->buf, p->buf_len, p->idev, p->map, p->devs);
  pthread_exit(NULL);
}

/* Create thread to route packet(s) to output */
void create_routing_thread(uint8_t *buf, int buf_len, device *idev, halmap *map, device *devs) {
  thread_args  args;
  pthread_t    thread_id;             /* structure with thread ID */
  
  args.buf     = buf;
  args.buf_len = buf_len;
  args.idev    = idev;
  args.map     = map;
  args.devs    = devs;
  pthread_create(&thread_id, NULL, route_packets_thread, (void *) &args);
}
#endif

/**********************************************************************/
/* Listen for input from any open device using unix select or zmq poll  */
/**********************************************************************/
/* TODOz - replace with ZMQ POLL */
void select_add(int fd, int *maxrfd, fd_set *readfds){
  if (fd > 0) {
    if (fd >= *maxrfd) *maxrfd = fd + 1;
    FD_SET(fd, readfds);
  }
}

/* Iniitialize file descriptor set for select (from linked-list of devices) */
int select_init(device *dev_linked_list_root, fd_set *readfds) {
  device     *d;             /* Temporary device pointer */
  int         maxrfd;        /* Maximum file descriptor number for select */
  int         i=0;
  char        s[256]="", str_new[64];
  
  FD_ZERO(readfds);
  maxrfd = -1;
  // TODOz - search for ZMQ sockets
  for(d = dev_linked_list_root; d != NULL; d = d->next) {
    if ((d->enabled != 0) && (d->read_fd >= 0)) {
      select_add(d->read_fd, &maxrfd, readfds);
      sprintf(str_new, "%s(fd=%d) ", d->id, d->read_fd);
      strcat(s, str_new);
      i++;
    }
  }
  log_debug("==================== HAL Waiting for first input from %d device(s): %s ====================\n", i, s);
  return (maxrfd);     /* Maximum file descriptor number for select */
}

/* Wait for input from any read interface */
void read_wait_loop2(device *devs, halmap *map, int hal_wait_us) {
  int       nunready, nready;
  int       maxrfd;                   /* Maximum file descriptor number for select */
  fd_set    readfds, readfds_saved;   /* File descriptor set for select */
  device   *idev;
  uint8_t  *buf;
  int       buf_len;

  maxrfd = select_init(devs,  &readfds_saved);

  while (1) {
//    maxrfd = select_init(devs,  &readfds);
    readfds = readfds_saved;
    nunready=0;
    if((nready = select(maxrfd, &readfds, NULL, NULL, NULL)) == -1) perror("select()");
    if (nready < 0) {
      log_error("Select error rv=%d errno=%d\n", nready, errno);
      if (errno == EINTR) log_error("EINTR: A signal was caught");
      // fprintf(stderr, "Other error codes: %d %d %d", ENOMEM, EINVAL, EBADF);
      // EBADF invalid file descriptor in the set. (closed, or errored)
      // EINVAL  nfds is negative or the value contained withintimeout is invalid.
      // ENOMEM  unable to allocate memory for internal tables
      // exit(EXIT_FAILURE);
    }
    
    if (sel_verbose) log_trace("Select found n=%d ready (max=%d)\n", nready, maxrfd);
    for (int i = 0; i < maxrfd && nready > 0; i++) {
      if (FD_ISSET(i, &readfds)) {
        idev = find_device_by_read_fd(devs, i);
        if (idev == NULL)      log_warn("Device not found for input\n");
        else {
          buf = read_input_dev_into_buffer(idev, &buf_len);
          nunready += route_packets(buf, buf_len, idev, map, devs);
        }
        nready--;
      }
    }
    if (nunready >= 0) {
      if (sel_verbose) log_trace("%d devices were not ready atter being selected (hal_wait_us=%d)\n", nunready, hal_wait_us);
      if (hal_wait_us < 0) exit(EXIT_FAILURE);
      else                 usleep (hal_wait_us);
    }
  }
}

int zmq_poll_init(device *dev_linked_list_root, zmq_pollitem_t *items, int *num_zmq_items) {
  int      i=0;
  char     s[256]="", str_new[64];
  device  *d;                   /*  device pointer */

  /* First load ØMQ sockets into item */
  for(d = dev_linked_list_root; d != NULL; d = d->next) {
    if ((d->enabled != 0) && (d->read_soc != NULL)) {
      sprintf(str_new, "%s(soc=%p) ", d->id, d->read_soc);
      strcat(s, str_new);
      items[i].socket = d->read_soc;
      items[i].events = ZMQ_POLLIN;
      i++;
    }
  }
  *num_zmq_items = i;
  /* Second get standard unix socket 'fd' into item*/
  for(d = dev_linked_list_root; d != NULL; d = d->next) {
    if ((d->enabled != 0) && (d->read_fd >= 0)) {
      sprintf(str_new, "%s(fd=%d) ", d->id, d->read_fd);
      strcat(s, str_new);
      items[i].socket = NULL;
      items[i].fd     = d->read_fd;
      items[i].events = ZMQ_POLLIN;
      i++;
    }
  }
  log_debug("========== HAL Waiting for first input from %d ZMQ and %d Unix device(s): %s\n", *num_zmq_items, i-(*num_zmq_items), s);
  return (i);
}

/* Wait for input from any read interface */
void read_wait_loop(device *devs, halmap *map, int hal_wait_us) {
  int             num_items, num_zmq_items, i, rc;
  zmq_pollitem_t  items[MAX_POLL_ITEMS];
  device         *idev;
  uint8_t        *buf;
  int             buf_len;

  num_items = zmq_poll_init(devs, items, &num_zmq_items);
  while (1) {
    rc = zmq_poll(items, num_items, -1);    /* Poll for events indefinitely (-1) */
    log_trace("Found %d (of %d) devices ready to be read", rc, num_items);
    if (rc < 0) log_error("Poll error rc=%d errno=%d\n", rc, errno);
    for (i=0; i<num_items; i++) {
//      log_trace("Checking device %d", i);
      if (items[i].revents & ZMQ_POLLIN) {  /* Any returned events (stored in items[].revents)? */
        if (i < num_zmq_items) idev = find_device_by_read_soc(devs, items[i].socket);
        else                   idev = find_device_by_read_fd (devs, items[i].fd);
        if (idev == NULL)      log_warn("Device not found for input\n");
        else {
          buf = read_input_dev_into_buffer(idev, &buf_len);
#ifdef MTHREAD
          create_routing_thread(buf, buf_len, idev, map, devs);
#else
          route_packets(buf, buf_len, idev, map, devs);
#endif
        }
      }
    }
  }
}

