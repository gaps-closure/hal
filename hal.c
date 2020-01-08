#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>	
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libconfig.h>

typedef struct _dev {
  int enabled;
  int readfd;
  int writefd;
  const char *id;
  const char *path;
  const char *model;
  struct _dev *next;
} device;

typedef struct _sel {
  char *dev;
  char *mux;
  char *sec;
  char *typ;
} selector;

typedef struct _hal {
  selector    from;
  selector    to;
  char        *codec;
  struct _hal *next;
} halmap;

typedef struct _pdu {
  int pdutype;
  void *payload;
  int len;
} pdu;

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

char *get_zcpath(config_t *cfg) {
  const char *zcpath;
  if(config_lookup_string(cfg, "zcpath", &zcpath)) return strdup(zcpath);
  fprintf(stderr, "No 'zcpath' setting in configuration file.\n");
  exit(EXIT_FAILURE);
}

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

/* Construct selectors and halmap linked list from config */
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
  for(device *d = root; d != NULL && d->enabled != 0; d = d->next) {
    // fprintf(stderr, "fd=%d: dev=%s rfd=%d wfd=%d\n", fd, d->id, d->readfd, d->writefd);
    if (d->readfd == fd)  return (d);
  }
  return ((device *) NULL);
}

/* Loop through devs linked list, find entry matching "id", get its writefd and return */
int find_writefd_for_devid(device *root, char *id) {
  for(device *d = root; d != NULL && d->enabled != 0; d = d->next) {
    // fprintf(stderr, "list=%s find=%s\n", d->id, id);
    if (strcmp(d->id, id) == 0)  return (d->writefd);
  }
  return (-1);
}

/* Open enabled devices (from linked-list of devices) and get their in/out handles */
void devices_open(device *dev_linked_list_root) {
  device   *d;             /* Temporary device pointer */
  int       fd;

  for(d = dev_linked_list_root; d != NULL; d = d->next) {
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

/* Given the fromfd, determine input dev from devs */
/* XXX: Given fromfd and pdu, determine from selector <dev,sec,mux,typ> */
/* XXX: Return entry matching the from selector in the halmap list */
halmap *halmap_find(int fromfd, pdu *p, device *dev_root, halmap *map_root) {
  halmap * ret = NULL;
  device *d;

  d = find_device_for_readfd(dev_root, fromfd);
  // fprintf(stderr, "Input fd=%d is from device %s\n", fromfd, d->id);
  for(halmap *hm = map_root; hm != NULL; hm = hm->next) {
    // fprintf(stderr, "halmap_find: %s %s\n", hm->from.dev, d->id);
    // XXX: should get mux,sec,typ from PDU and match entire tuple, not just dev
    if ( strcmp(hm->from.dev, d->id) == 0 ) return (hm);
    /*
    if ( strcmp(hm->from.dev, d->id) == 0 
         && strcmp(hm->from.mux, p->mux) == 0 
         && strcmp(hm->from.sec, p->sec) == 0 
         && strcmp(hm->from.typ, p->typ) == 0 ) 
      return (hm);
    */
  }
  return ret;
}

/* From halmap entry, get to selector, get device id, get writefd for device id */
int get_writefd_from_mapentry(halmap *mapentry, device *devs) {
    char *odev;
    int ofd=-1;
    
    odev = mapentry->to.dev;
    ofd = find_writefd_for_devid(devs, odev);
    fprintf(stderr, "odev=%s ofd=%d\n", odev, ofd);
    return (ofd);
}

/* XXX: Determine codec from halmap entry, apply and return transformed pdu */
pdu *codec(halmap *mapentry, pdu *ipdu) {
  pdu *opdu = NULL;

  // XXX: must apply appropriate transformation based on mapentry
  // XXX: Doing null transform for now
  opdu = ipdu;
  return opdu;
}

void pdu_print(pdu *pdu) {
  fprintf(stderr, "PDU (type=%d): %s\n", pdu->pdutype, (char *) pdu->payload);
}

void pdu_delete(pdu *pdu) {
  free(pdu);
}

/* Read and return pdu from fd */
/* XXX: Do we need info about device? */
pdu *read_pdu(int fd) {
  pdu         *ret = NULL;
  static char  buf[100];
  int          len;
    
  memset(buf,'\0',100);
  len = read(fd, buf, 100);
  fprintf(stderr, "HAL read input on fd=%d rv=%d (len=%ld):\n%s\n", fd, len, strlen(buf), buf);
  ret = malloc(sizeof(pdu));
  ret->pdutype = 0;
  ret->payload = buf;
  ret->len = strlen(buf);
  return ret;
}

/* Determine the write fd from the halmap entry, then write pdu */
void write_pdu(int fd, pdu *p) {
  int   rv;

  char *delim="EOF\0"; // XXX: should come from config file
  rv = write(fd, p->payload, p->len);
  fprintf(stderr, "HAL wrote data on fd=%d rv=%d (len=%d):\n%s\n", fd, rv, p->len, (char *) p->payload);
  rv = write(fd, delim, strlen(delim));
  fprintf(stderr, "HAL wrote delimiter on fd=%d rv=%d (len=%ld):\n%s\n", fd, rv, strlen(delim), delim);
}

/* Process input from device (with 'input_fd') and send to output */
void process_input(int ifd, halmap *map, device *devs) {
  pdu    *ipdu, *opdu;
  halmap *h;
  int    ofd;
    
  ipdu = read_pdu(ifd);
  if(ipdu == NULL) { 
    fprintf(stderr, "Input PDU is NULL"); 
    return; 
  }

  h = halmap_find(ifd, ipdu, devs, map);
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
  // pdu_print(opdu);
  
  ofd  = get_writefd_from_mapentry(h, devs);
  // fprintf(stderr, "ofd=%d\n", ofd);
  
  write_pdu(ofd, opdu);
  // pdu_delete(ipdu);
  pdu_delete(opdu);
}

/* Iniitialize file descriptor set for select (from linked-list of devices) */
int select_init(device *dev_linked_list_root, fd_set *readfds) {
  device   *d;             /* Temporary device pointer */
  int       maxrfd;        /* Maximum file descriptor number for select */

  FD_ZERO(readfds);
  maxrfd = -1;
  for(d = dev_linked_list_root; d != NULL && d->enabled != 0; d = d->next) {
    if (d->readfd >= maxrfd) maxrfd = d->readfd + 1;
    FD_SET(d->readfd, readfds);
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
  device    zcroot;        /* Fake device for zc */
  device   *devs;          /* Linked list of enabled devices */
  halmap   *map;           /* Linked list of selector mappings */
  int       pid;           /* Process ID of child or parent from fork call */
  int       read_pipe[2];  /* Read pipes for parent-child communication */
  int       write_pipe[2]; /* Write pipes for parent-child communication */
  int       zcsubpid;      /* Subscriber PID */
  int       zcpubpid;      /* Publisher PID */

  read_config(argc, argv, &cfg);
  devs   = get_devices(&cfg);
  zcpath = get_zcpath(&cfg);
  map    = get_mappings(&cfg);
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
    /* XXX: Arguments should come from config file */
    // char *argv2[] = {zcpath, "-b", "-v", "sub", "ipc://halsub", NULL};
    char *argv2[] = {zcpath, "-b", "sub", "ipc://halsub", NULL};
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
    /* XXX: Arguments should come from config file */
    // char *argv2[] = {zcpath, "-b", "-d", "EOF", "-v", "pub", "ipc://halpub", NULL};
    char *argv2[] = {zcpath, "-b", "-d", "EOF", "pub", "ipc://halpub", NULL};
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
  zcroot.model   = "NONE";
  zcroot.next    = devs;
  zcroot.readfd  = PARENT_READ;
  zcroot.writefd = PARENT_WRITE;

  devices_open(devs);

  fprintf(stderr, "zcpath=%s\n", zcpath);
  fprintf(stderr, "Spawned %s subscriber %d and publisher %d\n", zcpath, zcsubpid, zcpubpid);
  devices_print_all(&zcroot);
  halmap_print_all(map);

  /* Select across readfds */
  while (1) {
    int nready; 
          
    maxrfd = select_init(&zcroot,  &readfds);
    // fprintf(stderr, "Waiting for input on fds (max+1=%d)\n", maxrfd);
    if((nready = select(maxrfd, &readfds, NULL, NULL, NULL)) == -1) perror("select()");
    // fprintf(stderr, "Selected n=%d max=%d\n", nready, maxrfd);
    for (int i = 0; i < maxrfd && nready > 0; i++) {
      if (FD_ISSET(i, &readfds)) {
        process_input(i, map, &zcroot);
        nready--;
      }
    }
  }

  /* XXX: Log to specified logfile, not stderr */
  /* XXX: Properly daemonize, close standard fds, trap signals etc. */
  /* XXX: Deal with frag/defrag and other functionality etc. */

  return 0;
}
