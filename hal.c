#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>	
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libconfig.h>
#include <arpa/inet.h>

#define MAXPDU 4096
#define BKEND_MAX_TLVS 4
#define BKEND_MIN_HEADER_BYTES 20
#define HALJSON_MAX_TAGS 3
#define HALJSON_DELIM_DATA " "
#define HALJSON_DELIM_TAGS "-"

/* HAL Device structure */
typedef struct _dev {
  int enabled;
  int readfd;
  int writefd;
  const char *id;
  const char *path;
  const char *model;
  struct _dev *next;
} device;

/* HAL Selector structure used in HAL map entries and PDUs */
typedef struct _sel {
  char *dev;
  char *mux;
  char *sec;
  char *typ;
} selector;

/* HAL map entry structure */
typedef struct _hal {
  selector    from;
  selector    to;
  char        *codec;
  struct _hal *next;
} halmap;

/* Internal HAL PDU structure */
typedef struct _pdu {
  selector  psel;
  int       len;
  void     *payload;
} pdu;

/* BKEND TLV */
typedef struct _btlv {
  uint32_t  data_tag;     /* Type (e.g., DATA_PAYLOAD_1) */
  uint32_t  data_len;     /* Length (in bytes) */
  uint8_t   data[236];    /* Value (up to 236 (256 - 5*4) bytes of payload) */
} bkend_tlv;

/* BKEND packet structure */
typedef struct _bkd {
  uint32_t  session_tag;           /* App Mux */
  uint32_t  message_tag;           /* Security */
  uint32_t  message_tlv_count;     /* TLV count (1 for demo) */
  bkend_tlv tlv[BKEND_MAX_TLVS];   /* TLV */
} bkend;


void print_help() {
  printf("Hardware Abstraction Layer (HAL) for gaps CLOSURE project\n");
  printf("Usage: hal [OPTION]... CONFIG-FILE\n");
  printf("OPTION: one of the following options:\n");
  printf(" -h --help : print this message\n");
  printf(" -v --verbose : print some messages in stderr\n");
  printf("CONFIG-FILE: path to file with HAL configuration information (e.g., sample.cfg)\n");
}

int verbose=0;

/* Parse the configuration file */
void read_config (int argc, char **argv, config_t *cfg) {
  int opt;
  char  *file_name;
  
  if (argc < 2) {
    print_help();
    exit(EXIT_FAILURE);
  }
  while((opt =  getopt(argc, argv, "v")) != EOF)
  {
    switch (opt)
    {
      case 'v':
        verbose = 1;
        break;
      default:
        printf("\nSkipping undefined Option (%d)\n", opt);
        print_help();
    }
  }
  if(optind<argc) {
     file_name = argv[optind++];
  }
  if(verbose) fprintf(stderr, "Read script options: v=%d conifg-file=%s (optind=%d)\n", verbose, file_name, optind);
  config_init(cfg);
  if(! config_read_file(cfg, file_name))
  {
    fprintf(stderr, "%s:%d - %s\n", config_error_file(cfg), config_error_line(cfg), config_error_text(cfg));
    config_destroy(cfg);
    exit(EXIT_FAILURE);
  }
  if(verbose) fprintf(stderr, "Read configuration file: %s\n", file_name);
}

/* Get a top-level string item from config */
char *get_cfgstr(config_t *cfg, char *fld) {
  const char *ret;
  if(config_lookup_string(cfg, fld, &ret)) return strdup(ret);
  fprintf(stderr, "No '%s' setting in configuration file.\n", fld);
  exit(EXIT_FAILURE);
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
      int enabled;
      const char *id, *path, *model;
      config_setting_t *dev = config_setting_get_elem(devs, i);
      if(!(config_setting_lookup_string(dev, "id", &id)
           && config_setting_lookup_string(dev, "path", &path)
           && config_setting_lookup_string(dev, "model", &model)
           && config_setting_lookup_int(dev, "enabled", &enabled)
         )) {
        fprintf(stderr, "Incorrect fields for device %d\n", i);
        exit(EXIT_FAILURE);
      }
      ret[i].enabled = enabled;
      ret[i].id      = strdup(id);
      ret[i].path    = strdup(path);
      ret[i].model   = strdup(model);
      ret[i].next    = i < count - 1 ? &ret[i+1] : (device *) NULL;
      ret[i].readfd  = -1; /* to be set when opened */
      ret[i].writefd = -1; /* to be set when opened */
    }
  }
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
      config_setting_t *entry = config_setting_get_elem(hmaps, i);
      if config_setting_is_array(entry) {
        ret[i].from.dev = strdup(config_setting_get_string_elem(entry,0));
        ret[i].from.mux = strdup(config_setting_get_string_elem(entry,1));
        ret[i].from.sec = strdup(config_setting_get_string_elem(entry,2));
        ret[i].from.typ = strdup(config_setting_get_string_elem(entry,3));
        ret[i].to.dev   = strdup(config_setting_get_string_elem(entry,4));
        ret[i].to.mux   = strdup(config_setting_get_string_elem(entry,5));
        ret[i].to.sec   = strdup(config_setting_get_string_elem(entry,6));
        ret[i].to.typ   = strdup(config_setting_get_string_elem(entry,7));
        ret[i].codec    = strdup(config_setting_get_string_elem(entry,8));
        ret[i].next     = i < count - 1 ? &ret[i+1] : (halmap *) NULL;
      }
    }
  }
  return ret;
}

/* Print list of devices for debugging */
void devices_print_all(device *root)  {
    fprintf(stderr, "HAL device list (%p): ", (void *) root);
    for(device *d = root; d != NULL; d = d->next) {
        fprintf(stderr, "[%s: v=%d rfd=%d wfd=%d] ", d->id, d->enabled, d->readfd, d->writefd);
    }
    fprintf(stderr, "\n");
}

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
    if(verbose) fprintf(stderr, "About to open device: %s %s\n", d->id, d->path);
    /* Open device for read-write, get fd and update device entry */
    if ((fd = open(d->path, O_RDWR)) < 0) {
      fprintf(stderr, "Error opening device: %s %s\n", d->id, d->path);
      exit(EXIT_FAILURE);
    }
    d->readfd = fd;
    d->writefd = fd;
  }
}

/* Print a single HAL map entry for debugging */
void halmap_print_one(halmap *hm) {
    fprintf(stderr, "HAL map  (%p): ", (void *) hm);
    fprintf(stderr, "[%s ->", hm->from.dev);
    fprintf(stderr, " %s]\n", hm->to.dev);
}

/* Print list of HAL map entries for debugging */
void halmap_print_all(halmap *map_root) {
    fprintf(stderr, "HAL map list (%p): ", (void *) map_root);
    for(halmap *hm = map_root; hm != NULL; hm = hm->next) {
      fprintf(stderr, "[%s %s -> ", hm->from.dev, hm->from.mux);
      fprintf(stderr, "%s %s] ", hm->to.dev, hm->to.mux);
    }
    fprintf(stderr, "\n");
}

/* Return halmap with from selector matching PDU selector from the halmap list */
halmap *halmap_find(pdu *p, halmap *map_root) {
  for(halmap *hm = map_root; hm != NULL; hm = hm->next) {
    if ( strcmp(hm->from.dev, p->psel.dev) == 0 
         && strcmp(hm->from.mux, p->psel.mux) == 0 
         && strcmp(hm->from.sec, p->psel.sec) == 0 
         && strcmp(hm->from.typ, p->psel.typ) == 0 ) 
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

/* Print a inofrmation from an internal PDU */
void pdu_print(pdu *pdu) {
  if (pdu == NULL)  fprintf(stderr, "Cannot print NULL PDU\n");
  else              fprintf(stderr, "PDU dev=%s,mux=%s,sec=%s,typ=%s (len=%d): %s\n",
		  pdu->psel.dev, pdu->psel.mux, pdu->psel.sec, pdu->psel.typ, pdu->len, (char *) pdu->payload);
}

/* Free memory allocated for PDU */
void pdu_delete(pdu *pdu) {
  free(pdu->psel.dev);
  free(pdu->psel.mux);
  free(pdu->psel.sec);
  free(pdu->psel.typ);
  // free(pdu->payload); // XXX
  free(pdu);
}

/* Print haljson (assuming entire input is a string) */
void haljson_print(char *buf) {
  fprintf(stderr, "%s (len=%ld)='%s'\n", __func__, strlen(buf), buf);
}

/* Put data from buf (using haljson model) into internal HAL PDU */
/* XXX: QUick fix for phase 1 (use haljson structure, once defined) */
void haljson_parse_into_PDU (char *buf, const char *dev_id, pdu *p) {
  char        *tag, *tag2, *tag_next;
  char        *data;
  int          i;
    
  tag  = strtok(buf, HALJSON_DELIM_DATA);
  data = strtok(NULL, HALJSON_DELIM_DATA);
  if ( (tag==NULL) || (data==NULL) ) {
    fprintf(stderr, "Error in PDU arrive on device %s\n", dev_id);
    fprintf(stderr, "tag=%s data=%s\n", tag, data);
    exit (1);
  }
  tag_next  = strtok(tag, HALJSON_DELIM_TAGS);
  for (i=0; tag_next != NULL; i++) {
//      fprintf(stderr, "i=%d: tag_next=%s\n", i, tag_next);
    if (i==1) p->psel.mux = strdup(tag_next);
    if (i==2) p->psel.sec = strdup(tag_next);
    if (i==3) p->psel.typ = strdup(tag_next);
    if (i>HALJSON_MAX_TAGS) {
      fprintf(stderr, "Error, too many tag field (> %d) on device %s\n", HALJSON_MAX_TAGS, dev_id);
      exit (1);
    }
    tag_next = strtok(NULL, HALJSON_DELIM_TAGS);
  }
  if (i<HALJSON_MAX_TAGS) {
    fprintf(stderr, "Error, too few tag field (< %d) on device %s\n", HALJSON_MAX_TAGS, dev_id);
    exit (1);
  }
  p->payload = data;              //XXX:  copy buf to allow multiple reads in parallel?
  p->len = strlen(data);
}

/* Put data from internal PDU into buf (using haljson model) */
/* XXX: QUick fix for phase 1 (move to using haljson structure once defined) */
int haljson_parse_from_PDU (char *buf, const char *dev_id, pdu *p) {
  if(verbose) {fprintf(stderr, "%s for device %s: ", __func__, dev_id); pdu_print(p);}
  strcpy(buf, "tag");
  strcat(buf, HALJSON_DELIM_TAGS);
  strcat(buf, p->psel.mux);
  strcat(buf, HALJSON_DELIM_TAGS);
  strcat(buf, p->psel.sec);
  strcat(buf, HALJSON_DELIM_TAGS);
  strcat(buf, p->psel.typ);
  strcat(buf, HALJSON_DELIM_DATA);
  strcat(buf, p->payload);
//  haljson_print(buf);
  return (strlen(buf));
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
int haljson_to_bkend(char *s) {
  int d;
  
  /* XXX: may need diferent mappings for some sessions */
  d = s[(strlen(s)-1)] - '0';     /* get last digit of string */
//  fprintf(stderr, "Convert haljson string (%s) into bkend int (%d)\n", s, d);
  return (htonl(d));
}

/* Default Convert to haljson string from bkend integer (in network order) */
char *haljson_from_bkend(int d, char *prefix) {
  char r[12];
  char temp[12] = "";

  strcpy(temp, prefix);
  sprintf(r, "%d", ntohl(d));
  strcat(temp, r);
//  fprintf(stderr, "Convert into haljson string (%s) from bkend int (%d)\n", temp, ntohl(d));
  /* XXX Free memory when done */
  return (strdup(temp));
}


/* Put data into buf (using bkend model) from internal HAL PDU */
/* Returns length of buffer */
int bkend_parse_from_PDU (char *buf, const char *dev_id, pdu *p) {
  bkend     *b;
  bkend_tlv *tlv;
  char      *s;
  int       d;
  
  if(verbose) {fprintf(stderr, "%s for device %s: ", __func__, dev_id); pdu_print(p);}
  b = (bkend *) buf;
  b->session_tag = haljson_to_bkend(p->psel.mux);
  b->message_tag = haljson_to_bkend(p->psel.sec);
  b->message_tlv_count = htonl(1);
  for (int i=0; i < 1; i++) {
    tlv = &(b->tlv[i]);
    tlv->data_tag = haljson_to_bkend(p->psel.typ);
    tlv->data_len = htonl(p->len);
    strcpy((char *) tlv->data, (char *) p->payload);
  }
  /* XXX: Fix length to depend on message_tlv_count */
  return ((p->len) + BKEND_MIN_HEADER_BYTES);
}

/* Put data from buf (using bkend model) into internal HAL PDU */
void bkend_parse_into_PDU (char *buf, const char *dev_id, pdu *p, int len) {
  bkend     *b;
  bkend_tlv *tlv;
    
  if(verbose) fprintf(stderr, "Put bkend model data from dev=%s into internal HAL PDU\n", dev_id);
  
  /* XXX: Handle case where BKEND message is split - using framing? */
  if (len < BKEND_MIN_HEADER_BYTES) {
    fprintf(stderr, "XXX: BKEND Message split?: len=%d\n", len);
    exit (22);
  }
  
  p->psel.dev = strdup(dev_id);
  b = (bkend *) buf;
// fprintf(stderr, "HAL get input on %s: ", dev_id); bkend_print((bkend *) buf);
  p->psel.mux = haljson_from_bkend(b->session_tag, "app");
  p->psel.sec = haljson_from_bkend(b->message_tag, "m");
//  fprintf(stderr, "YYYY: count=%d\n", ntohl(b->message_tlv_count));
  for (int i=0; i < ntohl(b->message_tlv_count); i++) {
    tlv = &(b->tlv[0]);
    p->psel.typ = haljson_from_bkend(tlv->data_tag, "d");
    p->len = ntohl(tlv->data_len);
    p->payload = tlv->data;
  }
}

/* Read device and return pdu */
/* XXX: Use idev to determines how to parse, then extract selector info and fill psel */
pdu *read_pdu(device *idev) {
  pdu         *ret = NULL;
  static char  buf[MAXPDU];
  int          len;
  int          fd;
  const char  *dev_id;

  dev_id = idev->id;
  fd = idev->readfd;
  len = read(fd, buf, MAXPDU - 1);
  buf[len] = '\0';
  if(verbose) fprintf(stderr, "HAL read input on %s (fd=%d rv=%d len=%ld):\n%s\n", dev_id, fd, len, strlen(buf), buf);
  ret = malloc(sizeof(pdu));

//  fprintf(stderr, "extract mux, sec, typ and payload (using model %s) from data: %s", idev->model, buf);
  if (strcmp(idev->model, "haljson") == 0) {
    fprintf(stderr, "HAL reading from %s: ", dev_id); haljson_print(buf);
    haljson_parse_into_PDU (buf, dev_id, ret);
  } else if (strcmp(idev->model, "bkend") == 0) {
    bkend_parse_into_PDU   (buf, dev_id, ret, len);
    fprintf(stderr, "HAL reading from %s: ", dev_id); bkend_print((bkend *) buf);
  } else if (strcmp(idev->model, "notag") == 0) {
    ret->psel.mux = strdup("app1");  //XXX: fix
    ret->psel.sec = strdup("m1");    //XXX: fix
    ret->psel.typ = strdup("d1");    //XXX: fix
    ret->payload = buf;              //XXX: create copy of buf to allow multiple reads in parallel?
    ret->len = strlen(buf);
  } else {
    fprintf(stderr, "Error, unkwown device model: %s\n", idev->model);
    exit (22);
  }
  ret->psel.dev = strdup(dev_id);
  if(verbose) pdu_print(ret);
  return ret;
}

/* Write pdu to specified fd */
void write_pdu(device *odev, pdu *p, char *delim) {
  int          rv;
  int          fd;
  int          len;
  const char  *dev_id;
  const char  *dev_model;
  static char  buf[MAXPDU]={0};


  dev_id = odev->id;
  dev_model = odev->model;
  if (strcmp(dev_model, "haljson") == 0) {
    len = haljson_parse_from_PDU (buf, dev_id, p);
    fprintf(stderr, "HAL writing to %s: ", odev->id); haljson_print(buf);
  } else if (strcmp(dev_model, "bkend") == 0) {
    if(verbose) fprintf(stderr, "Convert internal PDU into %s model for device %s\n", dev_model, dev_id);
    len = bkend_parse_from_PDU (buf, dev_id, p);
    fprintf(stderr, "HAL writing to %s: ", odev->id); bkend_print((bkend *) buf);
//    fprintf(stderr, "XXX: for now just copy data\n");
//    strcpy(buf, p->payload);
  }
  fd = odev->writefd;
  rv = write(fd, buf, len);
  if(verbose) fprintf(stderr, "HAL wrote data on %s (fd=%d rv=%d len=%d): %s\n", dev_id, fd, rv, len, (char *) p->payload);
  if (strcmp(dev_id, "zc") == 0) {
    rv = write(fd, delim, strlen(delim));
    if(verbose) fprintf(stderr, "HAL wrote delimiter to %s (fd=%d rv=%d len=%ld): %s\n", dev_id, fd, rv, strlen(delim), delim);
  }
}

/* Process input from device (with 'input_fd') and send to output */
void process_input(int ifd, halmap *map, device *devs, char *delim) {
  pdu    *ipdu, *opdu;
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
  // halmap_print_one(h);

  /*
  opdu = codec(h, ipdu);
  if(opdu == NULL) { 
    fprintf(stderr, "Output PDU is NULL"); 
    pdu_delete(ipdu);
    return;
  }
   */
  // pdu_delete(ipdu);
  // pdu_print(opdu);
  
  odev = find_device_by_id(devs, h->to.dev);
  if(odev == NULL) { 
    fprintf(stderr, "Device not found for output");
    return; 
  }

  write_pdu(odev, ipdu, delim);
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


#define PARENT_READ  read_pipe[0]
#define PARENT_WRITE write_pipe[1]
#define CHILD_WRITE  read_pipe[1]
#define CHILD_READ   write_pipe[0]


int main(int argc, char **argv) {
  config_t  cfg;           /* Configuration */
  int       maxrfd;        /* Maximum file descriptor number for select */
  fd_set    readfds;       /* File descriptor set for select */
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
  
  read_config(argc, argv, &cfg);
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
  if(verbose) fprintf(stderr, "Pipe FDs: hal_r=%d hal_w=%d zc_sub_w=%d zc_pub_r=%d\n", PARENT_READ, PARENT_WRITE,  CHILD_WRITE, CHILD_READ);
    
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
  if(verbose) fprintf(stderr, "Spawned %s subscriber %d and publisher %d\n", zcpath, zcsubpid, zcpubpid);
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
