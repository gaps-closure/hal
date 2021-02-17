/*
 * HAL device Open, Find and Print
 *   December 2020, Perspecta Labs
 *
 *     a) IPC to application (unix pipe and ZMQ process)
 *     b) INET device (tcp or udp)
 *     c) Bidirectional Serial Device (tty)
 *     d) Unidirectional Serial Device (ilp)
 */

#include "hal.h"

/* Define IPC parent-child process file descriptors */
#define PARENT_IN  pipe_a2h[0]
#define PARENT_OUT pipe_h2a[1]
#define CHILD_OUT  pipe_a2h[1]
#define CHILD_IN   pipe_h2a[0]
#define PIPE_BUFFER_CAP  655360

/* Store information on ILP devices associated with serial root device */
#define ILP_MAX_DEVICES_PER_ROOT 16
#define ILP_MAX_ROOT_DEVICES  8
typedef struct _root_device {
  const char   *root_path;
  int           data_dev_count;
  device       *data_dev_list[ILP_MAX_DEVICES_PER_ROOT];
} root_device;

/**********************************************************************/
/* Print Device Deffinition */
/*********t************************************************************/
/* Print device definition element (if it exists) */
void device_print_str(FILE *fd, char *name, const char *s) {
  if (strlen(s) > 0) fprintf(fd, " %s=%s", name, s);
}

/* Print device definition element (if it exists) */
void device_print_int(FILE *fd, char *name, int v) {
  if (v >= 0) fprintf(fd, " %s=%d", name, v);
}

/* Print device definition */
void devices_print_one(device *d, FILE *fd)  {
  if (fd == NULL) return;
  fprintf(fd, "   %s [v=%d d=%s m=%s c=%s", d->id, d->enabled, d->path,  d->model, d->comms);
  device_print_int(fd, "ie", d->init_enable);
  device_print_str(fd, "ai", d->addr_in);
  device_print_str(fd, "ao", d->addr_out);
  device_print_str(fd, "mi", d->mode_in);
  device_print_str(fd, "mo", d->mode_out);
  device_print_int(fd, "pi", d->port_in);
  device_print_int(fd, "po", d->port_out);
  device_print_int(fd, "fi", d->readfd);
  device_print_int(fd, "fo", d->writefd);
  device_print_str(fd, "di", d->path_r);
  device_print_str(fd, "do", d->path_w);
  device_print_int(fd, "Pi", d->pid_in);
  device_print_int(fd, "Po", d->pid_out);
  device_print_int(fd, "mx", d->from_mux);
  fprintf(fd, " ci=%d co=%d", d->count_r, d->count_w);
  fprintf(fd, "]\n");
}
  
/* Print list of devices for debugging */
void log_log_devs(int level, device *root, const char *fn)  {
  FILE *fd[2];
  int   i;
  
  log_get_fds(level, &fd[0], &fd[1]);
  for (i=0; i<2; i++) {
//    fprintf(stderr, "XXX fd=%p\n", fd[i]);
    if (fd[i] != NULL) {
      fprintf(fd[i], "  HAL device list (from %s):\n", fn);
      for(device *d = root; d != NULL; d = d->next) devices_print_one(d, fd[i]);
      fflush (fd[i]);
    }
  }
}

/**********************************************************************/
/* Match from Device List  */
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

/**********************************************************************/
/* Open Device; a) IPC  */
/*********t************************************************************/
/* Start child process (currently zcat) to communicate between APP and HAL */
void ipc_child_run(int *pipe_a2h, int *pipe_h2a, const char *path, const char *mode, const char *addr) {
  
//    log_trace("HAL-ZMQ-API process in mode=%s (dup_dupe=%d=%d)", mode, child_fd_duped, STDOUT_FILENO);
  if (strcmp(mode, "pub") == 0) {
    close(CHILD_OUT);
    dup2(CHILD_IN,  STDIN_FILENO);        /* copy file descriptor, so both point to pipe with HAL  */
  }
  if (strcmp(mode, "sub") == 0) {
    close(CHILD_IN);
    dup2(CHILD_OUT, STDOUT_FILENO);        /* copy file descriptor, so both point to pipe with HAL  */
  }
  if ((strcmp(mode, "req") == 0) ||
      (strcmp(mode, "rep") == 0)) {
    dup2(CHILD_IN,  STDIN_FILENO);        /* copy file descriptor, so both point to pipe with HAL  */
    dup2(CHILD_OUT, STDOUT_FILENO);        /* copy file descriptor, so both point to pipe with HAL  */
  }
//  char *argv2[] = {(char *) path, "-b", "-v", (char *) mode, (char *) addr, NULL};   /* unix command */
  char *argv2[] = {(char *) path, "-b", (char *) mode, (char *) addr, NULL};   /* unix command */
  if(execvp(argv2[0], argv2) < 0) perror("execvp()");
}
  
/* Open HAL child process to read or write using the Application API (using zcat) */
int ipc_open_process(int *pipe_a2h, int *pipe_h2a, const char *path, const char *mode, const char *addr) {
  int  pid=-1;
  
  if( access(path, X_OK ) == -1 ) {
    log_fatal("HAL Exiting: ZCAT path (%s) from config file is not executable", path);
    exit(EXIT_FAILURE);
  }
  if( (access(strstr(addr, "//"), F_OK ) == 0) && (access(strstr(addr, "//"), W_OK ) == -1) ) {
    log_fatal("HAL Exiting: Cannot write to IPC address %s", addr);
    exit(EXIT_FAILURE);
  }

  if (strlen(addr) > 0) {
    if ((pid = fork()) < 0) {
      log_fatal("Fork failed");
      exit(EXIT_FAILURE);
    } else if (pid == 0) {  /* child (API) process starts here */
      close(PARENT_IN);
      close(PARENT_OUT);
      ipc_child_run(pipe_a2h, pipe_h2a, path, mode, addr);
      exit(EXIT_FAILURE);                                      /* child process should not reach here */
    } else {                /* parent (HAL) process */
      log_trace("HAL spawned %s HAL-ZMQ-API process with pid=%d", path, pid);
    }
  }
  else {
    log_trace("Not forking HAL-ZMQ-API process: addr=%s mode=%s", addr, mode);
  }
  return(pid);
}

/* Set the pipe capacity */
static void pipe_cap_set(int fd, long sz) {
  int ret;
  ret = fcntl(fd, F_SETPIPE_SZ, sz);
  if (ret < 0) {
    log_fatal("Pipe capacity could not be set");
    exit(EXIT_FAILURE);
  }
}

/* Open IPC interfaces (on device specified in d structure) for app to hal (a2h) and/or hal to app (h2a) communication
 *   For each (_in or _out) address specified in the configuration, HQL creates one processs, with:
 *      a) 1 pipe (unidirectional comms) for PUB/SUB
 *      b) 2 pipes (bidirectional comms) for REQ/REP
 *   If both addresses specified, HAL creates two processs, with 2 pipes for PUB/SUB and 4 for pipesREQ/REP
 */
void interface_open_ipc(device *d) {
  int  pipe_a2h[2];    /* Pipe from HAL-ZMQ-APP process (CHILD) into HAL (PAREBT) */
  int  pipe_h2a[2];    /* Parent-child pipe from HAL (PAREBT) into HAL-ZMQ-APP process (CHILD) */

  /* a) Open  communication pipe for IPC with each of the HAL-ZMQ-API processes  */
  if(pipe(pipe_a2h) < 0 || pipe(pipe_h2a) < 0) {
    log_fatal("Pipe creation failed");
    exit(EXIT_FAILURE);
  }

  pipe_cap_set(pipe_a2h[0],PIPE_BUFFER_CAP);
  pipe_cap_set(pipe_a2h[1],PIPE_BUFFER_CAP);
  pipe_cap_set(pipe_h2a[0],PIPE_BUFFER_CAP);
  pipe_cap_set(pipe_h2a[1],PIPE_BUFFER_CAP);

  /* b) Fork HAL-ZMQ-API child processes (using ipc communication pipes with HAL) */
  log_trace("Openning API i=[%s:%s] o=[%s:%s]", d->addr_in, d->mode_in, d->addr_out, d->mode_out);
  d->pid_in  = ipc_open_process(pipe_a2h, pipe_h2a, d->path, d->mode_in,  d->addr_in);  /* HAL-ZMQ-API process sends its STDOUT to HAL using pipe_a2h */
  d->pid_out = ipc_open_process(pipe_a2h, pipe_h2a, d->path, d->mode_out, d->addr_out);  /* HAL-ZMQ-API process gets its STDIN from HAL using pipe_h2a */

  /* c) Parent (HAL) process finishes up */
  close(CHILD_IN);
  close(CHILD_OUT);
  
  if (strcmp(d->mode_in,  "req") ||
      strcmp(d->mode_in,  "rep") ||
      strcmp(d->mode_out, "req") ||
      strcmp(d->mode_out, "rep")) {
    d->readfd  = PARENT_IN;
    d->writefd = PARENT_OUT;
  }
  else
  {
    if (d->pid_in  > 0) d->readfd  = PARENT_IN;
    if (d->pid_out > 0) d->writefd = PARENT_OUT;
  }
  log_trace("Starred HAL-ZMQ-API process(es): in=[pid=%d:fd=%d] out=[pid=%d:fd=%d]", d->pid_in, d->readfd, d->pid_out, d->writefd);
}

/**********************************************************************/
/* Open Device; b) INET (TCP or UDP)  */
/*********t************************************************************/
/* Open network socket and return its fd: optionally bind (bind_flag=1) and connect (bind_flag=0 & tcp) */
int inet_open_socket(device *d, const char *addr, int port, struct sockaddr_in *serv_addr, int bind_flag) {
  int fd, comm_type;
  
  /* a) Copy IP destination information into sockaddr_in struture */
  serv_addr->sin_family = AF_INET;
  serv_addr->sin_port = htons(port);
  if(inet_pton(AF_INET, addr, &(serv_addr->sin_addr))<=0)
  {
    log_fatal("\nInvalid address or Address not supported for %s %s\n", d->id, d->path);
    exit(EXIT_FAILURE);
  }
  
  /* b) Create socket */
  if ( strcmp(d->comms, "udp") == 0) comm_type = SOCK_DGRAM;
  else                               comm_type = SOCK_STREAM;
  if ((fd = socket(AF_INET, comm_type, 0)) < 0)
  {
    log_fatal("\nSocket creation error for %s\n", d->id);
    exit(EXIT_FAILURE);
  }

  // c1) Bind the socket with the HAL server address
  if (bind_flag == 1) {
    if ( bind(fd, (const struct sockaddr *) serv_addr, sizeof(*serv_addr)) < 0 )
    {
      perror("\nbind failed");
      printf("%s\n", addr);
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
  if (strlen(d->addr_out) > 0) fd_out = inet_open_socket(d, d->addr_out, d->port_out, &(d->socaddr_out), 0);
  if (strlen(d->addr_in)  > 0) fd_in  = inet_open_socket(d, d->addr_in,  d->port_in,  &(d->socaddr_in),  1);

  /* Save file descriptors */
  if ( (fd_out == -1) && (fd_in == -1) ) {log_fatal("\nNo address speciffied for interface %s\n", d->id); exit(EXIT_FAILURE);}
  if ( (fd_out == -1) && (fd_in != -1) ) {d->readfd = fd_in;  d->writefd = fd_in;}
  if ( (fd_out != -1) && (fd_in == -1) ) {d->readfd = fd_out; d->writefd = fd_out;}
  if ( (fd_out != -1) && (fd_in != -1) ) {d->readfd = fd_in;  d->writefd = fd_out;}
}

/**********************************************************************/
/* Open Device; c) Bidirectional TTY Serial Link  */
/*********t************************************************************/
/* Open a serial (tty) interface for read-write and save the fds */
void interface_open_tty(device *d) {
  int fd;
  if ((fd = open(d->path, O_RDWR)) < 0) {
    log_fatal("Error opening device %s: %s\n", d->id, d->path);
    exit(EXIT_FAILURE);
  }
  d->readfd = fd;
  d->writefd = fd;
}

/**********************************************************************/
/* Open Device; d) Unidirectional Serial Links (based on root device) */
/*********t************************************************************/
/* open unidirection ILIP device pair (for read and write) */
void ilp_open_data_devices(device *d) {
  int fd_read, fd_write;

  // fprintf(stderr, "%s: Create data Devices external to HAL: %s %s\n", __func__, d->path_r, d->path_w);
  // tried | O_NONBLOCK  and O_RDWR
  if (d->enabled == 0) return;
  

  if (strlen(d->path_r) > 0) {

    log_trace("About to open device %s: %s", d->id, d->path_r);

    if ((fd_read  = open(d->path_r, O_RDONLY, S_IRUSR)) < 0) {
      log_fatal("Error opening device %s: %s\n", d->id, d->path_r);
      exit(EXIT_FAILURE);
    }
    d->readfd  = fd_read;

    log_trace("Opened read device (fd=%d) %s: %s", d->readfd, d->id, d->path_r);
  }
  
  if (strlen(d->path_w) > 0) {
    log_trace("About to open device %s: %s", d->id, d->path_w);

    if ((fd_write = open(d->path_w, O_WRONLY, S_IWUSR)) < 0) {
      log_fatal("Error opening device %s: %s", d->id, d->path_w);
      exit(EXIT_FAILURE);
    }
    d->writefd = fd_write;

    log_trace("Opened write device (fd=%d) %s: %s", d->writefd, d->id, d->path_w);
  }
}

/* Get session information (to_mux) from root device */
void ilp_root_check(int fd_root, root_device *rd) {
  int      i, j, device_pair_count;
  ssize_t  write_bytes, read_bytes;
  uint32_t application_id[2] = { 1, 2 };
  uint32_t session_id[2] = {0xffffffff, 0xffffffff};

  /* Take application_id from HAL config */
  device_pair_count = rd->data_dev_count;
  for (j=0; j<device_pair_count; j++)
    application_id[j] = rd->data_dev_list[j]->from_mux;
  
  /* For this test and this set of devices application ID is equal to session ID */
  for ( i=0; i<2; i++ ) {
    log_trace("Write Application ID (%d) to Root Device", application_id[i]);
    write_bytes = write(fd_root, (const void *)&application_id[i], sizeof(application_id[i]));
    if ( write_bytes != sizeof(session_id[i])) {
      perror( "Error in the write of the application ID: ");
      exit(EXIT_FAILURE);
    }
    read_bytes = read( fd_root, (void*)&session_id[i], sizeof(session_id[i]) );
    if ( read_bytes != sizeof(session_id[i])) {
      perror( "Error in the read of the session ID: ");
      exit(EXIT_FAILURE);
    }
    
    log_trace("TODO %s: Write Session IDs from Root Device into halmap (from_mux=%u to_mux=%u)", __func__, application_id[i], session_id[i]);
    
    if (session_id[i] != application_id[i]) {
      log_fatal("Invalid session ID returned Application: %u, Session ID: %u", application_id[i], session_id[i] );
      exit(EXIT_FAILURE);
    }
  }
}

/* Open root ILIP device */
void ilp_root_device_open(root_device *rd) {
  const char *root_path = rd->root_path;
  int  fd_root = -1;
  
  if (root_path != NULL) {
    if ((fd_root = open(root_path, O_RDWR, S_IRUSR | S_IWUSR)) < 0) {
      log_fatal("Error opening device %s: fd=%d", root_path, fd_root);
      exit(EXIT_FAILURE);
    }
    log_trace("Successfully opened root device %s (fd=%d)", root_path, fd_root);
    ilp_root_check(fd_root, rd);
    close(fd_root);
  }
}

/* save root's data paths and devices inro root_device structs */
void ilp_root_device_save_conig(device *d, int *root_count, root_device *root_list) {
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
  /* b) If no match found, write new root device */
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
}

/* Open ILIP interface */
void interface_open_ilp(device *d) {
  static int          root_count=0;
  static root_device  root_list[ILP_MAX_ROOT_DEVICES], *rd;
  int                 i, j;
  
  if (d != NULL) {        /* adding new devces */
//    fprintf(stderr, "%s: dev=%s %s\n", __func__, d->id, d->comms);
    ilp_root_device_save_conig(d, &root_count, root_list);
    return;
  }
//  fprintf(stderr, "%s: Open %d root device(s)\n", __func__, root_count);
  for (j=0; j<root_count; j++) {
    rd = &(root_list[j]);
    if (((rd->data_dev_list)[0])->init_enable == 1) {
      ilp_root_device_open(rd);
    }
    for (i=0; i<(rd->data_dev_count); i++) {
      ilp_open_data_devices(rd->data_dev_list[i]);
    }
  }
}

/**********************************************************************/
/* Open Devices */
/*********t************************************************************/
/* Open enabled devices (from linked-list of devices) and get their in/out handles */
void devices_open(device *dev_linked_list_root) {
  for(device *d = dev_linked_list_root; d != NULL; d = d->next) {
    if (d->enabled == 0) continue;
    if        (!strncmp(d->comms, "tty", 3))                                      interface_open_tty(d);
    else if ( (!strncmp(d->comms, "udp", 3)) || (!strncmp(d->comms, "tcp", 3)) )  interface_open_inet(d);
    else if   (!strncmp(d->comms, "ipc", 3))                                      interface_open_ipc(d);
    else if   (!strncmp(d->comms, "ilp", 3))                                      interface_open_ilp(d);
    else { log_fatal("Device %s [%s] unknown", d->id, d->path); exit(EXIT_FAILURE);}
// fprintf(stderr, "Open succeeded for %s [%s] (with fdr=%d fdw=%d)\n", d->id, d->path, d->readfd, d->writefd);
  }
  interface_open_ilp(NULL);
}
