/* Open, Find and Print Devices (based on config file info) */
/*   Currently, it can open three types of devices:
 *     a) Serial Device (tty)
 *     b) INET device (tcp or udp)
 *     c) IPC device (ZMQ pub/sub)
 */

#include "hal.h"

#define PARENT_READ  read_pipe[0]
#define PARENT_WRITE write_pipe[1]
#define CHILD_WRITE  read_pipe[1]
#define CHILD_READ   write_pipe[0]

typedef struct _root_device {
  const char   *root_path;
  int           data_dev_count;
  device       *data_dev_list[10];
} root_device;

void device_print_str(char *name, const char *s) {
  if (strlen(s) > 0) fprintf(stderr, " %s=%s", name, s);
}

void device_print_int(char *name, int v) {
  if (v >= 0) fprintf(stderr, " %s=%d", name, v);
}

void devices_print_one(device *d)  {
  fprintf(stderr, " %s [v=%d d=%s m=%s c=%s", d->id, d->enabled, d->path,  d->model, d->comms);
  device_print_str("ai", d->addr_in);
  device_print_str("ao", d->addr_out);
  device_print_str("mi", d->mode_in);
  device_print_str("mo", d->mode_out);
  device_print_int("pi", d->port_in);
  device_print_int("po", d->port_out);
  device_print_int("fr", d->readfd);
  device_print_int("fw", d->writefd);
  device_print_str("dr", d->path_r);
  device_print_str("dw", d->path_w);
  device_print_int("mx", d->from_mux);
  fprintf(stderr, "]\n");
}
  
/* Print list of devices for debugging */
void devices_print_all(device *root)  {
  fprintf(stderr, "HAL device list:\n");
  for(device *d = root; d != NULL; d = d->next) devices_print_one(d);
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
    // fprintf(stderr, "Spawned %s subscriber pid=%d\n", d->path, pid);
  }
}

/* Open IPC interfaces (specified in d) */
void interface_open_ipc(device *d) {
  int  read_pipe[2];    /* Read  pipes for parent-child (HAL-API) communication */
  int  write_pipe[2];   /* Write pipes for parent-child (HAL-API) communication */

  /* a) Open communication pipes for IPC reading and writing */
// fprintf(stderr, "Open IPC %s %s\n", d->id, d->path);
  if(pipe(read_pipe) < 0 || pipe(write_pipe) < 0) {
    fprintf(stderr, "Pipe creation failed\n");
    exit(EXIT_FAILURE);
  }
//  fprintf(stderr, "Pipe FDs: hal_r=%d hal_w=%d zc_sub_w=%d zc_pub_r=%d\n", PARENT_READ, PARENT_WRITE,  CHILD_WRITE, CHILD_READ);

//  fprintf(stderr, "UUUUUUUUUUU %s %s %s %s\n", d->addr_in, HAL_IPC_SUB, d->addr_out, HAL_IPC_PUB);
  /* b) Fork HAL child processes (to read and write on ipc channels) */
  start_api_process(d, read_pipe, write_pipe, CHILD_WRITE, CHILD_READ,  STDOUT_FILENO, d->mode_in,  HAL_IPC_SUB);
  start_api_process(d, read_pipe, write_pipe, CHILD_READ,  CHILD_WRITE, STDIN_FILENO,  d->mode_out, HAL_IPC_PUB);

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

  // c1) Bind the socket with the HAL server address
  if (bind_flag == 1) {
    if ( bind(fd, (const struct sockaddr *) serv_addr, sizeof(*serv_addr)) < 0 )
    {
      perror("\nbind failed");
      exit(EXIT_FAILURE);
    }
  }
  /* c2) Connect the socket (if not UDP) */
  else {
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

/* Open a serial (tty) interface for read-write and save the fds */
void interface_open_tty(device *d) {
  int fd;
  if ((fd = open(d->path, O_RDWR)) < 0) {
    fprintf(stderr, "Error opening device %s: %s\n", d->id, d->path);
    exit(EXIT_FAILURE);
  }
  d->readfd = fd;
  d->writefd = fd;
}

/* open read and write ILIP device pair */
void ilp_open_read_writes(device *d) {
  int fd_read, fd_write;

//fprintf(stderr, "TODO %s: Create data Devices external to HAL: %s %s\n", __func__, d->path_r, d->path_w);
//return;

  if ((fd_read  = open(d->path_r, O_RDONLY, S_IRUSR)) < 0) {
    fprintf(stderr, "Error opening device %s: %s\n", d->id, d->path_r);
    exit(EXIT_FAILURE);
  }
  if ((fd_write = open(d->path_w, O_RDONLY, S_IRUSR)) < 0) {
    fprintf(stderr, "Error opening device %s: %s\n", d->id, d->path_w);
    exit(EXIT_FAILURE);
  }
  d->readfd  = fd_read;
  d->writefd = fd_write;
}

/* Check ILIP root device */
void ilp_root_check(int fd_root, root_device *rd) {
  int      i, j, device_pair_count;
  ssize_t  write_bytes, read_bytes;
  uint32_t application_id[2] = { 1, 2 };
  uint32_t session_id[2] = {0xffffffff, 0xffffffff};

  /* Take application_id from HAL config */
  device_pair_count = rd->data_dev_count;
  for (j=0; j<device_pair_count; j++) application_id[j] = rd->data_dev_list[j]->from_mux;
  
  /* For this test and this set of devices application ID is equal to session ID */
  for ( i=0; i<2; i++ ) {
    write_bytes = write(fd_root, (const void *)&application_id[i], sizeof(application_id[i]));
    if ( write_bytes != sizeof(session_id[i])) {
      perror( "Error in the write of the application ID");
      exit(EXIT_FAILURE);
    }
    read_bytes = read( fd_root, (void*)&session_id[i], sizeof(session_id[i]) );
    if ( read_bytes != sizeof(session_id[i])) {
      perror( "Error in the read of the session ID");
      exit(EXIT_FAILURE);
    }
    
fprintf(stderr, "TODO %s: Write Session IDs from Root Device into halmap: from_mux=%u to_mux=%u\n", __func__, application_id[i], session_id[i]);
    
    if (session_id[i] != application_id[i]) {
      fprintf( stderr, "Invalid session ID returned Application: %u, Session ID: %u\n", application_id[i], session_id[i] );
      exit(EXIT_FAILURE);
    }
  }
}

/* Open root ILIP device */
void ilp_root_open(root_device *rd) {
  const char *root_path = rd->root_path;
  int  fd_root = -1;
  
  if (root_path != NULL) {
    if ((fd_root = open(root_path, O_RDWR, S_IRUSR | S_IWUSR)) < 0) {
      fprintf(stderr, "%s: Error opening device %s: fd=%d\n", __func__, root_path, fd_root);
      exit(EXIT_FAILURE);
    }
//    fprintf(stderr, "%s: successfully opened root device %s (fd=%d)\n", __func__, root_path, fd_root);
    ilp_root_check(fd_root, rd);
    close(fd_root);
  }
}

/* save root paths (and the associated devices) in root_device structs */
void ilp_add2root(device *d, int *root_count, root_device *root_list) {
  int i, rc, dc;
  const char *path=d->path;
  root_device *rdp = NULL;

  /* a) Find if matches previous root device */
  rc = *root_count;
//  fprintf(stderr, "%s: dev=%s %s rc=%d\n", __func__, d->id, d->comms, rc);
  for (i=0; i<rc; i++) {
    rdp = &(root_list[i]);
    if (strcmp((char *) (rdp->root_path), path) == 0)  break;
  }
  
  /* b) If no match found, wrtie new root device */
  if (i == rc) {
    rdp = &(root_list[rc]);
    rdp->data_dev_count = 0;
    rdp->root_path = path;
    (*root_count)++;
  }

  /* c) Add device to (new or existing) entry pointed to by root_device struct */
  dc = rdp->data_dev_count;
  (rdp->data_dev_list)[dc] = d;
  (rdp->data_dev_count)++;
  
//  fprintf(stderr, "%s: New device (%s) added (root path = %s, dev count = %d, root count = %d)\n", __func__, d->id, path, rdp->data_dev_count, *root_count);
}

/* Open ILIP interface */
void interface_open_ilp(device *d) {
  static int          root_count=0;
  static root_device  root_list[8], *rd;
  int                 i, j;
  
  if (d != NULL) {   /* adding new devces */
//    fprintf(stderr, "%s: dev=%s %s\n", __func__, d->id, d->comms);
    ilp_add2root(d, &root_count, root_list);
    return;
  }
//  fprintf(stderr, "%s: Open %d root device(s)\n", __func__, root_count);
  for (j=0; j<root_count; j++) {
    rd = &(root_list[j]);
    ilp_root_open(rd);
    for (i=0; i<(rd->data_dev_count); i++)
      ilp_open_read_writes(rd->data_dev_list[i]);
  }
}

/* Open enabled devices (from linked-list of devices) and get their in/out handles */
void devices_open(device *dev_linked_list_root) {
  for(device *d = dev_linked_list_root; d != NULL; d = d->next) {
    if (d->enabled == 0) continue;
    if        (!strncmp(d->comms, "tty", 3))                                      interface_open_tty(d);
    else if ( (!strncmp(d->comms, "udp", 3)) || (!strncmp(d->comms, "tcp", 3)) )  interface_open_inet(d);
    else if   (!strncmp(d->comms, "ipc", 3))                                      interface_open_ipc(d);
    else if   (!strncmp(d->comms, "ilp", 3))                                      interface_open_ilp(d);
    else { fprintf(stderr, "Device %s [%s] unknown\n", d->id, d->path); exit(EXIT_FAILURE);}
// fprintf(stderr, "Open succeeded for %s [%s] (with fdr=%d fdw=%d)\n", d->id, d->path, d->readfd, d->writefd);
  }
  interface_open_ilp(NULL);
}
