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
  selector from;
  selector to;
  struct _hal *next;
} halmap;

typedef struct _pdu {
  int pdutype;
  void *payload;
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
           && config_setting_lookup_int(dev, "enabled", &enabled))) {
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

/* XXX: construct selectors and halmap linked list from config */
halmap *get_mappings(config_t *cfg) {
  halmap *ret = NULL;
  return ret;
}

/* XXX: Given the fromfd, determine input dev from devs */
/* XXX: Given fromfd and pdu, determine from selector <dev,sec,mux,typ> */
/* XXX: Return entry matching the from selector in the halmap list */
halmap *find_halmap_entry(int fromfd, pdu *p, device *devs, halmap *map) {
  halmap * ret = NULL;
  return ret;
}

/* XXX: Determine codec from halmap entry, apply and return transformed pdu */
pdu *codec(halmap *mapentry, pdu *inpdu, char *buf) {
  pdu *ret = NULL;
    
  buf[0] = 'X';
  return ret;
}

/* XXX: Read and return pdu from fd, do we need more info about device? */
pdu *read_pdu(int fd, char *buf, int *len) {
  pdu  *ret = NULL;

  memset(buf,'\0',100);
  *len = read(fd, buf, 100);
  fprintf(stderr, "HAL reads input on fd=%d rv=%d (len=%ld):\n%s\n", fd, *len, strlen(buf), buf);
  return ret;
}

/* XXX: Determine the write fd from the halmap entry, then write pdu */
void write_pdu(halmap *mapentry, pdu *p, char *buf, int fd) {
  int   rv;

  //  char out[40];
  //  rv = write(fd, out, strlen(buf));
    
  char* line = "bbbbbbbbbbbbbbbbbbbbbbbb\nEOF\n\0";
  rv = write(fd, line, strlen(line));
  fprintf(stderr, "HAL writes on fd=%d rv=%d (len=%ld):\n%s\n", fd, rv, strlen(line), line);
  return;
}


void print_all_devices(device *root)  {
    device   *d;             /* Temporary pointer */

    for(d = root; d != NULL && d->enabled != 0; d = d->next) {
        fprintf(stderr, "dev=%s rfd=%d wfd=%d\n", d->id, d->readfd, d->writefd);
    }
}

/* Open enabled devices (from linked-list of devices) and get their in/out handles */
void real_devices_open(device *dev_linked_list_root) {
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
  
/* Iniitialize File descriptor set for select (from linked-list of devices) */
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

/* Process input from device (with 'input_fd') and send to output */
void process_input(int input_fd, halmap *map, device *devs, int output_fd) {
  pdu    *inp, *outp;
  halmap *h;
  char    buf[100];
  int     len;
    
  inp = read_pdu(input_fd, buf, &len);
  h = find_halmap_entry(input_fd, inp, devs, map);
  outp = codec(h, inp, buf);
  write_pdu(h, outp, buf, output_fd);
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
  zcpath = get_zcpath(&cfg);
  //zcpath = "zc";
  devs   = get_devices(&cfg);
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
    char *argv2[] = {zcpath, "-b", "-v", "sub", "ipc://halsub", NULL};
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
    // char *argv2[] = {zcpath, "-b", "-v", "pub", "ipc://halpub", NULL};
    char *argv2[] = {zcpath, "-b", "-d", "EOF", "-v", "pub", "ipc://halpub", NULL};
    if(execvp(argv2[0], argv2) < 0) perror("execvp()");
    exit(EXIT_FAILURE);
  } else { /* save publisher child pid in parent */
    zcpubpid = pid;
  }

  fprintf(stderr, "Spawned %s subscriber %d and publisher %d\n", zcpath, zcsubpid, zcpubpid);

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

  real_devices_open(devs);
  print_all_devices(&zcroot);

  /* Select across readfds */
  while (1) {
    int nready; 
          
    maxrfd = select_init(&zcroot,  &readfds);
    fprintf(stderr, "Waiting for input on fds (max+1=%d)\n", maxrfd);
    if((nready = select(maxrfd, &readfds, NULL, NULL, NULL)) == -1) perror("select()");
//    fprintf(stderr, "select detects n=%d max=%d\n", nready, maxrfd);
    for (int i = 0; i < maxrfd && nready > 0; i++) {
      if (FD_ISSET(i, &readfds)) {
        process_input(i, map, devs, PARENT_WRITE);
        nready--;
      }
    }
  }

  /* XXX: Log to specified logfile, not stderr */
  /* XXX: Properly daemonize, close standard fds, trap signals etc. */
  /* XXX: Deal with frag/defrag and other functionality etc. */

  return 0;
}


/*
  TONY TESTING FUNCTIONS

 char str[100];
 fprintf(stderr, "Waiting to Read from (fd=%d)\n", PARENT_READ);
 nready = read(PARENT_READ, str, 100);
 printf("Read string %s (ret=%d)\n", str, nready);

  char* line = "bbbbbbbbbbbbbbbbbbbbbbbb\n";
 write(CHILD_WRITE, line, strlen(line));
 char *argv2[] = {"ls", "-l", NULL};

 */
