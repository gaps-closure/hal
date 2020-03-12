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

/* Open a serial (tty) interface for read-write and return its fd */
void interface_open_tty(device *d) {
  int fd;
  if ((fd = open(d->path, O_RDWR)) < 0) {
    fprintf(stderr, "Error opening device %s: %s\n", d->id, d->path);
    exit(EXIT_FAILURE);
  }
  d->readfd = fd;
  d->writefd = fd;
}

/* Open enabled devices (from linked-list of devices) and get their in/out handles */
void devices_open(device *dev_linked_list_root) {
  for(device *d = dev_linked_list_root; d != NULL; d = d->next) {
    if (d->enabled == 0) continue;
    if        (!strncmp(d->comms, "tty", 3))                                      interface_open_tty(d);
    else if ( (!strncmp(d->comms, "udp", 3)) || (!strncmp(d->comms, "tcp", 3)) )  interface_open_inet(d);
    else if   (!strncmp(d->comms, "ipc", 3))                                      interface_open_ipc(d);
    else { fprintf(stderr, "Device %s [%s] unknown\n", d->id, d->path); exit(EXIT_FAILURE);}
    // fprintf(stderr, "Open succeeded for %s [%s] (with fdr=%d fdw=%d)\n", d->id, d->path, d->readfd, d->writefd);
  }
}
