/*
 * Simple file synchronization example using fsync
 *  gcc -o test_fsync test_fsync.c && ./test_fsync testfile && cat testfile
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // for close()
#include <time.h>       // for time()


int main(int argc, char *argv[]) {
   char str[64];
   int fd, rv;
  
  // A) Print Help
  if (argc != 2) {
    fprintf(stderr, "Usage: %s filename\n", argv[0]);
    fprintf(stderr, "       file:   %s testfile\n", argv[0]);
    fprintf(stderr, "       memory: %s /dev/mem\n", argv[0]);
    exit(1);
  }
  
  // B) Open File
  fd = open(argv[1], O_RDWR | O_CREAT);
  if (fd < 0) {
    perror(argv[1]);
    exit(1);
  }
  else fprintf(stderr, "B) open(%s) worked with fd=%d\n", argv[1], fd);

  // E) Store a string to the Persistent Memory */
  sprintf(str, "Hello at %ld seconds\n", time(NULL));
   rv = write(fd, str, strlen(str));
   if(rv < -1) {
      perror("write()");
      exit(1); }
   else fprintf(stderr, "E) write() worked rv=%d\n", rv);
  
  // F) Flush above store all the say to the Shared Memory
   rv = fsync(fd);
   if(rv != 0) {
     perror("fsync()");
     exit(1);
   }
   else fprintf(stderr, "F) fsync() worked rv=%d\n", rv);
  
  /* D) Don't need the fd anymore, the mapping stays around */
   close(fd);

   return 0;
}
