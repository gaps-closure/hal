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
  int count = (*(idev->block_r.shm_w)) - (*(idev->block_r.shm_r));
  log_trace("Poll Shared Memory (%s %s): count=%d w=%d r=%d wo=0x%x ro=0x%x", idev->id, idev->path, count,  *(idev->block_r.shm_w), *(idev->block_r.shm_r), idev->addr_off_w, idev->addr_off_r);
  return (count);
}

/* Read message from idev (Shared Memory data and control) into buffer  */
/* (not directly into pdu, as function name suggests for historical reasons) */
int pdu_from_sdh_sm_v1 (pdu *out, device *idev) {
  int count = (*(idev->block_r.shm_w)) - (*(idev->block_r.shm_r));
  int r;
  
  log_fatal("Read Shared Memory (%s %s): count=%d w=%d r=%d wo=0x%x ro=0x%x", idev->id, idev->path, count,  *(idev->block_r.shm_w), *(idev->block_r.shm_r), idev->addr_off_w, idev->addr_off_r);
    
  r = *(idev->block_r.shm_r);
  out->data_len = (idev->block_r.shm_l)[r];
  log_fatal("Todo: Fill in PDU from SHM Len=%d\n", out->data_len);

  exit (111);
  return (0);
}

/* Write PDU message (in) into odev (Shared Memory) with tag 'otag' */
int pdu_into_sdh_sm_v1(device *odev, pdu *in, gaps_tag *otag) {
  struct timeval tv;
  uint64_t       ns_now;
  uint32_t       w;
  int            len = in->data_len;

//  sdh_shm_print(odev);
  gettimeofday(&tv,NULL);
  ns_now = (tv.tv_usec * 1000) + (tv.tv_sec  * 1000000000);
  w = *(odev->block_w.shm_w);
  log_trace("Write buffer into Shared Memory (%s %s) at index=%d: time in ns = (c=%lu o=%lu)\n", odev->id, odev->path, w, ns_now, odev->local_w.shm_t[w]);
  memcpy((odev->block_w.shm_d)[w], (uint8_t *) in->data, len);
  odev->block_w.shm_l[w] = len;
  tag_cp(&(odev->block_w.tag[w]), otag);
  *(odev->block_w.shm_w) = ++w;
  log_devs_debug(odev, __func__);
  return (len);
}
