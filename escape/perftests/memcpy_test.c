// Memory Speed Tests varying memory pair types, payload lengths and copy functions
//    March 10, 2023
// Usage:  ./memcpy_test -h

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>         // clock_gettime
#include <fcntl.h>        // open
#include <unistd.h>       // close
#include <errno.h>
#include <sys/mman.h>     // mmap
#include "apex_memmove.h" // memcpyA

// 1) experiment iteration parameters
#define DEFAULT_TEST_RUNS      5                // Number of iterations for given types, lengths and copy-func
#define DEF_NUM_PAYLOAD_LEN    9                // Default Number of payload lengths (in list)
#define MAX_NUM_PAYLOAD_LEN    10               // MAX Number of payload lengths (in list)
static unsigned long  payload_len_list[MAX_NUM_PAYLOAD_LEN] = {0x10, 0x100, 0x400, 0x1000, 0x10000, 0x80000, 0x100000, 0x400000, 0x1000000, 0x10000000};


// 2) Memory allocation parameters
//#define LEN_HOST_HEAP        0x10000000UL     // 256 MB (mmap param #2)
//#define LEN_HOST_MMAP        0x100000UL       // 1   MB  (<= 0x100000  allowed for /dev/mem)  XXX Fails
//#define LEN_HOST_HEAP        0x1000000UL      // 16  MB (<= 0x1000000 prevents memcpy buf overflow)
#define LEN_HOST_HEAP        0x10000000UL     // 256  MB (<= 0x1000000 prevents memcpy buf overflow)
#define LEN_HOST_MMAP        0x80000UL        // 0.5 MB
#define LEN_ESCA_MMAP        0x10000000UL     // 256 MB
#define PAGE_MASK            (sysconf(_SC_PAGE_SIZE) - 1)    // Normally 4K - 1
#define MMAP_ADDR_ESCAPE     0x2080000000UL   // mmap physical memory address @ 130 GB
#define MMAP_ADDR_HOST       0x0UL            // System selectsmmap physical memory address

// 3) Other
#define BYTES_PER_WORD       (sizeof(unsigned long)/sizeof(char))
#define MAX_MEM_PAIRS        6                 // Number of memory type combos: 4,5 only on escape boxes
#define MAX_NAME_LEN         32
#define BILLION              1000000000
#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
    __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)

//#define DEBUG
//#define PRINT_DATA

//**************************************************************************************
// A) Timing Operations
//**************************************************************************************
// Print time difference with nanosecond accuracy
void get_delta(struct timespec ts, struct timespec te, unsigned long *delta_nsec) {
  *delta_nsec += ((te.tv_sec - ts.tv_sec) * BILLION) + (te.tv_nsec - ts.tv_nsec);
#ifdef DEBUG
  printf("Delta=%ld ns [%ld.%ld - %ld.%ld]\n", ((te.tv_sec - ts.tv_sec) * BILLION) + (te.tv_nsec - ts.tv_nsec), te.tv_sec, te.tv_nsec, ts.tv_sec, ts.tv_nsec);
#endif // DEBUG
}

double get_rate(unsigned long bytes_per_run, int num_runs, unsigned long delta_nsec) {
  return ((double) bytes_per_run * num_runs / delta_nsec);
}

// Print time difference with nanosecond accuracy
void print_delta(char cpy_type[MAX_NAME_LEN], unsigned long delta_nsec, unsigned long bytes_per_run, int num_runs) {
  if (delta_nsec > 0) {
    fprintf(stderr, "%8ld bytes using %s = %7.3f GB/s (%d runs: ave delta = %.9f secs)\n", bytes_per_run, cpy_type, get_rate(bytes_per_run, num_runs, delta_nsec), num_runs, ((double) delta_nsec)/BILLION/num_runs);
  }
  else {
    fprintf(stderr, "%s = 0 Gb/s\n", cpy_type);
  }
}

void write_delta(char cpy_type[MAX_NAME_LEN], unsigned long delta_nsec, unsigned long bytes_per_run, int num_runs, FILE *fptr, char *mem_pair_label) {
  if (delta_nsec > 0) {
    fprintf(fptr, "%s, %ld, %s, %.3f, %d\n", mem_pair_label, bytes_per_run, cpy_type, get_rate(bytes_per_run, num_runs, delta_nsec), num_runs);
  }
}

void write_header(FILE *fptr) {
  fprintf(fptr, "Experiment Description, Copy lenth (Bytes), Copy Type, Throughput (Gbps), Number of Runs\n");
}

//**************************************************************************************
// B) Payload data operations
//**************************************************************************************
/* print first two and last three 8-byte words of data */
void print_data(const char *str, void *data, unsigned long len_in_bytes) {
  int i, end_print=3;
  unsigned long *ul_data_ptr = ((unsigned long *) data) + end_print - 1;
  unsigned long *last        = ((unsigned long *) data) + (len_in_bytes/BYTES_PER_WORD) - 1;

  fprintf(stderr, "%s data [len=0x%lx bytes]: 0x %lx %lx ...", str, len_in_bytes, *last, *(last -1));
  for (i = 0; i < end_print; i++) {
    fprintf(stderr, " %lx", *ul_data_ptr--);
  }
  fprintf(stderr, "\n");
}

void set_data(char *d, unsigned long len_in_bytes, int data_offset) {
  for (int i = 0; i < len_in_bytes; i++) *d++ = i + data_offset;
  print_data("    sour", d - len_in_bytes, len_in_bytes);
}

void clear_data(char *d, unsigned long len_in_bytes) {
  for (int i = 0; i < len_in_bytes; i++) *d++ = 0;
//  print_data(__func__, d - len_in_bytes, len_in_bytes);
}

// no longer used (faster?)
void clear_data2(char *d, unsigned long len_in_bytes) {
  unsigned long   len_in_words=len_in_bytes/BYTES_PER_WORD;
  unsigned long  *w = (unsigned long *) d;
  for (int i = 0; i < len_in_words; i++) *w++ = 0;
}

//**************************************************************************************
// C) Test with different memcpy memthods and different payload lengths
//**************************************************************************************
// Dumb memory copy using 8-byte words
void naive_memcpy(unsigned long *d, const unsigned long *s, unsigned long len_in_words) {
  for (int i = 0; i < len_in_words; i++) *d++ = *s++;
}

void process_results(unsigned long payload_len, FILE *fptr, char *mem_pair_label, struct timespec ts, struct timespec te, char *cpy_type, char *destin, int num_test_runs) {
  unsigned long    delta_nsec=0;

  get_delta(ts, te, &delta_nsec);
  print_delta(cpy_type, delta_nsec, payload_len, num_test_runs);
  write_delta(cpy_type, delta_nsec, payload_len, num_test_runs, fptr, mem_pair_label);
#ifdef PRINT_DATA
  print_data("memcpy1", destin, payload_len);
#endif // PRINT_DATA
//  clear_data(destin, payload_len);
}

// Different memcpy methods
void batch_tests_for_given_length(char *destin, char *source, unsigned long payload_len, FILE *fptr, char *mem_pair_label, int num_test_runs) {
  struct timespec  ts, te;
  
#ifdef DEBUG
  fprintf(stderr, "%s: L=0x%lx d=%p s=%p d-s=0x%x\n", __func__, payload_len, destin, source, abs((int) (destin - source)));
#endif // DEBUG
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);         // nanosecond clock
  for (int j=0; j < num_test_runs; j++) memcpy(destin, source, payload_len);
  clock_gettime(CLOCK_MONOTONIC_RAW, &te);
  process_results(payload_len, fptr, mem_pair_label, ts, te, "glibc_memcpy", destin, num_test_runs);
          
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  for (int j=0; j < num_test_runs; j++) naive_memcpy((unsigned long *) destin, (const unsigned long *) source, payload_len/BYTES_PER_WORD);
  clock_gettime(CLOCK_MONOTONIC_RAW, &te);
  process_results(payload_len, fptr, mem_pair_label, ts, te, "naive_memcpy", destin, num_test_runs);

  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  for (int j=0; j < num_test_runs; j++) apex_memcpy(destin, source, payload_len);
  clock_gettime(CLOCK_MONOTONIC_RAW, &te);
  process_results(payload_len, fptr, mem_pair_label, ts, te, " apex_memcpy", destin, num_test_runs);
}
          

// X axis payload: 16B, 256B, 1024B, 4KB, 64KB (video demo), 1MB, 4MB, 16MB  (>= 0x28UL to print data)
void run_per_payload_length(char *source, char *destin, int data_offset, unsigned long pa_map_length, int payload_len_num, int num_test_runs, unsigned long  *payload_len_list, FILE *fptr, char *mem_pair_label) {

#ifdef DEBUG
  fprintf(stderr, "%s\n", __func__);
#endif // DEBUG
  for (int indexL=0; indexL<payload_len_num; indexL++) {
    if (payload_len_list[indexL] > pa_map_length) {
      fprintf(stderr, "    Cannot run test with payload length (0x%lx) > mapped length (0x%lx)\n", payload_len_list[indexL], pa_map_length);
      break;
    }
    batch_tests_for_given_length(destin, source, payload_len_list[indexL], fptr, mem_pair_label, num_test_runs);
  }
  print_data("    dest", destin, payload_len_list[payload_len_num-1]);
}

//**************************************************************************************
// D) Allocate (and deallocate) different memory types
//**************************************************************************************
// Close shared mmap memory
void mmdealloc(int fd, void *pa_virt_addr, unsigned long pa_map_length) {
#ifdef __TEST_USE_MLOCK__
  int lockerr;
  lockerr = munlock(pa_virt_addr, pa_map_length);
  if (lockerr) fprintf(stderr, "ERROR: munlock failed \n");  // XXX: ought to bail out
#endif /* __TEST_USE_MLOCK__ */
  if(munmap(pa_virt_addr, pa_map_length) != 0) FATAL;
  close(fd);
}

// Open shared mmap memory at given Physical address
//   Returns file descriptor and page-aligned address/length, so can deallocate
char *mmalloc(int protection, unsigned long phys_addr, int *fd, void **pa_virt_addr, unsigned long *pa_map_length, int anon) {
  void          *virt_addr;
  unsigned long  pa_phys_addr;       /* page aligned physical address (offset) */
  int            flags = 0;
    flags |= MAP_SHARED;
//  flags |= MAP_HUGETLB | MAP_HUGE_1GB;
//  flags |= MAP_NORESERVE;
//  flags |= MAP_HUGETLB;
//  flags |= MAP_PRIVATE | MAP_POPULATE | MAP_LOCKED;

  if (anon > 0) flags |= MAP_ANONYMOUS;
  if((*fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
  pa_phys_addr   = phys_addr & ~PAGE_MASK;    // Align physical addr (offset) to be at multiple of page size.
  *pa_map_length = (*pa_map_length) + phys_addr - pa_phys_addr;     // Increase len due to phy addr alignment
  *pa_virt_addr  = mmap(0, *pa_map_length, protection, flags, *fd, pa_phys_addr);
  if (*pa_virt_addr == (void *) MAP_FAILED) FATAL;   // MAP_FAILED = -1
  virt_addr      = *pa_virt_addr + phys_addr - pa_phys_addr;   // add offset to page aligned addr
  fprintf(stderr, "    Shared mmap'ed DDR [len=0x%lx Bytes] starts at virtual address %p\n", *pa_map_length, virt_addr);

#ifdef __TEST_USE_MLOCK__
  int lockerr;
  lockerr = mlock(*pa_virt_addr, *pa_map_length);
  if (lockerr) fprintf(stderr, "ERROR: mlock failed \n");  // XXX: ought to bail out
#endif /* __TEST_USE_MLOCK__ */

#ifdef DEBUG
  fprintf(stderr, "Shared mmap'ed DDR [len=0x%lx Bytes] starts at virtual address %p\n", *pa_map_length, virt_addr);
  if (phys_addr > 0) fprintf(stderr, "Using Linux Physical addr %p: up to pa=0x%lX\n",
                                  (void *) phys_addr, phys_addr + (*pa_map_length) - 1);
#endif
  return (virt_addr);
}

void log_results(int indexM, char *mem_pair_label, int *fd, char *v_addr, unsigned long p_addr, unsigned long pa_map_length) {
  fprintf(stderr, "%d) %s (fd=%d, vir-addr=%p, phy-addr=0x%lx, len=%f MB)\n",
          indexM, mem_pair_label, *fd, v_addr, p_addr, (double) pa_map_length/1000000);
}

void mem_deallocate(int fd, void *pa_virt_addr, unsigned long pa_map_length) {
  if (fd > 0) mmdealloc(fd, pa_virt_addr, pa_map_length);
  else        free(pa_virt_addr);
}

// Open shared memory of different types for source (read) or destination (write)
//   mem: host DDR heap, host DDR mmap, ESCAPE device DDR mmap
//   op: read, write, write then read from other side
void mem_allocate(int indexM, char *mem_pair_label, char *app_mem, char **source, char **destin, int data_offset, int source_init, int anon, int *fd, void **pa_virt_addr, unsigned long *pa_map_length, int *payload_len_num) {
  
  *fd = -1;
  switch (indexM) {
    case 0:
      strcpy(mem_pair_label, "App writes to host-heap");
      *pa_map_length = LEN_HOST_HEAP;
      *source        = app_mem;                               // Source of write is applicaiton memory
      set_data(*source, *pa_map_length, data_offset);
      if ((*destin = malloc(*pa_map_length)) == NULL) FATAL;  // Destimation of write in host heap
      log_results(indexM, mem_pair_label, fd, *destin, 0, *pa_map_length);
      *pa_virt_addr  = *destin;                               // for free()
      break;
    case 1:
      strcpy(mem_pair_label, "App reads from host-heap");
      *pa_map_length = LEN_HOST_HEAP;
      if ((*source = malloc(*pa_map_length)) == NULL) FATAL;  // Source of read is host heap
      if (source_init==0) set_data(*source, *pa_map_length, data_offset);
      *destin = app_mem;                             // Destimation of read is applicaiton memory
      log_results(indexM, mem_pair_label, fd, *source, 0, *pa_map_length);
      *pa_virt_addr = *source;   /* for free() */
      break;
    case 2:
      strcpy(mem_pair_label, "App writes to host-mmap");
      *pa_map_length = LEN_HOST_MMAP;
      *source = app_mem;                                                     // Source of write is app mem
      set_data(*source, *pa_map_length, data_offset);
      *destin = mmalloc(PROT_WRITE, MMAP_ADDR_HOST, fd, pa_virt_addr, pa_map_length, anon); // write to host mmap
      log_results(indexM, mem_pair_label, fd, *destin, 0, *pa_map_length);
      break;
    case 3:
      strcpy(mem_pair_label, "App reads from host-mmap");
      *pa_map_length = LEN_HOST_MMAP;
      *source = mmalloc(PROT_READ | PROT_WRITE, MMAP_ADDR_HOST, fd, pa_virt_addr, pa_map_length, anon); // Source of write is host mmap
      if (source_init==0) set_data(*source, *pa_map_length, data_offset);
      *destin = app_mem;                                                     // Write to app memory
      log_results(indexM, mem_pair_label, fd, *source, 0, *pa_map_length);
      break;
    case 4:
      strcpy(mem_pair_label, "App writes to escape-mmap");
      *pa_map_length = LEN_ESCA_MMAP;
      *source = app_mem;                                                     // Source of write is app mem
      set_data(*source, *pa_map_length, data_offset);
      *destin = mmalloc(PROT_WRITE, MMAP_ADDR_ESCAPE, fd, pa_virt_addr, pa_map_length, anon); // write to ESCAPE mmap
      log_results(indexM, mem_pair_label, fd, *destin, MMAP_ADDR_ESCAPE, *pa_map_length);
      break;
    case 5:
      strcpy(mem_pair_label, "App reads from escape-mmap");
      *pa_map_length = LEN_ESCA_MMAP;
      *source = mmalloc(PROT_READ | PROT_WRITE, MMAP_ADDR_ESCAPE, fd, pa_virt_addr, pa_map_length, anon); // Write to ESCAPE mmap
      if (source_init==0) set_data(*source, *pa_map_length, data_offset);
      *destin = app_mem;                             // Destimation of write is applicaiton memory
      log_results(indexM, mem_pair_label, fd, *source, MMAP_ADDR_ESCAPE, *pa_map_length);
      break;
    default:
      fprintf(stderr, "%s: Not implemented for index=%d\n", __func__, indexM);
      exit (1);
  }
}

// Test with different Memory types (host heap, host mmap, escape mmap)
void run_per_mem_type_pair (int *mem_pair_list, int num_mem_pairs, int data_offset, int source_init, int payload_len_num, int num_test_runs, int anon, FILE *fptr) {
  int            i, fd = -1;
  void          *pa_virt_addr;
  unsigned long  pa_map_length;
  char          *app_mem, *source, *destin, mem_pair_label[32];

  if ((app_mem = malloc(LEN_HOST_HEAP)) == NULL) FATAL;
  fprintf(stderr, "App Memory uses host Heap [len=0x%lx Bytes] at virtual address %p\n", LEN_HOST_HEAP, app_mem);
  for (i=0; i<num_mem_pairs; i++) {
    fprintf(stderr, "--------------------------------------------------------------------------------------\n");
    mem_allocate(mem_pair_list[i], mem_pair_label, app_mem, &source, &destin, data_offset, source_init, anon, &fd, &pa_virt_addr, &pa_map_length, &payload_len_num);
    run_per_payload_length(source, destin, data_offset, pa_map_length, payload_len_num, num_test_runs, payload_len_list, fptr, mem_pair_label);
#ifdef DEBUG
    fprintf(stderr, "Deallocating memroy: fd=%d pa_virt_addr=%p pa_map_len=%ld mem_typ_pair_indexM=%d\n", fd, pa_virt_addr, pa_map_length, mem_pair_list[i]);
#endif // DEBUG
    mem_deallocate(fd, pa_virt_addr, pa_map_length);
  }
}


//**************************************************************************************
// E) Get user options for test
//**************************************************************************************
void init_mem_pair_list(int *mem_pair_list, int *num_mem_pairs) {
  for (; *num_mem_pairs<MAX_MEM_PAIRS; (*num_mem_pairs)++) {
    mem_pair_list[*num_mem_pairs] = *num_mem_pairs; // default to all mem-pairs
  }
  *num_mem_pairs = MAX_MEM_PAIRS;
}

void opts_print(void) {
  printf("Memory speed test for GAPS CLOSURE project\n");
  printf("Usage: ./escape_test [OPTIONS]... [Experiment ID List]\n");
  printf("OPTIONS: are one of the following:\n");
  printf(" -h : print this message\n");
  printf(" -i : which source data is initialized\n"
         "\t 0 = all sources (default)\n"
         "\t 1 = only if source is application - read on different node to write\n"
        );
  printf(" -n : number of length tests (default=%d, maximum = %d)\n", DEF_NUM_PAYLOAD_LEN, MAX_NUM_PAYLOAD_LEN);
  printf(" -o : source data initialization offset value (before writing)\n");
  printf(" -r : number of test runs for each a) memory pair type, b) payload length and c) copy function\n");
  printf("Experiment IDs (default runs all experiments):\n"
         "\t 0 = write to host heap\n"
         "\t 1 = read from host heap\n"
         "\t 2 = write to host mmap\n"
         "\t 3 = read from host mmap\n"
         "\t 4 = write to shared escape mmap\n"
         "\t 5 = read from shared escape mmap\n"
        );
}

/* Get script's command line options */
void get_options(int argc, char *argv[], int *mem_pair_list, int *data_offset, int *source_init, int *payload_len_num, int *num_test_runs, int *num_mem_pairs, int *anon) {
  int  opt;
  
  while((opt = getopt(argc, argv, ":ahi:n:o:r:")) != EOF) {
    switch (opt) {
      case 'a':
        *anon = 1;
        break;
      case 'h':
        opts_print();
        exit(0);
        break;
      case 'i':
        *source_init = atoi(optarg);
        break;
      case 'n':
        *payload_len_num = atoi(optarg);
        if ((*payload_len_num) > MAX_NUM_PAYLOAD_LEN) *payload_len_num = DEF_NUM_PAYLOAD_LEN;
        break;
      case 'o':
        *data_offset = atoi(optarg);
        break;
      case 'r':
        *num_test_runs = atoi(optarg);
        break;
      case ':':
        fprintf(stderr, "Option -%c needs a value\n", optopt);
        opts_print();
        exit(EXIT_FAILURE);
        break;
      default:
        fprintf(stderr, "Skipping undefined Option (%d)\n", optopt);
    }
  }
  for (*num_mem_pairs=0; optind<argc; optind++) {
    mem_pair_list[*num_mem_pairs] = atoi(argv[optind]);
    (*num_mem_pairs)++;
    if (*num_mem_pairs == MAX_MEM_PAIRS) break;
  }
  if (*num_mem_pairs == 0) init_mem_pair_list(mem_pair_list, num_mem_pairs);  // set default
}

void config_print(int data_offset, int source_init, int payload_len_num, int num_test_runs, int num_mem_pairs, int *mem_pair_list) {
  int i;
  
  fprintf(stderr, "PAGE_MASK=0x%08lx data_off=%d source_init=%d payload_len_num=%d runs=%d num_mem_pairs=%d [ ", PAGE_MASK, data_offset, source_init, payload_len_num, num_test_runs, num_mem_pairs);
  for (i=0; i<num_mem_pairs; i++) fprintf(stderr, "%d ", mem_pair_list[i]);
  fprintf(stderr, "]\n");
}

/* Run Shared Memory Experiment */
int main(int argc, char *argv[]) {
  int  num_mem_pairs;
  int  anon            = 0;                    // default to not backed by a file
  int  data_offset     = 0;                    // default to data bytes start at 0
  int  source_init     = 0;                    // default to initialize all sources
  int  payload_len_num = DEF_NUM_PAYLOAD_LEN;  // default to first nine lengths
  int  mem_pair_list[MAX_MEM_PAIRS];           // List of memory pair indexes to run
  int  num_test_runs   = DEFAULT_TEST_RUNS;    // default to first nine lengths
  FILE *fptr;                                  // file pointer to work with files
  
  get_options(argc, argv, mem_pair_list, &data_offset, &source_init, &payload_len_num, &num_test_runs, &num_mem_pairs, &anon);
  config_print(data_offset, source_init, payload_len_num, num_test_runs, num_mem_pairs, mem_pair_list);
  fptr = fopen("results.csv", "w");   // opening file in writing mode
  write_header(fptr);
  run_per_mem_type_pair(mem_pair_list, num_mem_pairs, data_offset, source_init, payload_len_num, num_test_runs, anon, fptr);
  fclose(fptr);
  return 0;
}
