/*
 * Convert between PDU and Shared Memory
 *   August 2021, Perspecta Labs
 */

#include "hal.h"
#include "device_open.h"
#include "packetize_sdh_sm_v1.h"
#include "map.h"            /* get data_print */

/* Print M1 Packet */
void sdh_sm_print_one(sdh_sm_v1 *p) {
  fprintf(stderr, "%s: ", __func__);
  fprintf(stderr, "\n");
}

/* Put closure packet (input) into internal HAL PDU structure (output) */
//int pdu_from_sdh_sm_v1 (device *idev) {
int pdu_from_sdh_poll (device *d) {
  int count;
  count = (*(d->block_r.shm_w)) - (*(d->block_r.shm_r));
  log_fatal("Check if Shared Memory ready to read: count=%d (w=%d r=%d wo=0x%x ro=0x%x)", count, *(d->block_r.shm_w), *(d->block_r.shm_r), d->addr_off_w, d->addr_off_r);
  return (count);
}

void pdu_into_sdh_sm_v1(uint8_t *out, device *odev, int pkt_len, gaps_tag *otag) {
  struct timeval tv;
  uint64_t       ns_now;
  uint32_t       w;

//  sdh_shm_print(odev);
  gettimeofday(&tv,NULL);
  ns_now = (tv.tv_usec * 1000) + (tv.tv_sec  * 1000000000);
  w = *(odev->block_w.shm_w);
  log_trace("Shared Memory time (nsec) c=%lu o=%lu)\n", ns_now, odev->local_w.shm_t[w]);
  memcpy((odev->block_w.shm_d)[w], out, pkt_len);
  odev->block_w.shm_l[w] = pkt_len;
  tag_cp(&(odev->block_w.tag[w]), otag);
  *(odev->block_w.shm_w) = ++w;
//  log_devs_debug(odev, __func__);
}
