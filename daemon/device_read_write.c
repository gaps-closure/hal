/*
 * HAL device read and write
 *   March 2020, Perspecta Labs
 */

/**********************************************************************/
/* HAL Library Includes and Deinitions */
/**********************************************************************/

#include "hal.h"
#include "map.h"
#include "device_open.h"
#include "packetize.h"

int sel_verbose=0;
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
/* Read device and return pdu */
/* Uses idev to determines how to parse, then extracts selector info and fill psel */
pdu *read_pdu(device *idev, int hal_verbose) {
  int             rv=-1;
  int             pkt_len=0;
  pdu            *ret = NULL;
  static uint8_t  buf[PACKET_MAX];
  int             fd;
  const char     *dev_id;
  const char     *com_type;

  /* a) read input into buf and get its length (with input dev_id and fd) */
  dev_id = idev->id;
  fd = idev->readfd;
  com_type = idev->comms;
//exit (21);
  if(hal_verbose) fprintf(stderr, "HAL reading using comms type %s\n", com_type);
  if (   (strcmp(com_type, "ipc") == 0)
      || (strcmp(com_type, "tty") == 0)
      || (strcmp(com_type, "ilp") == 0)
      || (strcmp(com_type, "tcp") == 0)
      ) {
    pkt_len = read(fd, buf, PACKET_MAX);     /* write = send for tcp with no flags */
    if (pkt_len < 0) {
      if (sel_verbose==1) printf("%s read error on fd=%d: rv=%d errno=%d ", __func__, fd, pkt_len, errno);
      if (errno == EAGAIN) {
        if (sel_verbose==1) printf("(EAGAIN: device unavaiable or read would block)\n");
        return (NULL);
      }
      printf("%s read error on fd=%d: rv=%d errno=%d ", __func__, fd, pkt_len, errno);
      printf("\n");
      exit(EXIT_FAILURE);
    }
  }
  else if (strcmp(com_type, "udp") == 0) {
    rv = recvfrom(fd, buf, PACKET_MAX, PACKET_MAX, (struct sockaddr *) &(idev->socaddr_out), (socklen_t *) &pkt_len);
    if (rv < 0) {
      printf("%s recvfrom errno code: %d\n", __func__, errno);
      exit(EXIT_FAILURE);
    }
      
  }
  else {fprintf(stderr, "%s unknown comms type %s", __func__, com_type); exit(EXIT_FAILURE);}
  
  fprintf(stderr, "HAL reads  %s from %s, fd=%02d:", idev->model, dev_id, fd); data_print("", (uint8_t *) buf, pkt_len);
  
  /* b) Write input into internal PDU */
  ret = malloc(sizeof(pdu));
  pdu_from_packet(ret, buf, pkt_len, idev);
  if(hal_verbose) {fprintf(stderr, "HAL created new "); pdu_print(ret);}
  return(ret);
}

/* Write pdu to specified fd */
void write_pdu(device *odev, selector *selector_to, pdu *p, int hal_verbose) {
  
  // old gaps_tag *otag
  int             rv=-1;
  int             fd;
  int             pkt_len=0;
  const char     *com_type;
  static uint8_t  buf[PACKET_MAX];

  if (hal_verbose) {
    fprintf(stderr, "HAL write to "); selector_print(selector_to);
    fprintf(stderr, "on"); devices_print_one(odev);
  }
  /* a) Convert into packet based on interface packet model  */
  if(hal_verbose) {fprintf(stderr, "%s: ", __func__); pdu_print(p);}
  pdu_into_packet(buf, p, &pkt_len, selector_to, odev->model);
  if(hal_verbose) {fprintf(stderr, "%s: ", __func__); data_print("Packet", buf, pkt_len);}

  /* b) Write to interface based on interface comms type */
  fd = odev->writefd;
  com_type = odev->comms;


  if(hal_verbose) fprintf(stderr, "HAL writing using comms type %s\n", com_type);
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
  else {fprintf(stderr, "%s unknown comms type %s", __func__, com_type); exit(EXIT_FAILURE);}
  
  if(hal_verbose) fprintf(stderr, "%s rv=%d\n", __func__, rv);
  fprintf(stderr, "HAL writes %s onto %s, fd=%02d:", odev->model, odev->id, fd); data_print("", buf, pkt_len);
}

/**********************************************************************/
/* HAL Device Process Chain  */
/**********************************************************************/
/* Free memory allocated for PDU */
void pdu_delete(pdu *pdu) {
  free(pdu);
}

/* Process input from device (with 'input_fd') and send to output */
int process_input(int ifd, halmap *map, device *devs, int hal_verbose) {
  pdu    *ipdu; //, *opdu;
  device *idev, *odev;
  halmap *h;
  
  idev = find_device_by_readfd(devs, ifd);
  if(idev == NULL) { 
    fprintf(stderr, "%s: Device not found for input fd\n", __func__);
    return (0);
  } 

  ipdu = read_pdu(idev, hal_verbose);
  if(ipdu == NULL) { 
//    if (sel_verbose==1) fprintf(stderr, "%s: Input PDU is NULL\n", __func__);
    return (1);
  }

  h = halmap_find(ipdu, map);
  if(h == NULL) { 
    fprintf(stderr, "%s: No matching HAL map entry for: ", __func__);
    pdu_print(ipdu);
    pdu_delete(ipdu);
    return (0);
  }
  
  odev = find_device_by_id(devs, h->to.dev);
  if(odev == NULL) { 
    fprintf(stderr, "%s: Device %s not found for output\n", __func__,  h->to.dev);
    return (0);
  }

  write_pdu(odev, &(h->to), ipdu, hal_verbose);
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
  static int  do_once=1;
  
  
  FD_ZERO(readfds);
  maxrfd = -1;
  if (do_once) fprintf(stderr, "\nHAL Waiting for input on fds");
  for(d = dev_linked_list_root; d != NULL; d = d->next) {
    if (d->enabled != 0) {
      select_add(d->readfd, &maxrfd, readfds);
      if (do_once) fprintf(stderr, ", %d", d->readfd);
    }
  }
  if (do_once) fprintf(stderr, "\n\n");
  do_once = 0;
  return (maxrfd);     /* Maximum file descriptor number for select */
}

/* Wait for input from any read interface */
void read_wait_loop(device *devs, halmap *map, int hal_verbose, int hal_wait_us) {
  int       nunready, nready;
  int       maxrfd;        /* Maximum file descriptor number for select */
  fd_set    readfds;       /* File descriptor set for select */

  while (1) {
    maxrfd = select_init(devs,  &readfds);
    nunready=0;
    if((nready = select(maxrfd, &readfds, NULL, NULL, NULL)) == -1) perror("select()");
    if(hal_verbose) fprintf(stderr, "Selected n=%d max=%d\n", nready, maxrfd);
    for (int i = 0; i < maxrfd && nready > 0; i++) {
      if (FD_ISSET(i, &readfds)) {
        nunready += process_input(i, map, devs, hal_verbose);
        nready--;
      }
    }
    if(hal_verbose) fprintf(stderr, "Selected number unready=%d\n", nunready);
    if (nunready >= 0) usleep (hal_wait_us);
  }
}
