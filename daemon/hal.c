/* TODO list (200224)
XXX: Pass ffunction pointer to API
XXX: Fix README.md and fig (show APP1, NET1,...)
XXX: Log to specified logfile, not stderr
XXX: Properly daemonize: put into hal/daemon, close standard fds, trap signals, Exit only when needed (not to debug), etc.
XXX: Dynamically change HAL daemon conifg file
XXX: frag/defrag and other functionality etc.
XXX: Codec ADU transformation (using halmap entry)
XXX: Organize code with device, HAL, and codec/parser in separate files
*/

/**********************************************************************/
/* HAL Library Includes and Deinitions */
/**********************************************************************/

#include "hal.h"
#include "config.h"
#include "devices.h"
#include "packetize.h"

int hal_verbose=0;

/**********************************************************************/
/* HAL Print (structure information) */
/*********t************************************************************/
/* print halmap selector */
void selector_print(selector *s) {
  fprintf(stderr, " %s ", s->dev);
  if (s->ctag < 0) tag_print(&(s->tag));
  else             fprintf(stderr, "[ctag=%u] ", s->ctag);
}

/* Print a information from an internal PDU */
void pdu_print(pdu *pdu) {
  if (pdu == NULL)  fprintf(stderr, "Cannot print NULL PDU\n");
  else {
    fprintf(stderr, "PDU dev=%s ", pdu->psel.dev);
    selector_print(&(pdu->psel));
    data_print("Encoded-Data", pdu->data, pdu->data_len);
  }
}

/* Print a single HAL map entry for debugging */
void halmap_print_one(halmap *hm) {
  selector_print(&(hm->from));
  fprintf(stderr, "-> ");
  selector_print(&(hm->to));
  fprintf(stderr, ", codec=%s\n", hm->codec);
}

/* Print list of HAL map entries for debugging */
void halmap_print_all(halmap *map_root) {
    fprintf(stderr, "HAL map list (%p):\n", (void *) map_root);
    for(halmap *hm = map_root; hm != NULL; hm = hm->next) {
      halmap_print_one(hm);
    }
    fprintf(stderr, "\n");
}

/**********************************************************************/
/* HAL get options */
/*********t************************************************************/
void opts_print(void) {
  printf("Hardware Abstraction Layer (HAL) for gaps CLOSURE project\n");
  printf("Usage: hal [OPTION]... CONFIG-FILE\n");
  printf("OPTION: one of the following options:\n");
  printf(" -h --help : print this message\n");
  printf(" -v --hal_verbose : print some messages in stderr\n");
  printf("CONFIG-FILE: path to file with HAL configuration information (e.g., sample.cfg)\n");
}

/* Parse the configuration file */
char *opts_get (int argc, char **argv) {
  int opt;
  char  *file_name= NULL;
  
  if (argc < 2) {
    opts_print();
    exit(EXIT_FAILURE);
  }
  while((opt =  getopt(argc, argv, "v")) != EOF)
  {
    switch (opt)
    {
      case 'v':
        hal_verbose = 1;
        break;
      default:
        printf("\nSkipping undefined Option (%d)\n", opt);
        opts_print();
    }
  }
  if(optind<argc) {
     file_name = argv[optind++];
  }
  if(hal_verbose) fprintf(stderr, "Read script options: v=%d conifg-file=%s (optind=%d)\n", hal_verbose, file_name, optind);
  return (file_name);
}

/**********************************************************************/
/* HAL MAP processing */
/*********t************************************************************/

/* Return halmap with from selector matching PDU selector from the halmap list */
halmap *halmap_find(pdu *p, halmap *map_root) {
  selector *hsel;
  selector *psel = &(p->psel);
  gaps_tag *tag  = &(psel->tag);
  int       ctag = psel->ctag;

//  fprintf(stderr, "%s", __func__); pdu_print(p);
  for(halmap *hm = map_root; hm != NULL; hm = hm->next) {
//    halmap_print_one(hm);
    if (strcmp(hm->from.dev, p->psel.dev) == 0) {
      hsel = &(hm->from);
      if (ctag < 0) {
         if ( (hsel->tag.mux == tag->mux)
           && (hsel->tag.sec == tag->sec)
           && (hsel->tag.typ == tag->typ)
             ) {
           return (hm);
         }
      }
      else
      {
        if (hsel->ctag == ctag) {
          return (hm);
        }
      }
    }
  }
  return (halmap *) NULL;
}

/* XXX: Determine codec from halmap entry, apply and return transformed pdu */
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
/* HAL Device Read and Write Functions */
/*********t************************************************************/
/* Read device and return pdu */
/* Uses idev to determines how to parse, then extracts selector info and fill psel */
pdu *read_pdu(device *idev) {
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
      || (strcmp(com_type, "tcp") == 0)
      ) {
    pkt_len = read(fd, buf, PACKET_MAX);     /* write = send for tcp with no flags */
  }
  else if (strcmp(com_type, "udp") == 0) {
    rv = recvfrom(fd, buf, PACKET_MAX, PACKET_MAX, (struct sockaddr *) &(idev->socaddr_out), (socklen_t *) &pkt_len);
    if (rv < 0) {
      printf("%s recvfrom errno code: %d\n", __func__, errno);
      exit(EXIT_FAILURE);
    }
      
  }
  else {fprintf(stderr, "%s unknown comms type %s", __func__, com_type); exit(EXIT_FAILURE);}
  if(hal_verbose) {
    fprintf(stderr, "HAL read input (len=%d rv=%d) on %s (model=%s) fd=%d", pkt_len, rv, dev_id, idev->model, fd);
    data_print("", (uint8_t *) buf, pkt_len);
  }
  
  /* b) Write input into internal PDU */
  ret = malloc(sizeof(pdu));
  pdu_from_packet(ret, buf, pkt_len, idev);
  if(hal_verbose) {fprintf(stderr, "HAL created new "); pdu_print(ret);}
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
      || (strcmp(com_type, "tcp") == 0)
     ) {
    rv = write(fd, buf, pkt_len);     /* write = send for tcp with no flags */
  }
  else if (strcmp(com_type, "udp") == 0) {
    fprintf(stderr, "XXX: Write udp mode to %s interface\n", com_type);
//    len = sizeof(cliaddr);  //len is value/resuslt
    /* (const struct sockaddr *) &cliaddr */
    rv = sendto(fd, buf, pkt_len, MSG_CONFIRM, (const struct sockaddr *) &(odev->socaddr_out), sizeof(odev->socaddr_out));
//    rv = recvfrom(fd, buf, pkt_len, MAXLINE, ( struct sockaddr *) &cliaddr, &len);
  }
  else {fprintf(stderr, "%s unknown comms type %s", __func__, com_type); exit(EXIT_FAILURE);}
  
  if(hal_verbose) fprintf(stderr, "HAL wrote on %s (fd=%d) and got rv=%d\n", odev->id, fd, rv);
}

/* Free memory allocated for PDU */
void pdu_delete(pdu *pdu) {
  free(pdu);
}

/* Process input from device (with 'input_fd') and send to output */
void process_input(int ifd, halmap *map, device *devs) {
  pdu    *ipdu; //, *opdu;
  device *idev, *odev;
  halmap *h;
  
  idev = find_device_by_readfd(devs, ifd);
  if(idev == NULL) { 
    fprintf(stderr, "%s: Device not found for input fd\n", __func__);
    return; 
  } 

  ipdu = read_pdu(idev);
  if(ipdu == NULL) { 
    fprintf(stderr, "%s: Input PDU is NULL\n", __func__);
    return; 
  }

  h = halmap_find(ipdu, map);
  if(h == NULL) { 
    fprintf(stderr, "%s: No matching HAL map entry for: ", __func__);
    pdu_print(ipdu);
    pdu_delete(ipdu);
    return; 
  }
  
  odev = find_device_by_id(devs, h->to.dev);
  if(odev == NULL) { 
    fprintf(stderr, "%s: Device %s not found for output\n", __func__,  h->to.dev);
    return; 
  }

  write_pdu(odev, &(h->to), ipdu);
  pdu_delete(ipdu);
}

/**********************************************************************/
/* HAL Top level                                                      */
/*********t************************************************************/
/* Iniitialize file descriptor set for select (from linked-list of devices) */
int select_init(device *dev_linked_list_root, fd_set *readfds) {
  device   *d;             /* Temporary device pointer */
  int       maxrfd;        /* Maximum file descriptor number for select */

  FD_ZERO(readfds);
  maxrfd = -1;
  fprintf(stderr, "HAL Waiting for input on fds");

  for(d = dev_linked_list_root; d != NULL; d = d->next) {
    if (d->enabled != 0) {
      if (d->readfd >= maxrfd) maxrfd = d->readfd + 1;
      FD_SET(d->readfd, readfds);
      if(hal_verbose) fprintf(stderr, ", %d", d->readfd);
    }
  }
  fprintf(stderr, "\n");
  return (maxrfd);     /* Maximum file descriptor number for select */
}

/* Wait for input from any read interface */
void read_wait_loop(device *devs, halmap *map) {
  int       nready;
  int       maxrfd;        /* Maximum file descriptor number for select */
  fd_set    readfds;       /* File descriptor set for select */

  while (1) {
    maxrfd = select_init(devs,  &readfds);
    if((nready = select(maxrfd, &readfds, NULL, NULL, NULL)) == -1) perror("select()");
    // fprintf(stderr, "Selected n=%d max=%d\n", nready, maxrfd);
    for (int i = 0; i < maxrfd && nready > 0; i++) {
      if (FD_ISSET(i, &readfds)) {
        process_input(i, map, devs);
        nready--;
      }
    }
  }
}

/* Get coniguration, then call read_wait_loop */
int main(int argc, char **argv) {
  config_t  cfg;           /* Configuration */
  char     *file_name;     /* Path to conifg file */
  device   *devs;          /* Linked list of enabled devices */
  halmap   *map;           /* Linked list of selector mappings */
  
  file_name = opts_get(argc, argv);
  cfg_read(&cfg, file_name);
  if(hal_verbose) fprintf(stderr, "Config file %s exists\n", file_name);
  devs = get_devices(&cfg);
  if(hal_verbose) {fprintf(stderr, "Config file "); devices_print_all(devs);}
  map  = get_mappings(&cfg);
  if(hal_verbose) {fprintf(stderr, "Config file "); halmap_print_all(map);}
  config_destroy(&cfg);
  devices_open(devs);
  devices_print_all(devs);
  halmap_print_all(map);
  read_wait_loop(devs, map);
  return 0;
}
