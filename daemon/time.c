/*
 * Put unix time into packets in network byte order (only for testing)
 *   December 2020, Perspecta Labs
 */

#include "hal.h"
#include "time.h"
#include "../appgen/6month-demo/float754.h"          /* use htonll */

void linux_time_set(uint64_t *t) {
  struct timeval tv;
  uint64_t ns;
  
  gettimeofday(&tv,NULL);
  ns =  (tv.tv_usec * 1000) + (tv.tv_sec  * 1000000000);
  *t = htonll(ns);          // Convert to BIG ENDIAN
  log_trace("unix time = %lu = 0x%lx nsec\n", ns, ns);
//  fprintf(stderr, "unix time=%lu.%lu sec = %lu = 0x%lx (net=0x%lx) nsec\n", tv.tv_sec, tv.tv_usec, ns, ns, *t);
}
