/* TODO list (200224)
XXX: udp interface
XXX: Create HAL.h
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

#include "api/closure.h"

#include <unistd.h>	
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libconfig.h>
#include <arpa/inet.h>
#include <errno.h>

#define PARENT_READ  read_pipe[0]
#define PARENT_WRITE write_pipe[1]
#define CHILD_WRITE  read_pipe[1]
#define CHILD_READ   write_pipe[0]

#define PKT_M1_MAX_TLVS         4
#define PKT_M1_ADU_SIZE_MAX   236
#define PKT_G1_ADU_SIZE_MAX  1000

int hal_verbose=0;

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

/**********************************************************************/
/* HAL Print (structure information) */
/*********t************************************************************/
/* Print a information from an internal PDU */
void pdu_print(pdu *pdu) {
  if (pdu == NULL)  fprintf(stderr, "Cannot print NULL PDU\n");
  else {
    fprintf(stderr, "PDU dev=%s ", pdu->psel.dev);
    tag_print(&(pdu->psel.tag));
    data_print("Encoded-Data", pdu->data, pdu->data_len);
  }
}

void devices_print_one(device *d)  {
  fprintf(stderr, " %s [v=%d p=%s m=%s c=%s", d->id, d->enabled, d->path,  d->model, d->comms);
  if (strlen(d->addr_in)  > 0) fprintf(stderr, " ai=%s", d->addr_in);
  if (strlen(d->addr_out) > 0) fprintf(stderr, " ao=%s", d->addr_out);
  if (strlen(d->mode_in)  > 0) fprintf(stderr, " mi=%s", d->mode_in);
  if (strlen(d->mode_out) > 0) fprintf(stderr, " mo=%s", d->mode_out);
  if (d->port_in          > 0) fprintf(stderr, " pi=%d", d->port_in);
  if (d->port_out         > 0) fprintf(stderr, " po=%d", d->port_out);
  if (d->readfd          >= 0) fprintf(stderr, " r=%d",  d->readfd);
  if (d->writefd         >= 0) fprintf(stderr, " w=%d",  d->writefd);
  fprintf(stderr, "]\n");
}
  
/* Print list of devices for debugging */
void devices_print_all(device *root)  {
  fprintf(stderr, "HAL device list (%p):\n", (void *) root);
  for(device *d = root; d != NULL; d = d->next) devices_print_one(d);
}

/* Print a single HAL map entry for debugging */
void halmap_print_one(halmap *hm) {
  fprintf(stderr, " %s ", hm->from.dev); tag_print(&(hm->from.tag));
  fprintf(stderr, "-> %s ", hm->to.dev); tag_print(&(hm->to.tag));
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

/* Print M1 Packet */
void m1_print(pkt_m1 *p) {
  tlv_m1 *tlv;
  
  fprintf(stderr, "%s: ", __func__);
  fprintf(stderr, "mux=%u ",  ntohl(p->session_tag));
  fprintf(stderr, "sec=%u ",  ntohl(p->message_tag));
  for (int i=0; i < ntohl(p->message_tlv_count); i++) {
    tlv = &(p->tlv[i]);
    fprintf(stderr, "[ty=%u ", ntohl(tlv->data_tag));
    data_print("Data", tlv->data, ntohl(tlv->data_len));
    fprintf(stderr, "]");
  }
  fprintf(stderr, "\n");
}

/* Print G1 Packet */
void g1_print(pkt_g1 *p) {
  fprintf(stderr, "%s: ", __func__);
  fprintf(stderr, "mux=%u ", ntohl(p->message_tag_ID));
  fprintf(stderr, "crc=%02x ", ntohs(p->crc16));
  data_print("Data", p->data, ntohs(p->data_len));
  fprintf(stderr, "\n");
}

/**********************************************************************/
/* HAL Configuration file (read and parse) */
/*********t************************************************************/
/* Read conifg file */
void cfg_read (config_t *cfg, char  *file_name) {
  config_init(cfg);
  if(! config_read_file(cfg, file_name))
  {
    fprintf(stderr, "%s:%d - %s\n", config_error_file(cfg), config_error_line(cfg), config_error_text(cfg));
    config_destroy(cfg);
    exit(EXIT_FAILURE);
  }
  if(hal_verbose) fprintf(stderr, "Config file %s exists\n", file_name);
}

/* Get a top-level string item from config */
char *cfg_get_top_str(config_t *cfg, char *fld) {
  const char *ret;
  if(!config_lookup_string(cfg, fld, &ret)) {
    fprintf(stderr, "No '%s' setting in configuration file.\n", fld);
    exit(EXIT_FAILURE);
  }
  return strdup(ret);
}

/* Get field parameter from config (exit if param is not found and non-optional) */
const char * get_param_str(config_setting_t *dev, const char *name, int optional, int field_num) {
  const char *val="";         /* not use NULL to avoid lookup failure */

//  fprintf(stderr, "%s %s\n", __func__, name);
  if( (!config_setting_lookup_string(dev, name, &val)) && (!optional) ) {
//    fprintf(stderr, "BBB\n");
    fprintf(stderr, "Missing a non-optional field '%s' (for device %d)\n", name, field_num);
    exit(EXIT_FAILURE);
  }
  return (strdup(val));
}

/* Get field parameter from config (exit if param is not found and non-optional) */
int get_param_int(config_setting_t *dev, const char *name, int optional, int field_num) {
  int  val = -1;
  
  if ( (!config_setting_lookup_int(dev, name, &val)) && (!optional) ) {
    fprintf(stderr, "Missing a non-optional field '%s' (for device %d)\n", name, field_num);
    exit(EXIT_FAILURE);
  }
  return (val);
}

/* Construct linked list of devices from config */
device *get_devices(config_t *cfg) {
  device *ret = NULL;
  config_setting_t *devs = config_lookup(cfg, "devices");
  if(devs != NULL) {
    int count = config_setting_length(devs);
    ret = malloc(count * sizeof(device));
    if (ret == NULL) {
      fprintf(stderr, "Memory allocation failed");
      exit(EXIT_FAILURE);
    }
    for(int i = 0; i < count; i++) {
      config_setting_t *dev = config_setting_get_elem(devs, i);
      /* required parameters */
      ret[i].enabled  = get_param_int(dev, "enabled",  0, i);
      ret[i].id       = get_param_str(dev, "id",       0, i);
      ret[i].path     = get_param_str(dev, "path",     0, i);
      ret[i].model    = get_param_str(dev, "model",    0, i);
      ret[i].comms    = get_param_str(dev, "comms",    0, i);
      /* optional parameters */
      ret[i].addr_in  = get_param_str(dev, "addr_in",  1, i);
      ret[i].addr_out = get_param_str(dev, "addr_out", 1, i);
      ret[i].mode_in  = get_param_str(dev, "mode_in",  1, i);
      ret[i].mode_out = get_param_str(dev, "mode_out", 1, i);
      ret[i].port_in  = get_param_int(dev, "port_in",  1, i);
      ret[i].port_out = get_param_int(dev, "port_out", 1, i);
      ret[i].readfd   = -1; /* to be set when opened */
      ret[i].writefd  = -1; /* to be set when opened */
      ret[i].next     = i < count - 1 ? &ret[i+1] : (device *) NULL;
//      if(hal_verbose) fprintf(stderr, "i=%d Read Config file %s %p\n", i, ret[i].id, ret[i].next);
    }
  }
  if(hal_verbose) {fprintf(stderr, "Config file "); devices_print_all(ret);}
  return ret;
}

/* Construct linked list of HAL map entries from config */
halmap *get_mappings(config_t *cfg) {
  halmap *ret = NULL;

  config_setting_t *hmaps = config_lookup(cfg, "maps");
  if(hmaps != NULL) {
    int count = config_setting_length(hmaps);
    ret = malloc(count * sizeof(halmap));
    if (ret == NULL) {
      fprintf(stderr, "Memory allocation failed");
      exit(EXIT_FAILURE);
    }
    for(int i = 0; i < count; i++) {
      config_setting_t *map = config_setting_get_elem(hmaps, i);
      ret[i].from.dev     = get_param_str(map, "from_dev", 0, i);
      ret[i].to.dev       = get_param_str(map, "to_dev",   0, i);
      ret[i].from.tag.mux = get_param_int(map, "from_mux", 0, i);
      ret[i].from.tag.sec = get_param_int(map, "from_sec", 0, i);
      ret[i].from.tag.typ = get_param_int(map, "from_typ", 0, i);
      ret[i].to.tag.mux   = get_param_int(map, "to_mux",   0, i);
      ret[i].to.tag.sec   = get_param_int(map, "to_sec",   0, i);
      ret[i].to.tag.typ   = get_param_int(map, "to_typ",   0, i);
      ret[i].codec        = get_param_str(map, "codec",    1, i);
      ret[i].next         = i < count - 1 ? &ret[i+1] : (halmap *) NULL;
    }
  }
  if(hal_verbose) {fprintf(stderr, "Config file "); halmap_print_all(ret);}
  return ret;
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
/* HAL Device Processing */
/*********t************************************************************/

/* Loop through devs linked list, return device with fd as its read file descriptor */
device *find_device_by_readfd(device *root, int fd) {
  for(device *d = root; d != NULL; d = d->next) {
    // fprintf(stderr, "%s: fd=%d: dev=%s rfd=%d wfd=%d\n", __func__, fd, d->id, d->readfd, d->writefd);
    if ( (d->enabled != 0) && (d->readfd == fd) ) return (d);
  }
  return ((device *) NULL);
}

/* Loop through devs linked list, find and return entry matching "id" */
device *find_device_by_id(device *root, const char *id) {
  for(device *d = root; d != NULL; d = d->next) {
    // fprintf(stderr, "%s: list=%s find=%s\n", __func__, d->id, id);
    if ( (d->enabled != 0) && (strcmp(d->id, id) == 0) )  return d;
  }
  return ((device *) NULL);
}

/* Open HAL child process to read or write using the Application API (using zcat) */
void start_api_process(device *d, int *read_pipe, int *write_pipe, int pipe_open, int pipe_close, int fd_open, const char *mode, const char *addr) {
  int  pid;

  if ((pid = fork()) < 0) {
    fprintf(stderr, "Fork failed\n");
    exit(EXIT_FAILURE);
  } else if (pid == 0) {  /* child (API) process starts here */
    close(PARENT_READ);
    close(PARENT_WRITE);
    close(pipe_close);
    dup2(pipe_open, fd_open);        /* copy fd_open file descriptor into pipe_open file descriptor */
    char *argv2[] = {(char *) d->path, "-b", (char *) mode, (char *) addr, NULL};   /* unix command */
    if(execvp(argv2[0], argv2) < 0) perror("execvp()");
    exit(EXIT_FAILURE);                                      /* child process should not reach here */
  } else {                /* parent (HAL) process */
    if(hal_verbose) fprintf(stderr, "Spawned %s subscriber pid=%d\n", d->path, pid);
  }
}

/* Open IPC interfaces (specified in d) */
void interface_open_ipc(device *d) {
  int  read_pipe[2];    /* Read  pipes for parent-child (HAL-API) communication */
  int  write_pipe[2];   /* Write pipes for parent-child (HAL-API) communication */

  /* a) Open communication pipes for IPC reading and writing */
  fprintf(stderr, "Open IPC %s %s\n", d->id, d->path);
  if(pipe(read_pipe) < 0 || pipe(write_pipe) < 0) {
    fprintf(stderr, "Pipe creation failed\n");
    exit(EXIT_FAILURE);
  }
  if(hal_verbose) fprintf(stderr, "Pipe FDs: hal_r=%d hal_w=%d zc_sub_w=%d zc_pub_r=%d\n", PARENT_READ, PARENT_WRITE,  CHILD_WRITE, CHILD_READ);
  
  /* b) Fork HAL child processes (to read and write on ipc channels) */
  start_api_process(d, read_pipe, write_pipe, CHILD_WRITE, CHILD_READ,  STDOUT_FILENO, d->mode_in,  d->addr_in);
  start_api_process(d, read_pipe, write_pipe, CHILD_READ,  CHILD_WRITE, STDIN_FILENO,  d->mode_out, d->addr_out);

  /* c) Parent (HAL) process finishes up */
  close(CHILD_READ);
  close(CHILD_WRITE);
  d->readfd  = PARENT_READ;
  d->writefd = PARENT_WRITE;
}

/* Open a network socket and return its fd. Optionally bind (bind_flag=1) and connect (bind_flag=0 & tcp) socket */
int interface_open_inet_one(device *d, const char *addr, int port, struct sockaddr_in *serv_addr, int bind_flag) {
  int fd, comm_type;
  
  /* a) Copy IP destination information into sockaddr_in struture */
  serv_addr->sin_family = AF_INET;
  serv_addr->sin_port = htons(port);
  if(inet_pton(AF_INET, addr, &(serv_addr->sin_addr))<=0)
  {
    printf("\nInvalid address/ Address not supported for %s %s\n", d->id, d->path);
    exit(EXIT_FAILURE);
  }
  
  /* b) Create socket */
  if ( strcmp(d->comms, "udp") == 0) comm_type = SOCK_DGRAM;
  else                               comm_type = SOCK_STREAM;
  if ((fd = socket(AF_INET, comm_type, 0)) < 0)
  {
    printf("\nSocket creation error for %s\n", d->id);
    exit(EXIT_FAILURE);
  }
  if(hal_verbose) fprintf(stderr, "Created new TCP socket (fd=%d ty=%d).......", fd, comm_type);

  if (bind_flag == 1) {
    // c1) Bind the socket with the HAL server address
    if ( bind(fd, (const struct sockaddr *) serv_addr, sizeof(*serv_addr)) < 0 )
    {
      perror("\nbind failed");
      exit(EXIT_FAILURE);
    }
  }
  else {
    /* c2) Connect the socket (if not UDP) */
    if ( strcmp(d->comms, "udp") != 0) {
      if (connect(fd, (struct sockaddr *) serv_addr, sizeof(*serv_addr)) < 0)
      {
        printf("\nConnection Failed for %s %s\n", d->id, d->path);
        exit(EXIT_FAILURE);
      }
    }
  }
  return(fd);
}
    
/* Open a network socket for read-write and store the fds (in the device structure) */
void interface_open_inet(device *d) {
  int fd_out = -1, fd_in = -1;
  
  /* Create socket and connect if required */
  if (strlen(d->addr_out) > 0) fd_out = interface_open_inet_one(d, d->addr_out, d->port_out, &(d->socaddr_out), 0);
  if (strlen(d->addr_in)  > 0) fd_in  = interface_open_inet_one(d, d->addr_in,  d->port_in,  &(d->socaddr_in),  1);

  /* Save file descriptors */
  if ( (fd_out == -1) && (fd_in == -1) ) {fprintf(stderr, "\n%s: No address speciffied for interface %s\n", __func__, d->id); exit(EXIT_FAILURE);}
  if ( (fd_out == -1) && (fd_in != -1) ) {d->readfd = fd_in;  d->writefd = fd_in;}
  if ( (fd_out != -1) && (fd_in == -1) ) {d->readfd = fd_out; d->writefd = fd_out;}
  if ( (fd_out != -1) && (fd_in != -1) ) {d->readfd = fd_in;  d->writefd = fd_out;}
}

/* Open a serial (tty) interface for read-write and return its fd */
void interface_open_tty(device *d) {
  int fd;
  if ((fd = open(d->path, O_RDWR)) < 0) {
    fprintf(stderr, "Error opening device: %s %s\n", d->id, d->path);
    exit(EXIT_FAILURE);
  }
  d->readfd = fd;
  d->writefd = fd;
}

/* Open enabled devices (from linked-list of devices) and get their in/out handles */
void devices_open(device *dev_linked_list_root) {
  for(device *d = dev_linked_list_root; d != NULL; d = d->next) {
    if (d->enabled == 0) continue;
    if(hal_verbose) fprintf(stderr, "About to open device: %s %s test=%d\n", d->id, d->path, strncmp(d->path, "/dev/", 5));
    if        (!strncmp(d->comms, "tty", 3))                                      interface_open_tty(d);
    else if ( (!strncmp(d->comms, "udp", 3)) || (!strncmp(d->comms, "tcp", 3)) )  interface_open_inet(d);
    else if   (!strncmp(d->comms, "ipc", 3))                                      interface_open_ipc(d);
    else { fprintf(stderr, "Device %s unknown\n", d->id); exit(EXIT_FAILURE);}
    if(hal_verbose) fprintf(stderr, "File open succeeded for %s (with fdr=%d fdw=%d)\n", d->id, d->readfd, d->writefd);
  }
}

/**********************************************************************/
/* HAL MAP processing */
/*********t************************************************************/

/* Return halmap with from selector matching PDU selector from the halmap list */
halmap *halmap_find(pdu *p, halmap *map_root) {
//  fprintf(stderr, "%s", __func__); pdu_print(p);
  for(halmap *hm = map_root; hm != NULL; hm = hm->next) {
//    halmap_print_one(hm);
    if ( strcmp(hm->from.dev, p->psel.dev) == 0 
         && (hm->from.tag.mux == p->psel.tag.mux)
         && (hm->from.tag.sec == p->psel.tag.sec)
         && (hm->from.tag.typ == p->psel.tag.typ))
      return (hm);
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
/* PDU Decoding from packet arriving at HAL */
/*********t************************************************************/
/* Put closure packet (in buf) into internal HAL PDU structure (*p) */
void pdu_from_pkt_c (pdu *out, uint8_t *in) {
  pkt_c  *pkt = (pkt_c *) in;
  
  if(hal_verbose) fprintf(stderr, "HAL put packet into internal PDU\n");
  tag_decode(&(out->psel.tag), &(pkt->tag));
  len_decode(&(out->data_len), pkt->data_len);
  memcpy(out->data, pkt->data, out->data_len);
//  pdu_print(p);
}

/* Put data from buf (using M1 model) into internal HAL PDU */
void pdu_from_pkt_m1 (pdu *out, uint8_t *in, int len) {
  pkt_m1  *pkt = (pkt_m1 *) in;
  tlv_m1  *tlv;
    
  if(hal_verbose) {fprintf(stderr, "HAL put packet (len = %d) into internal PDU: ", len); m1_print(pkt);}
  out->psel.tag.mux = ntohl(pkt->session_tag);
  out->psel.tag.sec = ntohl(pkt->message_tag);
//  fprintf(stderr, "YYYY: count=%d\n", ntohl(pkt->message_tlv_count));
  for (int i=0; i < ntohl(pkt->message_tlv_count); i++) {
    tlv = &(pkt->tlv[0]);
    out->psel.tag.typ = ntohl(tlv->data_tag);
    out->data_len = ntohl(tlv->data_len);
    memcpy (out->data, tlv->data, out->data_len);
  }
}

/* Put data from buf (using G1 model) into internal HAL PDU */
void pdu_from_pkt_g1 (pdu *out, uint8_t *in , int len) {
  pkt_g1    *pkt = (pkt_g1 *) in;
  
  if(hal_verbose) { fprintf(stderr, "HAL put packet (len = %d) into internal PDU: ", len); g1_print(pkt);}
  out->psel.tag.mux = ntohl(pkt->message_tag_ID);
  out->psel.tag.sec = 0;         /* 0 is a don't care */
  out->psel.tag.typ = 0;         /* 0 is a don't care */
  out->data_len     = ntohs(pkt->data_len);
  memcpy (out->data, pkt->data, out->data_len);
}

/* Write packet into internal PDU */
pdu *pdu_from_packet(pdu *out, uint8_t *in, int len_in, device *idev) {
  out = malloc(sizeof(pdu));
  out->psel.dev = strdup(idev->id);
  if      (strcmp(idev->model, "pkt_c")  == 0)  pdu_from_pkt_c  (out, in);
  else if (strcmp(idev->model, "pkt_m1") == 0)  pdu_from_pkt_m1 (out, in, len_in);
  else if (strcmp(idev->model, "pkt_g1") == 0)  pdu_from_pkt_g1 (out, in, len_in);
  else {fprintf(stderr, "%s: unknown interface model: %s\n", __func__, idev->model); exit(EXIT_FAILURE);}
  if(hal_verbose) {fprintf(stderr, "HAL created new PDU: "); pdu_print(out);}
  return (out);
}

/**********************************************************************/
/* PDU Encoding into packet sent by HAL */
/*********t************************************************************/
/* Put internal PDU into closure packet (in buf) */
int pdu_into_pkt_c (uint8_t *out, pdu *in, gaps_tag *otag) {
  size_t    pkt_len;
  pkt_c    *pkt = (pkt_c *) out;

  if(hal_verbose) {fprintf(stderr, "%s: ", __func__); pdu_print(in);}
  tag_encode(&(pkt->tag), otag);
  len_encode(&(pkt->data_len), in->data_len);
  memcpy(pkt->data, in->data, in->data_len);
  pkt_len = in->data_len + sizeof(pkt->tag) + sizeof(pkt->data_len);
  return (pkt_len);
}

/* Put data into buf (using M1 model) from internal HAL PDU */
/* Returns length of buffer */
int pdu_into_pkt_m1 (uint8_t *out, pdu *in, gaps_tag *otag) {
  pkt_m1  *pkt = (pkt_m1 *) out;
  tlv_m1  *tlv;

//  if(hal_verbose) {fprintf(stderr, "%s\n", __func__); pdu_print(in);}
  pkt->session_tag = htonl(otag->mux);
  pkt->message_tag = htonl(otag->sec);
  pkt->message_tlv_count = htonl(1);
  for (int i=0; i < 1; i++) {
    tlv = &(pkt->tlv[i]);
    tlv->data_tag = htonl(otag->typ);
    tlv->data_len = htonl(in->data_len);
    memcpy((char *) tlv->data, (char *) in->data, in->data_len);
  }
  /* XXX: Fix packet length to depend on message_tlv_count */
  return (sizeof(pkt->session_tag) + sizeof(pkt->message_tag) + sizeof(pkt->message_tlv_count) + sizeof(tlv->data_tag) + sizeof(tlv->data_len) + in->data_len);
}

/* Put data into buf (using bkend model) from internal HAL PDU */
/* Returns length of buffer */
int pdu_into_pkt_g1 (uint8_t *out, pdu *in, gaps_tag *otag) {
  pkt_g1    *pkt = (pkt_g1 *) out;
  uint16_t  len = (uint16_t) in->data_len;

  if(hal_verbose) {fprintf(stderr, "%s\n", __func__); pdu_print(in);}
  pkt->message_tag_ID = htonl(otag->mux);
  pkt->data_len = htons(len);
  pkt->crc16 = 0;   /* XXX what polynomial? */
  memcpy((char *) pkt->data, (char *) in->data, in->data_len);
  return (sizeof(pkt->message_tag_ID) + sizeof(pkt->data_len) + sizeof(pkt->crc16) + in->data_len);
}

/* Write packet from internal PDU into packet */
void pdu_into_packet(uint8_t *out, pdu *in, int *pkt_len, gaps_tag *otag, const char *dev_model) {
  if      (strcmp(dev_model, "pkt_c")  == 0)  *pkt_len = pdu_into_pkt_c  (out, in, otag);
  else if (strcmp(dev_model, "pkt_m1") == 0)  *pkt_len = pdu_into_pkt_m1 (out, in, otag);
  else if (strcmp(dev_model, "pkt_g1") == 0)  *pkt_len = pdu_into_pkt_g1 (out, in, otag);
  else {fprintf(stderr, "%s unknown interface model %s", __func__, dev_model); exit(EXIT_FAILURE);}
  if(hal_verbose) {fprintf(stderr, "HAL writing: "); data_print("Packet", out, *pkt_len);}
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
    rv = read(fd, buf, PACKET_MAX);     /* write = send for tcp with no flags */
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
  return(pdu_from_packet(ret, buf, pkt_len, idev));
}

/* Write pdu to specified fd */
void write_pdu(device *odev, gaps_tag *otag, pdu *p) {
  int             rv=-1;
  int             fd;
  int             pkt_len=0;
  const char     *com_type;
  static uint8_t  buf[PACKET_MAX];

  if (hal_verbose) {
    fprintf(stderr, "HAL write to "); tag_print(otag);
    fprintf(stderr, "on"); devices_print_one(odev);
  }
  /* a) Convert into packet based on interface packet model  */
  pdu_into_packet(buf, p, &pkt_len, otag, odev->model);

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

  write_pdu(odev, &(h->to.tag), ipdu);
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
  devs    = get_devices(&cfg);
  map     = get_mappings(&cfg);
  config_destroy(&cfg);
  devices_open(devs);
  devices_print_all(devs);
  halmap_print_all(map);
  read_wait_loop(devs, map);
  return 0;
}
