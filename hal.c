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

/* HAL Selector structure used in HAL map entries */
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

/* BKEND packet structure */
typedef struct _bkd {
  uint32_t  session_tag;            /* App Mux */
  uint32_t  message_tag;            /* Security */
  uint32_t  message_tlv_count;      /* TLV count (1 for demo) */
  uint32_t  tlv_data_tag;           /* Type (e.g., DATA_PAYLOAD_1) */
  uint32_t  tlv_data_payload_bytes; /* Length (in bytes) */
  uint8_t   tlv_data[236];          /* Value (up to 236 (256 - 5*4) bytes of payload) */
} bkend;


/* Maximum PDU size */
#define MAXPDU 4096
#define HALJSON_MAX_TAGS 3

#define HALJSON_DELIM_DATA " "
#define HALJSON_DELIM_TAGS "-"


/* Parse the configuration file */
void read_config (int argc, char **argv, config_t *cfg) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s path-to-config-file\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  config_init(cfg);
  if(! config_read_file(cfg, argv[1]))
  {
    fprintf(stderr, "%s:%d - %s\n", config_error_file(cfg), config_error_line(cfg), config_error_text(cfg));
    config_destroy(cfg);
    exit(EXIT_FAILURE);
  }
  fprintf(stderr, "Read configuration file: %s\n", argv[1]);
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

/* Loop through devs linked list, find device with fd as its read file descriptor */
device *find_device_for_readfd(device *root, int fd) {
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
    fprintf(stderr, "About to open device: %s %s\n", d->id, d->path);
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
        fprintf(stderr, "[%s ->", hm->from.dev);
        fprintf(stderr, " %s] ", hm->to.dev);
    }
    fprintf(stderr, "\n");
}

/* Return entry with from selector matching PDU selector from the halmap list */
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

void pdu_print(pdu *pdu) {
  fprintf(stderr, "PDU (dev=%s,mux=%s,sec=%s,typ=%s): %s\n", 
		  pdu->psel.dev, pdu->psel.mux, pdu->psel.sec, pdu->psel.typ, (char *) pdu->payload);
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

/* Put data from buf (using haljson model) into internal HAL PDU */
/* XXX: QUick fix for phase 1 (move to using haljson structure once defined) */
void parse_from_haljson_model (char *buf, const char *dev_id, pdu *p) {
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
  p->payload = data;              //XXX: create copy of buf to allow multiple reads in parallel?
  p->len = strlen(data);
}

/* Put data from internal PDU into buf (using haljson model) */
/* XXX: QUick fix for phase 1 (move to using haljson structure once defined) */
void parse_into_haljson_model (char *buf, const char *dev_id, pdu *p) {
  fprintf(stderr, "%s for device %s: ", __func__, dev_id); pdu_print(p);
  strcpy(buf, "tag");
  strcat(buf, HALJSON_DELIM_TAGS);
  strcat(buf, p->psel.mux);
  strcat(buf, HALJSON_DELIM_TAGS);
  strcat(buf, p->psel.sec);
  strcat(buf, HALJSON_DELIM_TAGS);
  strcat(buf, p->psel.typ);
  strcat(buf, HALJSON_DELIM_DATA);
  strcat(buf, p->payload);
  fprintf(stderr, "%s: buf(len=%ld)='%s'\n", __func__, strlen(buf), buf);
}

void bkend_print(bkend *b) {
  int i;
  fprintf(stderr, "%s: ", __func__);
  fprintf(stderr, "mux=%u ", b->session_tag);
  fprintf(stderr, "sec=%u ", b->message_tag);
  for (i=0; i < b->message_tlv_count; i++) {
    fprintf(stderr, "typ=%u ", b->tlv_data_tag);
    fprintf(stderr, ": (len=%ld) %s\n", strlen(b->tlv_data), b->tlv_data);
  }
}

int parse_into_bkend_model (char *buf, const char *dev_id, pdu *p) {
  bkend  *b;
  
  fprintf(stderr, "%s for device %s: ", __func__, dev_id); pdu_print(p);
  
  b = (bkend *) buf;
  /* XXX: FIXME (just assumes only one format for now) */
  b->session_tag = 1;
  b->message_tag = 2;
  b->message_tlv_count = 1;
  b->tlv_data_tag = 3;
  b->tlv_data_payload_bytes = p->len;
  strcpy(b->tlv_data, p->payload);
  bkend_print(b);
  return ((p->len) + 20);
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
  fprintf(stderr, "HAL read input on %s (fd=%d rv=%d len=%ld):\n%s\n", dev_id, fd, len, strlen(buf), buf);
  ret = malloc(sizeof(pdu));

  fprintf(stderr, "extract mux, sec, typ and payload (using model %s) from data: %s", idev->model, buf);
  if (strcmp(idev->model, "haljson") == 0) {
    parse_from_haljson_model (buf, dev_id, ret);
  } else if (strcmp(idev->model, "bkend") == 0) {
    //XXX: extract mux, sec, typ and payload from data read from device
    // parse_bkend(buf,len); // Must get mux, sec, typ, and payload out
    ret->psel.mux = strdup("app1");  //XXX: fix
    ret->psel.sec = strdup("m1");    //XXX: fix
    ret->psel.typ = strdup("d1");    //XXX: fix
    ret->payload = buf;              //XXX: create copy of buf to allow multiple reads in parallel?
    ret->len = strlen(buf);
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
pdu_print(ret);
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
    parse_into_haljson_model (buf, dev_id, p);
  } else if (strcmp(dev_model, "bkend") == 0) {
    fprintf(stderr, "XXX: Convert internal PDU into %s model for device %s\n", dev_model, dev_id);
    len = parse_into_bkend_model (buf, dev_id, p);
    bkend_print((bkend *) buf);
//    fprintf(stderr, "XXX: for now just copy data\n");
//    strcpy(buf, p->payload);
  }
  fd = odev->writefd;
  rv = write(fd, buf, len);
  fprintf(stderr, "HAL wrote data on %s (fd=%d rv=%d len=%d):\n%s\n", dev_id, fd, rv, len, (char *) p->payload);
  if (strcmp(dev_id, "zc") == 0) {
    rv = write(fd, delim, strlen(delim));
    fprintf(stderr, "HAL wrote delimiter on %s (fd=%d rv=%d len=%ld):\n%s\n", dev_id, fd, rv, strlen(delim), delim);
  }
}

/* Process input from device (with 'input_fd') and send to output */
void process_input(int ifd, halmap *map, device *devs, char *delim) {
  pdu    *ipdu, *opdu;
  device *idev, *odev;
  halmap *h;
  
  idev = find_device_for_readfd(devs, ifd);
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
  
  opdu = codec(h, ipdu);
  if(opdu == NULL) { 
    fprintf(stderr, "Output PDU is NULL"); 
    pdu_delete(ipdu);
    return;
  }
  // pdu_delete(ipdu);
  // pdu_print(opdu);
  
  odev = find_device_by_id(devs, h->to.dev);
  if(odev == NULL) { 
    fprintf(stderr, "Device not found for output");
    return; 
  }

  write_pdu(odev, opdu, delim);
  pdu_delete(opdu);
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
  fprintf(stderr, "Pipe FDs: hal_r=%d hal_w=%d zc_sub_w=%d zc_pub_r=%d\n", PARENT_READ, PARENT_WRITE,  CHILD_WRITE, CHILD_READ);
    
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

  fprintf(stderr, "zcpath=%s\n", zcpath);
  fprintf(stderr, "zcpub=%s\n",  zcpub);
  fprintf(stderr, "zcsub=%s\n",  zcsub);
  fprintf(stderr, "delim=%s\n",  delim);
  fprintf(stderr, "Spawned %s subscriber %d and publisher %d\n", zcpath, zcsubpid, zcpubpid);
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
