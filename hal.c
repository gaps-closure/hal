/* TODO list (200224)
XXX: udp interface
XXX: Rename models hal2hal to pkt_c
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
/* HAL Interface parameters */
typedef struct _dev {
  int         enabled;
  const char *id;
  const char *path;
  const char *model;
  const char *comms;
  const char *addr_bi;
  const char *addr_in;
  const char *addr_out;
  int         port;
  int         readfd;
  int         writefd;
  struct _dev *next;
} device;

/* HAL Selector (used in HAL map entries and PDUs) */
typedef struct _sel {
  char     *dev;
  gaps_tag  tag;
} selector;

/* HAL map entry */
typedef struct _hal {
  selector    from;
  selector    to;
  char        *codec;
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
/* HAL Print (structures) */
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
  if (strlen(d->addr_bi)  > 0) fprintf(stderr, " ab=%s", d->addr_bi);
  if (strlen(d->addr_in)  > 0) fprintf(stderr, " ai=%s", d->addr_in);
  if (strlen(d->addr_out) > 0) fprintf(stderr, " ao=%s", d->addr_out);
  if (d->port             > 0) fprintf(stderr, " p=%d",  d->port);
  if (d->readfd          >= 0) fprintf(stderr, " r=%d",  d->readfd);
  if (d->writefd         >= 0) fprintf(stderr, " w=%d",  d->writefd);
  fprintf(stderr, "]\n");
}
  
/* Print list of devices for debugging */
void devices_print_all(device *root)  {
  fprintf(stderr, "HAL device list (%p):\n", (void *) root);
  for(device *d = root; d != NULL; d = d->next) devices_print_one(root);
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
char *get_cfgstr(config_t *cfg, char *fld) {
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
      ret[i].addr_bi  = get_param_str(dev, "addr_bi",  1, i);
      ret[i].addr_in  = get_param_str(dev, "addr_in",  1, i);
      ret[i].addr_out = get_param_str(dev, "addr_out", 1, i);
      ret[i].port     = get_param_int(dev, "port",     1, i);
      ret[i].readfd   = -1; /* to be set when opened */
      ret[i].writefd  = -1; /* to be set when opened */
      ret[i].next     = i < count - 1 ? &ret[i+1] : (device *) NULL;
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
      const char *from_dev, *to_dev, *codec;
      int   from_mux, from_sec, from_typ, to_mux, to_sec, to_typ;
      
      config_setting_t *entry = config_setting_get_elem(hmaps, i);
      if(!(  config_setting_lookup_string(entry, "from_dev", &from_dev)
          && config_setting_lookup_string(entry, "to_dev",   &to_dev)
          && config_setting_lookup_string(entry, "codec",    &codec)
          && config_setting_lookup_int(   entry, "from_mux", &from_mux)
          && config_setting_lookup_int(   entry, "from_sec", &from_sec)
          && config_setting_lookup_int(   entry, "from_typ", &from_typ)
        && config_setting_lookup_int(   entry, "to_mux", &to_mux)
        && config_setting_lookup_int(   entry, "to_sec", &to_sec)
        && config_setting_lookup_int(   entry, "to_typ", &to_typ)
        )) {
             fprintf(stderr, "Incorrect fields for device %d\n", i);
             exit(EXIT_FAILURE);
      }
      ret[i].from.dev = strdup(from_dev);
      ret[i].to.dev   = strdup(to_dev);
      ret[i].codec    = strdup(codec);
      ret[i].from.tag.mux = from_mux;
      ret[i].from.tag.sec = from_sec;
      ret[i].from.tag.typ = from_typ;
      ret[i].to.tag.mux = to_mux;
      ret[i].to.tag.sec = to_sec;
      ret[i].to.tag.typ = from_typ;

      /*
      config_setting_t *tag = config_lookup(cfg, maps.[i].from_tag);
  fprintf(stderr, "tag=%p\n", tag);

 && config_setting_lookup_int(entry, "from_tag", from_list)

      fprintf(stderr, "from_list=%d\n", from_list[0]);

       int tag_count = config_setting_length(entry->from_tag);
      fprintf(stderr, "tag_count=%d\n", tag_count);


      const config_setting_t *tag;
      tag = config_lookup(cfg, "maps.from_tag");
if(tag != NULL) {
      int tag_count = config_setting_length(tag);
}
           printf("I have %d retries:\n", count);
           for (n = 0; n < count; n++) {
             printf("\t#%d. %d\n", n + 1,
               config_setting_get_int_elem(retries, n));
           }
      if config_setting_is_array(entry) {
             = strdup(config_setting_get_string_elem(entry,0));
        ret[i].from.tag.mux = config_setting_get_int_elem(entry,1);
        ret[i].from.tag.sec = config_setting_get_int_elem(entry,2);
        ret[i].from.tag.typ = config_setting_get_int_elem(entry,3);
        ret[i].to.dev       = strdup(config_setting_get_string_elem(entry,4));
        ret[i].to.tag.mux   = config_setting_get_int_elem(entry,5);
        ret[i].to.tag.sec   = config_setting_get_int_elem(entry,6);
        ret[i].to.tag.typ   = config_setting_get_int_elem(entry,7);
        ret[i].codec        = strdup(config_setting_get_string_elem(entry,8));
 */
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
device *find_device_by_id(device *root, char *id) {
  for(device *d = root; d != NULL; d = d->next) {
    // fprintf(stderr, "%s: list=%s find=%s\n", __func__, d->id, id);
    if ( (d->enabled != 0) && (strcmp(d->id, id) == 0) )  return d;
  }
  return ((device *) NULL);
}

/* Open an IPC interface for read and for write and return their fds */
void interface_open_ipc(device *d) {
  int  pid;
  int  read_pipe[2];   /* Read pipes for parent-child communication */
  int  write_pipe[2];  /* Write pipes for parent-child communication */

  fprintf(stderr, "Open IPC %s %s\n", d->id, d->path);
  if(pipe(read_pipe) < 0 || pipe(write_pipe) < 0) {
    fprintf(stderr, "Pipe creation failed\n");
    exit(EXIT_FAILURE);
  }
  if(hal_verbose) fprintf(stderr, "Pipe FDs: hal_r=%d hal_w=%d zc_sub_w=%d zc_pub_r=%d\n", PARENT_READ, PARENT_WRITE,  CHILD_WRITE, CHILD_READ);
  
  if ((pid = fork()) < 0) {
    fprintf(stderr, "Fork failed\n");
    exit(EXIT_FAILURE);
  } else if (pid == 0) { /* subscriber child */
    close(PARENT_READ);
    close(PARENT_WRITE);
    close(CHILD_READ);
    dup2(CHILD_WRITE, STDOUT_FILENO);   /* zc sub output goes to pipe (CHILD_WRITE) */
    char *argv2[] = {(char *) d->path, "-b", "sub", (char *) d->addr_in, NULL};
    if(execvp(argv2[0], argv2) < 0) perror("execvp()");
    exit(EXIT_FAILURE);
  } else { /* save subscriber child pid in parent */
    if(hal_verbose) fprintf(stderr, "Spawned %s subscriber pid=%d\n", d->path, pid);
  }

  if ((pid = fork()) < 0) {
    fprintf(stderr, "Fork failed\n");
    exit(EXIT_FAILURE);
  } else if (pid == 0) { /* publisher child */
    close(PARENT_READ);
    close(PARENT_WRITE);
    dup2(CHILD_READ, STDIN_FILENO);
    close(CHILD_WRITE);
    char *argv2[] = {(char *) d->path, "-b", "pub", (char *) d->addr_out, NULL};
//    char *argv2[] = {zcpath, "-b", "-v", "-d", delim, "pub", zcpub, NULL};
    if(execvp(argv2[0], argv2) < 0) perror("execvp()");
    exit(EXIT_FAILURE);
  } else { /* save publisher child pid in parent */
    if(hal_verbose) fprintf(stderr, "Spawned %s publisher pid=%d\n", d->path, pid);
  }

  close(CHILD_READ);
  close(CHILD_WRITE);
  d->readfd  = PARENT_READ;
  d->writefd = PARENT_WRITE;
}

/* Open a network socket for read-write and return its fd */
void interface_open_socket(device *d) {
  int fd;
  struct sockaddr_in serv_addr;

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("\n Socket creation error for %s\n", d->id);
    exit(EXIT_FAILURE);
  }
  
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(d->port);
  if(inet_pton(AF_INET, d->addr_bi, &serv_addr.sin_addr)<=0)
  {
    printf("\nInvalid address/ Address not supported for %s %s\n", d->id, d->path);
    exit(EXIT_FAILURE);
  }
  
  if (connect(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("\nConnection Failed for %s %s\n", d->id, d->path);
    exit(EXIT_FAILURE);
  }
  d->readfd = fd;
  d->writefd = fd;
}

/* Open a serial interface for read-write and return its fd */
void interface_open_serial(device *d) {
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
    /**/
    if (!strncmp(d->path, "/dev/", 5))  interface_open_serial(d);
    else if (strlen(d->addr_bi)  > 0)   interface_open_socket(d);
    else if (strlen(d->addr_in)  > 0)   interface_open_ipc(d);
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
/* PDU functions (including coding and decoding) */
/*********t************************************************************/
/* Free memory allocated for PDU */
void pdu_delete(pdu *pdu) {
  free(pdu->psel.dev);
}

/* Put closure packet (in buf) into internal HAL PDU structure (*p) */
void pdu_from_closure (pdu *out, uint8_t *in) {
  pkt_c  *pkt = (pkt_c *) in;
  
  tag_decode(&(out->psel.tag), &(pkt->tag));
  len_decode(&(out->data_len), pkt->data_len);
  memcpy(out->data, pkt->data, out->data_len);
//  pdu_print(p);
}

/* Put internal PDU into closure packet (in buf) */
int pdu_into_closure (uint8_t *out, pdu *in, gaps_tag *otag) {
  size_t    pkt_len;
  pkt_c    *pkt = (pkt_c *) out;

  if(hal_verbose) {fprintf(stderr, "%s: ", __func__); pdu_print(in);}
  tag_encode(&(pkt->tag), otag);
  len_encode(&(pkt->data_len), in->data_len);
  memcpy(pkt->data, in->data, in->data_len);
  pkt_len = in->data_len + sizeof(pkt->tag) + sizeof(pkt->data_len);
  return (pkt_len);
}

/* Default Convert from haljson string to bkend integer (in network order) */
uint32_t pdu_field_into_m1(uint32_t tag_field) {
    return (htonl(tag_field));
}

/* Default Convert to haljson string from bkend integer (in network order) */
uint32_t pdu_field_from_m1(uint32_t tag_field) {
  return (ntohl(tag_field));
}

/* Put data from buf (using M1 model) into internal HAL PDU */
void pdu_from_m1 (uint8_t *buf, const char *dev_id, pdu *p, int len) {
  pkt_m1  *b = (pkt_m1 *) buf;
  tlv_m1  *tlv;
    
  if(hal_verbose) fprintf(stderr, "Put M1 packet data from dev=%s into internal HAL PDU\n", dev_id);
  p->psel.dev = strdup(dev_id);
// fprintf(stderr, "HAL get input on %s: ", dev_id); m1_print((bkend *) buf);
  p->psel.tag.mux = pdu_field_from_m1(b->session_tag);
  p->psel.tag.sec = pdu_field_from_m1(b->message_tag);
//  fprintf(stderr, "YYYY: count=%d\n", ntohl(b->message_tlv_count));
  for (int i=0; i < ntohl(b->message_tlv_count); i++) {
    tlv = &(b->tlv[0]);
    p->psel.tag.typ = pdu_field_from_m1(tlv->data_tag);
    p->data_len = pdu_field_from_m1(tlv->data_len);
    memcpy (p->data, tlv->data, p->data_len);
  }
}

/* Put data into buf (using M1 model) from internal HAL PDU */
/* Returns length of buffer */
int pdu_into_m1 (uint8_t *out, pdu *in, gaps_tag *otag) {
  pkt_m1  *pkt = (pkt_m1 *) out;
  tlv_m1  *tlv;

//  if(hal_verbose) {fprintf(stderr, "%s\n", __func__); pdu_print(in);}
  pkt->session_tag = pdu_field_into_m1(otag->mux);
  pkt->message_tag = pdu_field_into_m1(otag->sec);
  pkt->message_tlv_count = htonl(1);
  for (int i=0; i < 1; i++) {
    tlv = &(pkt->tlv[i]);
    tlv->data_tag = pdu_field_into_m1(otag->typ);
    tlv->data_len = htonl(in->data_len);
    memcpy((char *) tlv->data, (char *) in->data, in->data_len);
  }
  /* XXX: Fix packet length to depend on message_tlv_count */
  return (sizeof(pkt->session_tag) + sizeof(pkt->message_tag) + sizeof(pkt->message_tlv_count) + sizeof(tlv->data_tag) + sizeof(tlv->data_len) + in->data_len);
}

/* Put data from buf (using G1 model) into internal HAL PDU */
void pdu_from_g1 (uint8_t *buf, const char *dev_id, pdu *p, int len) {
  pkt_g1    *pkt = (pkt_g1 *) buf;

  if(hal_verbose) fprintf(stderr, "Put G1 packet data from dev=%s into internal HAL PDU\n", dev_id);
  g1_print(pkt);
  exit (22);
}

/* Put data into buf (using bkend model) from internal HAL PDU */
/* Returns length of buffer */
int pdu_into_g1 (uint8_t *out, pdu *in, gaps_tag *otag) {
  pkt_g1    *pkt = (pkt_g1 *) out;
  uint16_t  len = (uint16_t) in->data_len;

  if(hal_verbose) {fprintf(stderr, "%s\n", __func__); pdu_print(in);}
  pkt->message_tag_ID = htonl(otag->mux);
  pkt->data_len = htons(len);
  pkt->crc16 = 0;   /* XXX what polynomial? */
  memcpy((char *) pkt->data, (char *) in->data, in->data_len);
  return (sizeof(pkt->message_tag_ID) + sizeof(pkt->data_len) + sizeof(pkt->crc16) + in->data_len);
}

/**********************************************************************/
/* HAL Read and Write Functions */
/*********t************************************************************/
/* Read device and return pdu */
/* Uses idev to determines how to parse, then extracts selector info and fill psel */
pdu *read_pdu(device *idev) {
  pdu            *ret = NULL;
  static uint8_t  buf[PACKET_MAX];
  int             len;
  int             fd;
  const char     *dev_id;

  /* a) read input into buf and get its length (with input dev_id and fd) */
  dev_id = idev->id;
  fd = idev->readfd;

  len = read(fd, buf, PACKET_MAX);
  if(hal_verbose) {
    fprintf(stderr, "HAL read input on %s (model=%s) fd=%d", dev_id, idev->model, fd);
    data_print("", (uint8_t *) buf, len);
  }
  /* b) Write input into internal PDU */
  ret = malloc(sizeof(pdu));
  ret->psel.dev = strdup(dev_id);
 
  if (strcmp(idev->model, "pkt_c") == 0) {
//    fprintf(stderr, "HAL reading from %s: ", dev_id);
    pdu_from_closure (ret, buf);
  } else if (strcmp(idev->model, "pkt_m1") == 0) {
    pdu_from_m1 (buf, dev_id, ret, len);
    fprintf(stderr, "HAL reading from %s: ", dev_id); m1_print((pkt_m1 *) buf);
  } else if (strcmp(idev->model, "pkt_g1") == 0) {
      pdu_from_g1 (buf, dev_id, ret, len);
  } else {
    fprintf(stderr, "Error, unknown interface model: %s\n", idev->model);
    exit (22);
  }
  if(hal_verbose) pdu_print(ret);
  return ret;
}

/* Write pdu to specified fd */
void write_pdu(device *odev, gaps_tag *otag, pdu *p) {
  int             rv=-1;
  int             fd;
  int             pkt_len=0;
  const char     *dev_model;
  const char     *com_type;
  static uint8_t  buf[PACKET_MAX];

  if (hal_verbose) {
    fprintf(stderr, "HAL write to "); tag_print(otag);
    fprintf(stderr, "on"); devices_print_one(odev);
  }
  /* a) Convert into packet based on interface packet model  */
  dev_model = odev->model;
  if      (strcmp(dev_model, "pkt_c") == 0)  pkt_len = pdu_into_closure (buf, p, otag);
  else if (strcmp(dev_model, "pkt_m1") == 0) pkt_len = pdu_into_m1 (buf, p, otag);
  else if (strcmp(dev_model, "pkt_g1") == 0) pkt_len = pdu_into_g1 (buf, p, otag);
  else {fprintf(stderr, "%s unknown interface model %s", __func__, dev_model); exit(EXIT_FAILURE);}
  if(hal_verbose) {fprintf(stderr, "HAL writing: "); data_print("Packet", buf, pkt_len);}

  /* b) Write to interace based on interface comms type */
  fd = odev->writefd;
  com_type = odev->comms;
  if(hal_verbose) fprintf(stderr, "HAL writing using comms type %s\n", com_type);
  if (   (strcmp(com_type, "ipc") == 0)
      || (strcmp(com_type, "serial") == 0)
      || (strcmp(com_type, "tcp") == 0)
     ) {
    rv = write(fd, buf, pkt_len);     /* write = send for tcp with no flags */
  }
  else if (strcmp(com_type, "udp") == 0) {
    fprintf(stderr, "XXX: Write udp mode to %s interface\n", com_type);
//    len = sizeof(cliaddr);  //len is value/resuslt
//    rv = sendto(fd, buf, pkt_len), MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);
//    rv = recvfrom(fd, buf, pkt_len, MAXLINE, ( struct sockaddr *) &cliaddr, &len);
  }
  else {fprintf(stderr, "%s unknown comms type %s", __func__, com_type); exit(EXIT_FAILURE);}
  
  if(hal_verbose) fprintf(stderr, "HAL wrote on %s (fd=%d) and got rv=%d\n", odev->id, fd, rv);
}

/* Process input from device (with 'input_fd') and send to output */
void process_input(int ifd, halmap *map, device *devs) {
  pdu    *ipdu; //, *opdu;
  device *idev, *odev;
  halmap *h;
  
  idev = find_device_by_readfd(devs, ifd);
  if(idev == NULL) { 
    fprintf(stderr, "Device not found for input fd\n");
    return; 
  } 

  ipdu = read_pdu(idev);
  if(ipdu == NULL) { 
    fprintf(stderr, "Input PDU is NULL\n");
    return; 
  }

  h = halmap_find(ipdu, map);
  if(h == NULL) { 
    fprintf(stderr, "No matching HAL map entry\n");
    pdu_delete(ipdu);
    return; 
  }
  
  odev = find_device_by_id(devs, h->to.dev);
  if(odev == NULL) { 
    fprintf(stderr, "Device not found for output\n");
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
