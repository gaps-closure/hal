#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>

#define STDIN_READ_SIZE 8192

int verbose = 0;

void print_help() {
  printf("A netcat like utility for ZMQ\n");
  printf("Usage: zc [OPTION]... TYPE ENDPOINT\n");
  printf("OPTION: one of the following options:\n");
  printf(" -h --help : print this message\n");
  printf(" -b --bind : bind instead of connect\n");
  printf(" -d --delim : string marking end of stdin stream\n");
  printf(" -f --filter : ZMQ subscriber incoming message filter string (default is all messages)\n");
  printf(" -n --nbiter : number of iterations (0 for infinite loop)\n");
  printf(" -v --verbose : print some messages in stderr\n");
  printf("TYPE: set ZMQ socket type in req/rep/pub/sub/push/pull\n");
  printf("ENDPOINT: a string consisting of two parts as follows: transport://address (see zmq documentation)\n");
}

void exit_with_zmq_error(const char* where) {
    fprintf(stderr,"%s error %d: %s\n",where,errno,zmq_strerror(errno));
    exit(-1);
}

void free_buffer(void* data,void* hint) {
  free(data);
}

size_t send_from_stdin(void* socket, char *delim, char *endpoint) {
  size_t total = 0;
  int    i, per_read_count=STDIN_READ_SIZE, delim_len=0;
  char*  s;
  char*  buffer = malloc(STDIN_READ_SIZE);
    
  // read from stdin
  if (delim != NULL)  {
    per_read_count = 1;
    delim_len = (int) strlen(delim);
  }
  if(verbose) fprintf(stderr, "zc waits for stdin (up to %d bytes/read) to send to %s\n", per_read_count, endpoint);
  while(1) {
    size_t read = fread(buffer+total, 1, per_read_count, stdin);
    if(verbose) fprintf(stderr, "Read %ld (total=%ld) bytes on stdin\n", read, total);
    total += read;
    if(ferror(stdin)) {
      fprintf(stderr,"fread error %d: %s\n",errno,strerror(errno));
      exit(-2);
    }
    // test for end of stdin input, based on: a) speciied delimiter, b) EOF signal
    if ( (delim != NULL) && (total >= delim_len) ) {
//      if(verbose) fprintf(stderr,"zc testing for delimiter %s (len=%d): \n", delim, delim_len);
      s = strstr(buffer, delim);
      if (s != NULL) {
        total -= strlen(s);
        buffer[total] = 0;
        if(verbose) fprintf(stderr,"Delimited string len=%ld: %s\n", total, buffer);
        break;
      }
    }
    if(feof(stdin)) break;
    buffer = realloc(buffer,total+STDIN_READ_SIZE);
  }
  // prepare and send zmq message
  int err;
  zmq_msg_t msg;
  err = zmq_msg_init_data(&msg, buffer, total, free_buffer, NULL);
  if(err) exit_with_zmq_error("zmq_msg_init_data");

  if(verbose) fprintf(stderr, "sending %ld bytes into ZMQ\n", total);

  err = zmq_sendmsg(socket, &msg, 0);
  if(err==-1) exit_with_zmq_error("zmq_sendmsg");
    
  clearerr(stdin);     /* clears stdin end-of-file and error indicators (prevent looping) */
  return total;
}

size_t recv_to_stdout(void* socket, char *endpoint) {
  size_t total = 0;
  int err;
  int64_t more = 1;
  size_t more_size = sizeof(more);
  while(more) {
    zmq_msg_t msg;

    err = zmq_msg_init(&msg);
    if(err) exit_with_zmq_error("zmq_msg_init");

    if(verbose) fprintf(stderr, "\nzc waits to read from ZMQ %s to send to stdout\n", endpoint);
    err = zmq_recvmsg(socket, &msg, 0);
    if(err==-1) exit_with_zmq_error("zmq_recvmsg");

    // print message to stdout
    size_t size = zmq_msg_size(&msg);
    total += size;
    if(verbose) fprintf(stderr, "zc received %ld bytes on ZMQ-sub (and forwarding to stdout, with flush)\n", size);
    fwrite(zmq_msg_data(&msg), 1, size, stdout);
    fflush(stdout);
    err = zmq_msg_close(&msg);
    if(err) exit_with_zmq_error("zmq_msg_close");

    // check for multipart messages
    zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &more_size);
  }
  return total;
}

int main(int argc, char** argv) {

  static struct option options[] = {
    {"help",    no_argument,       0, 'h'},
    {"bind",    no_argument,       0, 'b'},
    {"delim",   required_argument, 0, 'd'},
    {"nbiter",  required_argument, 0, 'n'},
    {"verbose", no_argument,       0, 'v'},
    {0,         0,                 0, 0}
  };

  int bind = 0;
  char *delim = NULL;
  char filter[64]= "\0";
  int type = 0;
  char* endpoint = 0;
  int nbiter = 0;
  int iopt = 0;
  
  while(1) {
     int c = getopt_long(argc,argv,"hbd:f:n:v",options, &iopt);
     if(c==-1)
       break;
     switch(c) {
       case 'h':
         print_help();
         exit(0);
         break;
       case 'b':
         bind = 1;
         break;
       case 'd':
         delim = optarg;
         break;
       case 'f':
         strcpy(filter, optarg);
         break;
       case 'n':
         nbiter = atoi(optarg);
         break;
       case 'v':
         verbose = 1;
         break;
     }
  }
  if(optind<argc) {
    char* str_type = argv[optind++];
    if (!strcasecmp(str_type,"req"))
      type = ZMQ_REQ;
    else if (!strcasecmp(str_type,"rep"))
      type = ZMQ_REP;
    else if (!strcasecmp(str_type,"pub"))
      type = ZMQ_PUB;
    else if (!strcasecmp(str_type,"sub"))
      type = ZMQ_SUB;
    else if (!strcasecmp(str_type,"push"))
      type = ZMQ_PUSH;
    else if (!strcasecmp(str_type,"pull"))
      type = ZMQ_PULL;
  }
  if(optind<argc) {
    endpoint = argv[optind++];
  }

  if(!type || !endpoint) {
    print_help();
    return -1;
  }

  if(verbose) fprintf(stderr, "started zc: type=%d\n", type);

    
  void *ctx = zmq_init(1);
  if(ctx == NULL) exit_with_zmq_error("zmq_init");

  void *socket = zmq_socket(ctx, type);
  if(socket == NULL) exit_with_zmq_error("zmq_socket");

  int err;
  if(bind) {
    err = zmq_bind(socket,endpoint);
    if(err) exit_with_zmq_error("zmq_bind");
    if(verbose) fprintf(stderr,"bound to %s\n",endpoint);
  }
  else {
    err = zmq_connect(socket,endpoint);
    if(err) exit_with_zmq_error("zmq_connect");
    if(verbose) fprintf(stderr,"connected to %s\n",endpoint);
  }

  if(type==ZMQ_SUB) {
    if(verbose) fprintf(stderr,"Subscriber input filter = \"%s\"\n",filter);
    zmq_setsockopt(socket,ZMQ_SUBSCRIBE,filter,strlen(filter));
  }

  if(type==ZMQ_PUB) {
    usleep(10000); // let a chance for the connection to be established before sending a message
  }

  int i = 0;
  while(nbiter==0 || i++<nbiter) {
    switch(type) {
      case ZMQ_REQ:
      case ZMQ_PUB:
      case ZMQ_PUSH:
        send_from_stdin(socket, delim, endpoint);
    }
    switch(type) {
      case ZMQ_REQ:
      case ZMQ_REP:
      case ZMQ_SUB:
      case ZMQ_PULL:
        recv_to_stdout(socket, endpoint);
    }
    switch(type) {
      case ZMQ_REP:
        send_from_stdin(socket, delim, endpoint);
    }
  }

  err = zmq_close(socket);
  if(err) exit_with_zmq_error("zmq_close");

  err = zmq_term(ctx);
  if(err) exit_with_zmq_error("zmq_term");

  return 0;
}
