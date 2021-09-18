/*
 * Convert between PDU and Shared Memory
 *   August 2021, Perspecta Labs
 */

#include "hal.h"
#include "device_open.h"
#include "packetize_sdh_sm_v1.h"
#include "map.h"            /* get data_print */

/* Poll input device and return count of number of messages (w - r) */
int sdh_shm_poll (device *idev) {
  log_fatal("Poll Shared Memory (%s %s): w=%d r=%d wo=0x%x ro=0x%x", idev->id, idev->path, *(idev->block_r.shm_w), *(idev->block_r.shm_r), idev->addr_off_w, idev->addr_off_r);
  return (*(idev->block_r.shm_w)) - (*(idev->block_r.shm_r));
}

/* Read message from idev (Shared Memory data and control) into buffer  */
/* (not directly into pdu, as function name suggests for historical reasons) */
int pdu_from_sdh_sm_v1 (pdu *out, device *idev) {
  log_trace("Reading from Shared Memory (%s %s) into buffer\n", idev->id, idev->path);
  log_fatal("Todo: Fill in PDU from SHM\n");

  exit (111);
}

/* Write PDU message (out, pkt_len) into odev (Shared Memory) with tag 'otag' */
void pdu_into_sdh_sm_v1(uint8_t *out, device *odev, int pkt_len, gaps_tag *otag) {
  struct timeval tv;
  uint64_t       ns_now;
  uint32_t       w;

//  sdh_shm_print(odev);
  gettimeofday(&tv,NULL);
  ns_now = (tv.tv_usec * 1000) + (tv.tv_sec  * 1000000000);
  w = *(odev->block_w.shm_w);
  log_trace("Write buffer into Shared Memory (%s %s) at index=%d: time in ns = (c=%lu o=%lu)\n", odev->id, odev->path, w, ns_now, odev->local_w.shm_t[w]);
  memcpy((odev->block_w.shm_d)[w], out, pkt_len);
  odev->block_w.shm_l[w] = pkt_len;
  tag_cp(&(odev->block_w.tag[w]), otag);
  *(odev->block_w.shm_w) = ++w;
//  log_devs_debug(odev, __func__);
}
