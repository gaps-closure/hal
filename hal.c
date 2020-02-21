/**********************************************************************/
/* HAL Library Includes and Deinitions */
/**********************************************************************/

#include "closure.h"

#include <unistd.h>	
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libconfig.h>
#include <arpa/inet.h>

#define BKEND_MAX_TLVS 4
#define BKEND_MIN_HEADER_BYTES 20
#define HALJSON_MAX_TAGS 3
#define HALJSON_DELIM_DATA " "
#define HALJSON_DELIM_TAGS "-"

#define ADU_SIZE_MAX   100
#define PACKET_MAX     (ADU_SIZE_MAX + 30)

int hal_verbose=0;

/**********************************************************************/
/* HAL Structures */
/*********t************************************************************/
/* HAL Device */
typedef struct _dev {
  int         enabled;
  int         readfd;
  int         writefd;
  const char *id;
  const char *path;
  const char *model;
  const char *host;
  int         port;
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

/* Internal PDU (NB: PDU data is opaque - serialized for network transer) */
typedef struct _pdu {
  selector  psel;                 /* Input device and tag info */
  size_t    data_len;
  uint8_t   data[ADU_SIZE_MAX];
} pdu;

/* BKEND TLV */
typedef struct _btlv {
  uint32_t  data_tag;     /* Type (e.g., DATA_PAYLOAD_1) */
  uint32_t  data_len;     /* Length (in bytes) */
  uint8_t   data[236];    /* Value (up to 236 (256 - 5*4) bytes of payload) */
} bkend_tlv;

/* BKEND packet */
typedef struct _bkd {
  uint32_t  session_tag;           /* App Mux */
  uint32_t  message_tag;           /* Security */
  uint32_t  message_tlv_count;     /* TLV count (1 for demo) */
  bkend_tlv tlv[BKEND_MAX_TLVS];   /* TLV */
} bkend;


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

/* Print list of devices for debugging */
void devices_print_all(device *root)  {
  fprintf(stderr, "HAL device list (%p):\n", (void *) root);
  for(device *d = root; d != NULL; d = d->next) {
    fprintf(stderr, "  [%s: v=%d m=%s p=%s r=%d w=%d", d->id, d->enabled, d->model, d->path, d->readfd, d->writefd);
    if ( (strncmp(d->path, "/dev/", 5)) && (strncmp(d->path, "zcpat", 5)) ) {
      fprintf(stderr, " h=%s p=%d", d->host, d->port);
    }
    fprintf(stderr, "]\n");
  }
//  fprintf(stderr, "\n");
}

/* Print a single HAL map entry for debugging */
void halmap_print_one(halmap *hm) {
  fprintf(stderr, "  [%s ", hm->from.dev); tag_print(&(hm->from.tag));
  fprintf(stderr, "-> %s ", hm->to.dev); tag_print(&(hm->to.tag));
  fprintf(stderr, ", codec=%s]\n", hm->codec);
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
      ret[i].id      = get_param_str(dev, "id",      0, i);
      ret[i].path    = get_param_str(dev, "path",    0, i);
      ret[i].model   = get_param_str(dev, "model",   0, i);
      ret[i].host    = get_param_str(dev, "host",    1, i);
      ret[i].enabled = get_param_int(dev, "enabled", 0, i);
      ret[i].port    = get_param_int(dev, "port",    1, i);
      ret[i].readfd  = -1; /* to be set when opened */
      ret[i].writefd = -1; /* to be set when opened */
      ret[i].next    = i < count - 1 ? &ret[i+1] : (device *) NULL;
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

/* Open enabled devices (from linked-list of devices) and get their in/out handles */
void devices_open(device *dev_linked_list_root) {
  int fd;

  for(device *d = dev_linked_list_root; d != NULL; d = d->next) {
    if (d->enabled == 0) continue;
    if(hal_verbose) fprintf(stderr, "About to open device: %s %s test=%d\n", d->id, d->path, strncmp(d->path, "/dev/", 5));
    if (!strncmp(d->path, "/dev/", 5))  {
      /* Open device for read-write, get fd and update device entry */
      if ((fd = open(d->path, O_RDWR)) < 0) {
        fprintf(stderr, "Error opening device: %s %s\n", d->id, d->path);
        exit(EXIT_FAILURE);
      }
    }
    else {
      if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
      {
        printf("\n Socket creation error for %s\n", d->id);
        exit(EXIT_FAILURE);
      }
      struct sockaddr_in serv_addr;
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_port = htons(d->port);
      if(inet_pton(AF_INET, d->host, &serv_addr.sin_addr)<=0)
      {
        printf("\nInvalid address/ Address not supported for %s %s\n", d->id, d->path);
        exit(EXIT_FAILURE);
      }
      if (connect(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
      {
        printf("\nConnection Failed for %s %s\n", d->id, d->path);
        exit(EXIT_FAILURE);
      }
    }
    d->readfd = fd;
    d->writefd = fd;
    if(hal_verbose) fprintf(stderr, "File open succeeded for %s (with fdr=%d fdw=%d)\n", d->id, fd, fd);
  }
}

/**********************************************************************/
/* HAL MAP processing */
/*********t************************************************************/

/* Return halmap with from selector matching PDU selector from the halmap list */
halmap *halmap_find(pdu *p, halmap *map_root) {
  for(halmap *hm = map_root; hm != NULL; hm = hm->next) {
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
void closure_parse_into_PDU (pdu *out, cpkt *in) {
  tag_decode(&(out->psel.tag), &(in->tag));
  len_decode(&(out->data_len), in->data_len);
  memcpy(out->data, in->data, out->data_len);
//  pdu_print(p);
}

/* Put internal PDU into closure packet (in buf) */
int closure_parse_from_PDU (cpkt *out, pdu *in, gaps_tag *otag, char *delim) {
  size_t        packet_len;

  if(hal_verbose) {fprintf(stderr, "%s: ", __func__); pdu_print(in);}
//  tag_encode(&(out->tag), &(in->psel.tag));
  tag_encode(&(out->tag), otag);
  len_encode(&(out->data_len), in->data_len);
  memcpy(out->data, in->data, in->data_len);
  packet_len = in->data_len + sizeof(out->tag) + sizeof(out->data_len);
//  fprintf(stderr, "HAL created ");  data_print("Packet", (uint8_t *) out, packet_len);
  return (packet_len);
}

/* Print bkend (assuming data is a string) */
void bkend_print(bkend *b) {
  bkend_tlv *tlv;
  
  fprintf(stderr, "%s: ", __func__);
  fprintf(stderr, "mux=%u ",  ntohl(b->session_tag));
  fprintf(stderr, "sec=%u ",  ntohl(b->message_tag));
  for (int i=0; i < ntohl(b->message_tlv_count); i++) {
    tlv = &(b->tlv[i]);
    fprintf(stderr, "[ty=%u ", ntohl(tlv->data_tag));
    fprintf(stderr, "l=%d ",   ntohl(tlv->data_len));
    fprintf(stderr, "%s] ",          tlv->data);
  }
  fprintf(stderr, "\n");
}

/* Default Convert from haljson string to bkend integer (in network order) */
uint32_t haljson_to_bkend(uint32_t tag_field) {
    return (htonl(tag_field));
}

/* Default Convert to haljson string from bkend integer (in network order) */
uint32_t haljson_from_bkend(uint32_t tag_field) {
  return (ntohl(tag_field));
}


/* Put data into buf (using bkend model) from internal HAL PDU */
/* Returns length of buffer */
int bkend_parse_from_PDU (uint8_t *buf, pdu *p, gaps_tag *otag, const char *dev_id) {
  bkend     *b;
  bkend_tlv *tlv;
  
  if(hal_verbose) {fprintf(stderr, "%s for device %s: ", __func__, dev_id); pdu_print(p);}
  b = (bkend *) buf;
  b->session_tag = haljson_to_bkend(p->psel.tag.mux);
  b->message_tag = haljson_to_bkend(p->psel.tag.sec);
  b->message_tlv_count = htonl(1);
  for (int i=0; i < 1; i++) {
    tlv = &(b->tlv[i]);
    tlv->data_tag = haljson_to_bkend(p->psel.tag.typ);
    tlv->data_len = htonl(p->data_len);
    memcpy((char *) tlv->data, (char *) p->data, p->data_len);
  }
  /* XXX: Fix length to depend on message_tlv_count */
  return ((p->data_len) + BKEND_MIN_HEADER_BYTES);
}

/* Put data from buf (using bkend model) into internal HAL PDU */
void bkend_parse_into_PDU (char *buf, const char *dev_id, pdu *p, int len) {
  bkend     *b;
  bkend_tlv *tlv;
    
  if(hal_verbose) fprintf(stderr, "Put bkend model data from dev=%s into internal HAL PDU\n", dev_id);
  
  /* XXX: Handle case where BKEND message is split - using framing? */
  if (len < BKEND_MIN_HEADER_BYTES) {
    fprintf(stderr, "XXX: BKEND Message split?: len=%d\n", len);
    exit (22);
  }
  
  p->psel.dev = strdup(dev_id);
  b = (bkend *) buf;
// fprintf(stderr, "HAL get input on %s: ", dev_id); bkend_print((bkend *) buf);
  p->psel.tag.mux = haljson_from_bkend(b->session_tag);
  p->psel.tag.sec = haljson_from_bkend(b->message_tag);
//  fprintf(stderr, "YYYY: count=%d\n", ntohl(b->message_tlv_count));
  for (int i=0; i < ntohl(b->message_tlv_count); i++) {
    tlv = &(b->tlv[0]);
    p->psel.tag.typ = haljson_from_bkend(tlv->data_tag);
    p->data_len = haljson_from_bkend(tlv->data_len);
    memcpy (p->data, tlv->data, p->data_len);
  }
}

/**********************************************************************/
/* HAL Read and Write Functions */
/*********t************************************************************/
/* Read device and return pdu */
/* Uses idev to determines how to parse, then extracts selector info and fill psel */
pdu *read_pdu(device *idev) {
  pdu         *ret = NULL;
  static char  buf[PACKET_MAX];
  int          len;
  int          fd;
  const char  *dev_id;

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
 
  if (strcmp(idev->model, "haljson") == 0) {
//    fprintf(stderr, "HAL reading from %s: ", dev_id);
    closure_parse_into_PDU (ret, (cpkt *) buf);
  } else if (strcmp(idev->model, "bkend") == 0) {
    bkend_parse_into_PDU   (buf, dev_id, ret, len);
    fprintf(stderr, "HAL reading from %s: ", dev_id); bkend_print((bkend *) buf);
  } else {
    fprintf(stderr, "Error, unkwown device model: %s\n", idev->model);
    exit (22);
  }
  if(hal_verbose) pdu_print(ret);
  return ret;
}

/* Write pdu to specified fd */
void write_pdu(device *odev, gaps_tag *otag, pdu *p, char *delim) {
  int             rv;
  int             fd;
  int             packet_len=0;
  const char     *dev_id;
  const char     *dev_model;
  static uint8_t  buf[PACKET_MAX];

  tag_print(otag);
  dev_id = odev->id;
  dev_model = odev->model;
  if (strcmp(dev_model, "haljson") == 0) {
    packet_len = closure_parse_from_PDU ((cpkt *) buf, p, otag, delim);
  } else if (strcmp(dev_model, "bkend") == 0) {
    if(hal_verbose) fprintf(stderr, "Convert internal PDU into %s model for device %s\n", dev_model, dev_id);
    packet_len = bkend_parse_from_PDU (buf, p, otag, dev_id);
    fprintf(stderr, "HAL writing to %s: ", odev->id); bkend_print((bkend *) buf);
  }
  fd = odev->writefd;
  if(hal_verbose) {
    fprintf(stderr, "HAL writing on %s (fd=%d): ", dev_id, fd);
    data_print("Packet", buf, packet_len);
  }
  rv = write(fd, buf, packet_len);
//  rv = write(fd, "hello", 5);
//  exit (1);

  if(hal_verbose) fprintf(stderr, "HAL wrote on %s: rv=%d)\n", dev_id, rv);

}

/* Process input from device (with 'input_fd') and send to output */
void process_input(int ifd, halmap *map, device *devs, char *delim) {
  pdu    *ipdu; //, *opdu;
  device *idev, *odev;
  halmap *h;
  
  idev = find_device_by_readfd(devs, ifd);
  if(idev == NULL) { 
    fprintf(stderr, "Device not found for input fd");
    return; 
  } 

  ipdu = read_pdu(idev);
  if(ipdu == NULL) { 
    fprintf(stderr, "Input PDU is NULL"); 
    return; 
  }

  h = halmap_find(ipdu, map);
  if(h == NULL) { 
    fprintf(stderr, "No matching HAL map entry"); 
    pdu_delete(ipdu);
    return; 
  }
  
  odev = find_device_by_id(devs, h->to.dev);
  if(odev == NULL) { 
    fprintf(stderr, "Device not found for output");
    return; 
  }

  write_pdu(odev, &(h->to.tag), ipdu, delim);
  pdu_delete(ipdu);
}

/* Iniitialize file descriptor set for select (from linked-list of devices) */
int select_init(device *dev_linked_list_root, fd_set *readfds) {
  device   *d;             /* Temporary device pointer */
  int       maxrfd;        /* Maximum file descriptor number for select */

  FD_ZERO(readfds);
  maxrfd = -1;
  for(d = dev_linked_list_root; d != NULL; d = d->next) {
    if (d->enabled != 0) {
      if (d->readfd >= maxrfd) maxrfd = d->readfd + 1;
      FD_SET(d->readfd, readfds);
    }
  }
  return (maxrfd);     /* Maximum file descriptor number for select */
}

/**********************************************************************/
/* HAL MAIN */
/*********t************************************************************/
#define PARENT_READ  read_pipe[0]
#define PARENT_WRITE write_pipe[1]
#define CHILD_WRITE  read_pipe[1]
#define CHILD_READ   write_pipe[0]

int main(int argc, char **argv) {
  config_t  cfg;           /* Configuration */
  int       maxrfd;        /* Maximum file descriptor number for select */
  fd_set    readfds;       /* File descriptor set for select */
  char     *file_name;     /* Path to zc executable */
  char     *zcpath;        /* Path to zc executable */
  char     *zcmodel;       /* model of APP tag and data */
  char     *zcpub;         /* Publisher URL */
  char     *zcsub;         /* Subscriber URL */
  char     *delim;         /* Data end delimiter */
  device    zcroot;        /* Fake device for zc */
  device   *devs;          /* Linked list of enabled devices */
  halmap   *map;           /* Linked list of selector mappings */
  int       pid;           /* Process ID of child or parent from fork call */
  int       read_pipe[2];  /* Read pipes for parent-child communication */
  int       write_pipe[2]; /* Write pipes for parent-child communication */
  int       zcsubpid;      /* Subscriber PID */
  int       zcpubpid;      /* Publisher PID */
  
  file_name = opts_get(argc, argv);
  cfg_read(&cfg, file_name);
  devs    = get_devices(&cfg);
  zcpath  = get_cfgstr(&cfg, "zcpath");
  zcmodel = get_cfgstr(&cfg, "zcmodel");
  zcpub   = get_cfgstr(&cfg, "zcpub");
  zcsub   = get_cfgstr(&cfg, "zcsub");
  delim   = get_cfgstr(&cfg, "delim");
  map     = get_mappings(&cfg);
  config_destroy(&cfg);

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
    char *argv2[] = {zcpath, "-b", "sub", zcsub, NULL};
    if(execvp(argv2[0], argv2) < 0) perror("execvp()");
    exit(EXIT_FAILURE);
  } else { /* save subscriber child pid in parent */
    zcsubpid = pid;
  }

  if ((pid = fork()) < 0) {
    fprintf(stderr, "Fork failed\n");
    exit(EXIT_FAILURE);
  } else if (pid == 0) { /* publisher child */
    close(PARENT_READ);
    close(PARENT_WRITE);
    dup2(CHILD_READ, STDIN_FILENO);
    close(CHILD_WRITE);
    char *argv2[] = {zcpath, "-b", "-d", delim, "pub", zcpub, NULL};
//    char *argv2[] = {zcpath, "-b", "-v", "-d", delim, "pub", zcpub, NULL};
    if(execvp(argv2[0], argv2) < 0) perror("execvp()");
    exit(EXIT_FAILURE);
  } else { /* save publisher child pid in parent */
    zcpubpid = pid;
  }

  close(CHILD_READ);
  close(CHILD_WRITE);

  /* Set up fake device for zc for uniform handling in halmap */
  zcroot.enabled = 1;
  zcroot.id      = "zc";
  zcroot.path    = "zcpath";
  zcroot.model   = zcmodel;
  zcroot.next    = devs;
  zcroot.readfd  = PARENT_READ;
  zcroot.writefd = PARENT_WRITE;

  devices_open(devs);

//  fprintf(stderr, "zcpath=%s\n", zcpath);
//  fprintf(stderr, "zcpub=%s\n",  zcpub);
//  fprintf(stderr, "zcsub=%s\n",  zcsub);
//  fprintf(stderr, "delim=%s\n",  delim);
  if(hal_verbose) fprintf(stderr, "Spawned %s subscriber %d and publisher %d\n", zcpath, zcsubpid, zcpubpid);
  devices_print_all(&zcroot);
  halmap_print_all(map);

  /* Select across readfds */
  while (1) {
    int nready; 
      
    maxrfd = select_init(&zcroot,  &readfds);
    // fprintf(stderr, "HAL Waiting for input on fds (max+1=%d)\n", maxrfd);
    if((nready = select(maxrfd, &readfds, NULL, NULL, NULL)) == -1) perror("select()");
    // fprintf(stderr, "Selected n=%d max=%d\n", nready, maxrfd);
    for (int i = 0; i < maxrfd && nready > 0; i++) {
      if (FD_ISSET(i, &readfds)) {
        process_input(i, map, &zcroot, delim);
        nready--;
      }
    }
  }

  /* XXX: Log to specified logfile, not stderr */
  /* XXX: Properly daemonize, close standard fds, trap signals etc. */
  /* XXX: Deal with frag/defrag and other functionality etc. */
  /* XXX: Organize code with device, HAL, and codec/parser in separate files */

  return 0;
}
