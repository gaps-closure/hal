#include "thread_pool.h"

Job             jobQueue[MAX_QUEUE_DEPTH];  // Job Queue
pthread_mutex_t job_queue_mutex;            // Job Queue lock
pthread_cond_t  job_queue_cond_signal;      // New Job in queue signal
pthread_t       thread_id[MAX_THREADS];     // Thread identifier list
int             jobCount = 0;
int             threads_idle = 0;

//#######################################################
// A) JOB QUEUE (Dumb, inefficient FIFO implementation)
//#######################################################
// Queue before: 1 2 3 4 5
// Queue after:  2 3 4 5
void job_pop(Job *job) {
  int  i;
    
  *job = jobQueue[0];
  for (i = 0; i < jobCount - 1; i++) {
    jobQueue[i] = jobQueue[i + 1];
  }
  jobCount--;
#ifdef DEBUG
  fprintf(stderr, "%s jobCount=%d\n", __func__, jobCount);
#endif // DEBUG
}

// Queue before: 1 2 3 4
// Queue after:  1 2 3 4 5
int job_push(Job job) {
//fprintf(stderr, "%s j=%c\n", __func__, job.type);

  pthread_mutex_lock(&job_queue_mutex);
  jobQueue[jobCount] = job;
  jobCount++;
  pthread_mutex_unlock(&job_queue_mutex);
  pthread_cond_signal(&job_queue_cond_signal);
  return (jobCount);
}

void job_wait_until_done(int thread_count) {
  while ((jobCount>0) || (threads_idle<thread_count)) usleep(10);
}

//#######################################################
// B) THREAD POOL
//#######################################################
// Worker threads continuously take jobs from queue and executes them
//   args is used to pass an thread index (for debug only)r
//   Specific to job 'Job' only where it uses 'job' parameter (can make generic)
void* startThread(void* args) {
  Job job;

  while (1) {
#ifdef DEBUG
    int index = * (int *) args;
    fprintf(stderr, "Thread %04d: (c=%d i=%d) 1) WAIT\n", index, jobCount, threads_idle);
#endif // DEBUG
    pthread_mutex_lock(&job_queue_mutex);  // only 1 thread passes here at one time
    threads_idle++;
    // Take next job (if none then wait for signal 'job_queue_cond_signal' and releases lock)
    while (jobCount == 0) pthread_cond_wait(&job_queue_cond_signal, &job_queue_mutex);
    threads_idle--;
    job_pop(&job);              // get next job
#ifdef DEBUG
    fprintf(stderr, "Thread %04d: (c=%d i=%d) 2) POPS\n", index, jobCount, threads_idle);
#endif // DEBUG
    pthread_mutex_unlock(&job_queue_mutex);
    job_execute(&job);  // Do job after unlock (so can do in parallel)
  }
}

// Create the 'thread_count' worker threads
void thread_pool_create(int thread_count) {
  int i;
  int *arg;
  
  if (thread_count <= 0) {
    return;
  }
  fprintf(stderr, "Starting %d worker threads\n", thread_count);
  arg = malloc(sizeof(*arg *thread_count));
  if (arg == NULL) {
    fprintf(stderr, "pthread arg Memory could not be allocated.\n");
    exit(0);
  }
  pthread_mutex_init(&job_queue_mutex, NULL);  // mutex for work queue (default attributes)
  pthread_cond_init(&job_queue_cond_signal, NULL);    // signal unblocks thread when queue has work

  for (i = 0; i < thread_count; i++) {
    *(arg+i) = i;
    if (pthread_create(&thread_id[i], NULL, &startThread, arg+i) != 0) {
      perror("Failed to create the thread");
    }
  }
  free(arg);
}

// Destroy all threads (if threads not in infinite loop)
void thread_pool_destroy(int thread_count) {
  int i;
  for (i = 0; i < thread_count; i++) {
    printf("stop index=%d\n", i);
    if (pthread_join(thread_id[i], NULL) != 0) {
      perror("Failed to join the thread");
    }
  }
  pthread_mutex_destroy(&job_queue_mutex);
  pthread_cond_destroy( &job_queue_cond_signal);
}
