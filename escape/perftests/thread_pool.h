#ifndef THREAD_POOL_HEADER_FILE
#define THREAD_POOL_HEADER_FILE

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>                    // for gettimeofday()

#define EG_TASK_MODULUS             50
#define EG_TASK_COUNT               16   // Number of example jobs to be ran
#define EG_TASK_EXECUTE_TIME_US 500000   // Example job run time
#define MAX_THREADS                100   // Maximum number of threads that can be specified
#define MAX_QUEUE_DEPTH           1000   // Maximum number of jobs in queue
//#define DEBUG                            // Print debug messages

// JOBS (could use C-union to save space)
typedef struct _Job {
  char  type;                            // 'a' Add, 's' sub, 'a' apex_memcpy, 'g' glibc_memcpy, 'n' naive_memcpy
  int   a, b, result;                    // Used for simple example test (add or subtract)
  void *destin;                          // destination virtual address
  char *source;                          // source virtual address
  int   length;                          // length of data
} Job;

void thread_pool_create(int thread_count);
void thread_pool_destroy(int thread_count);
void job_wait_until_done(int thread_count);
int  job_push(Job job);
void job_execute(Job *job);

#endif // THREAD_POOL_HEADER_FILE
