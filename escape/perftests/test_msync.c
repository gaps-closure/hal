/*
 * Simple file synchronization example using msync
  * gcc -o test_msync test_msync.c && ./test_msync testfile && cat testfile
 *   Create file:  dd if=/dev/zero of=testfile bs=4k count=1
 *   Compile:      gcc -o test_msync test_msync.c
 *   Run:          ./test_msync testfile
 *   Check:        od -c testfile    (OR  cat testfile)
 */


#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // for close()
#include <sys/mman.h>   // mmap()
#include <time.h>       // for time()

int main(int argc, char *argv[]) {
	int fd, rv;
	char *pmaddr;

  // A) Print Help
  if (argc != 2) {
    fprintf(stderr, "Usage: %s filename\n", argv[0]);
    fprintf(stderr, "       file:   %s testfile\n", argv[0]);
    fprintf(stderr, "       memory: %s /dev/mem\n", argv[0]);
		exit(1);
	}

  // B) Open File
  fd = open(argv[1], O_RDWR);
  if (fd < 0) {
    perror(argv[1]);
    exit(1);
  }
  else fprintf(stderr, "B) open(%s) worked with fd=%d\n", argv[1], fd);

	// C) Map 4k from the file into virtual memory for read & write.
  //    Use MAP_SHARED so stores go directly to shared memory (globally visible).
  pmaddr = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (pmaddr == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}
  else fprintf(stderr, "C) mmap() worked with rv=0x%p\n", pmaddr);
  
  /* D) Don't need the fd anymore, the mapping stays around */
  close(fd);

  // E) Store a string to the Persistent Memory */
  sprintf(pmaddr, "Hello at %ld seconds\n", time(NULL));

  // F) Flush above store all the say to the Shared Memory
  //    (may just be in cache, depending on system activity & cache pressure)
  rv = msync((void *)pmaddr, 4096, MS_SYNC);
  if (rv < 0) {
		perror("msync");
		exit(1);
	}
  else fprintf(stderr, "F) msync() worked with rv=%d\n", rv);
  
	exit(0);
}
