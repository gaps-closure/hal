// Test latency of simple 16 jobs (each taking 0.5 seconds)
// with different number of worker threads
//    March 24, 2023
//
// Usage: ./memcpy_test [number of threads]
//  $ make
//     gcc -c -o thread_pool.o thread_pool.c -g -Iinclude -O3 -Wall -Werror -std=gnu99
//     gcc -g -Iinclude -O3 -Wall -Werror -std=gnu99 -o thread_eg thread_eg.c thread_pool.o -lpthread

//  $ ./thread_eg
//     Starting 1 worker threads
//     Delta = 8.002, Threads = 1, Jobs = 16, Execution time of 1 job = 0.500000 sec
//  $ ./thread_eg 2
//     Starting 2 worker threads
//     Delta = 4.001, Threads = 2, Jobs = 16, Execution time of 1 job = 0.500000 sec

#include "thread_pool.h"

//#######################################################
// A) SIMPLE EXAMPLE TASKS; execution and creation
//#######################################################
// Push random example jobs onto job queue
void job_create_and_push(char type) {
  int  i;
  Job j1;  // Job description (contains its operation and i/o parameters)
  
  j1.type = type;
#ifdef DEBUG
  fprintf(stderr, "%s type=%c\n", __func__, j1.type);
#endif // DEBUG

  srand(time(NULL));
  for (i = 0; i < EG_TASK_COUNT; i++) {
    j1.a = rand() % EG_TASK_MODULUS;
    j1.b = rand() % EG_TASK_MODULUS;
    job_push(j1);             //  copy job into job queue
  }
}

void job_execute(Job *j1) {
  switch (j1->type) {
    case 'a':
      j1->result = j1->a + j1->b;
      usleep(EG_TASK_EXECUTE_TIME_US);      // Emulate computation time
#ifdef DEBUG
      fprintf(stderr, "%c(%d, %d) = %d\n", j1->type, j1->a, j1->b, j1->result);
#endif // DEBUG
      break;
    case 's':
      j1->result = j1->a - j1->b;
      usleep(EG_TASK_EXECUTE_TIME_US);      // Emulate computation time
#ifdef DEBUG
      fprintf(stderr, "%d - %d = %d\n", j1->a, j1->b, j1->result);
#endif // DEBUG
      break;
    default:
      fprintf(stderr, "Unknown example operation: op=%c (a=%d, b=%d)\n", j1->type, j1->a, j1->b);
      exit(1);
  }
}

//#######################################################
// B) RUN EXPERIMENT
//#######################################################
void usage(char **argv) {
  fprintf(stderr, "\nUsage:\t%s [number of threads up to %d]\n", argv[0], MAX_THREADS);
  exit(1);
}

void get_arguments(int argc, char **argv, int *thread_count) {
  if(argc > 1) *thread_count = atoi(argv[1]);
  else *thread_count = 1;
  if ((*thread_count) <= 0) usage(argv);
  if ((argc > 2) || ((*thread_count) > MAX_THREADS)) usage(argv);
}

int main(int argc, char* argv[]) {
  int       thread_count;
  struct    timeval  t2, t1;
  long      us;

  // a) Start thread pool and jobs (can do in either order)
  get_arguments(argc, argv, &thread_count);
#ifdef DEBUG
  fprintf(stderr, "%s Threads = %d\n", __func__, thread_count);
#endif // DEBUG
  thread_pool_create (thread_count);
  gettimeofday(&t2, NULL);
  job_create_and_push('s');      // subtract a - b
   
  // b) Wait until all jobs are done, then print results
  job_wait_until_done(thread_count);
  gettimeofday(&t1, NULL);
  us = ((t1.tv_sec - t2.tv_sec) * 1000000) + (t1.tv_usec - t2.tv_usec);
  fprintf(stderr, "Delta = %.03f, Threads = %d, Jobs = %d, Execution time of 1 job = %f sec\n", ((double) us)/1000000, thread_count, EG_TASK_COUNT, ((double) EG_TASK_EXECUTE_TIME_US)/1000000);
// Currently threads in infinine loop so not wait for threads to be done
// thread_pool_destroy (thread_count);
  return 0;
}
