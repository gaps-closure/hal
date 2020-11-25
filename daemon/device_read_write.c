/*
 * HAL device read and write (loop)
 *   April 2020, Perspecta Labs
 */

/**********************************************************************/
/* HAL Library Includes and Deinitions */
/**********************************************************************/

#include "hal.h"
#include "map.h"
#include "device_open.h"
#include "packetize.h"

int sel_verbose=1;
/**********************************************************************/
/* HAL Applicaiton Data Unit (ADU) Transformation */
/**********************************************************************/
/* Transfform ADU data  */
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

/* Read device and return pdu */
/* Uses idev to determines how to parse, then extracts selector info and fill psel */
pdu *read_pdu(device *idev) {
  int                 pkt_len=0, valid;
  pdu                *ret = NULL;
  static uint8_t      buf[PACKET_MAX];
  int                 fd;
  const char         *com_type, *com_model;
  struct sockaddr_in  socaddr_in;
  socklen_t           sock_len = sizeof(socaddr_in);

  /* a) read input into buf and get its length (with input dev_id and fd) */
  fd        = idev->readfd;
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
//      log_fatal("Temporary HACK to give ILIP buffer of 256 on fd=%d: buf=%p len=%d max=%d->256 err=%d\n", fd, buf, pkt_len, PACKET_MAX, errno);
      pkt_len = read(fd, buf, 256);     /* HACK, max to exact size of of ILIP packet */
    }
    else if (strcmp(com_model, "sdh_be_v3") == 0) {
      pkt_len = read(fd, buf, 512);     /* HACK to packet (256) + data (?); but not too high (e.g., 1350 fails) */
    }
    else {
      pkt_len = read(fd, buf, PACKET_MAX);     /* read = recv for tcp with no flags */
    }
          
    if (pkt_len < 0) {
      if (sel_verbose) log_trace("read error on fd=%d: rv=%d errno=%d", fd, pkt_len, errno);
      if (errno == EAGAIN) {
        if (sel_verbose) log_trace("EAGAIN: device unavaiable or read would block");
        return (NULL);
      }
      log_fatal("read error on fd=%d: rv=%d errno=%d", fd, pkt_len, errno);
      exit(EXIT_FAILURE);
    }
  }
  else if (strcmp(com_type, "udp") == 0) {
    pkt_len = recvfrom(fd, buf, PACKET_MAX, PACKET_MAX, (struct sockaddr *) &socaddr_in, &sock_len);
    if (pkt_len < 0) {
      log_fatal("recvfrom errno code: %d", errno);
      exit(EXIT_FAILURE);
    }
  }
  else {log_fatal("unknown comms type %s", com_type); exit(EXIT_FAILURE);}

  (idev->count_r)++;

  log_debug("HAL reads %s from %s, fd=%02d:", idev->model, idev->id, fd);
  log_buf_trace("Packet", buf, pkt_len);

  /* b) Write input into internal PDU */
  ret = malloc(sizeof(pdu));
  valid = pdu_from_packet(ret, buf, pkt_len, idev);
  if  (valid == 0) return(NULL);
    
  log_trace("HAL created new PDU");
  log_pdu_trace(ret, __func__);
  return(ret);
}

/* Write pdu to specified fd */
void write_pdu(device *odev, selector *selector_to, pdu *p) {
  
  // old gaps_tag *otag
  int             rv=-1;
  int             fd;
  int             pkt_len=0;
  const char     *com_type;
  static uint8_t  buf[PACKET_MAX];

  log_trace("HAL writting to %s on fd=%d\n", odev->id, odev->writefd);
  /* a) Convert into packet based on interface packet model  */
  log_pdu_trace(p, __func__);
  pdu_into_packet(buf, p, &pkt_len, selector_to, odev->model);
  log_buf_trace("Packet", buf, pkt_len);
  if (pkt_len == 0) return;      // do not write if bad length
    
  /* b) Write to interface based on interface comms type */
  fd = odev->writefd;
  com_type = odev->comms;

  log_trace("HAL writing using comms type %s", com_type);
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
  else {
    log_fatal("%s unknown comms type %s", __func__, com_type);
    exit(EXIT_FAILURE);
  }
  (odev->count_w)++;

  (void)rv;     /* do nothing, so compiler sees rv is used if logging not enabled  */
  log_debug("HAL writes %s onto %s (fd=%02d) rv=%d", odev->model, odev->id, fd, rv);
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

/* Process input from device (with 'input_fd') and send to output */
int process_input(int ifd, halmap *map, device *devs) {
  pdu    *ipdu; //, *opdu;
  device *idev, *odev;
  halmap *h;
  
  idev = find_device_by_readfd(devs, ifd);
  if(idev == NULL) { 
    log_warn("%s: Device not found for input fd\n", __func__);
    return (0);
  } 

  ipdu = read_pdu(idev);
  if(ipdu == NULL) { 
//    if (sel_verbose==1) fprintf(stderr, "%s: Input PDU is NULL\n", __func__);
    return (1);
  }

  h = halmap_find(ipdu, map);
  if(h == NULL) { 
    log_warn("%s: No matching HAL map entry for: ", __func__);
    log_pdu_trace(ipdu, __func__);
    pdu_delete(ipdu);
    return (0);
  }
  
  odev = find_device_by_id(devs, h->to.dev);
  if(odev == NULL) { 
    log_warn("%s: Device %s not found for output\n", __func__,  h->to.dev);
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
  return (0);
}

/**********************************************************************/
/* Listen for input from any open device                              */
/**********************************************************************/
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
  for(d = dev_linked_list_root; d != NULL; d = d->next) {
    if ((d->enabled != 0) && (d->readfd >= 0)) {
      select_add(d->readfd, &maxrfd, readfds);
      sprintf(str_new, "%s(fd=%d) ", d->id, d->readfd);
      strcat(s, str_new);
      i++;
    }
  }
  log_debug("HAL Waiting for first input from %d device(s): %s", i, s);
  return (maxrfd);     /* Maximum file descriptor number for select */
}

/* Wait for input from any read interface */
void read_wait_loop(device *devs, halmap *map, int hal_wait_us) {
  int       nunready, nready;
  int       maxrfd;                   /* Maximum file descriptor number for select */
  fd_set    readfds, readfds_saved;   /* File descriptor set for select */

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
        nunready += process_input(i, map, devs);
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
