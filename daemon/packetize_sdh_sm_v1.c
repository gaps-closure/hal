/*
 * Copy data and control information between PDU and Shared Memory
 *   September 2021, Perspecta Labs
 */

#include "hal.h"
#include "device_open.h"
#include "packetize_sdh_sm_v1.h"
#include "map.h"            /* get data_print */

/* increment index *v, modulo the maximum page size */
void mod_inc(uint32_t *v) {
  *v = ((*v) + 1) % PAGES_MAX;
}

/* Stores next Read Index. If mode=0 it returns value; else increments. */
uint32_t get_next_index(uint32_t r, uint32_t w, int mode) {
  static uint32_t next = 0;
         uint32_t current = next;
  if (mode == 1) mod_inc(&next);
  return (current);
}

/* Poll input device and return count of number of messages (w - r) */
int sdh_shm_poll (device *idev) {
  uint32_t r      = *(idev->block_r.shm_r);
  uint32_t w      = *(idev->block_r.shm_w);
  uint32_t next   = get_next_index(r, w, 0);
  uint32_t count  = (w - next) % PAGES_MAX;

  log_trace("Poll Shared Memory (%s %s): count=%d w=%d R=%d r=%d wo=0x%x ro=0x%x", idev->id, idev->path, count,  *(idev->block_r.shm_w), next, *(idev->block_r.shm_r), idev->addr_off_w, idev->addr_off_r);
  return (count);
}

/* Read message from idev (Shared Memory data and control) into PDU  */
int pdu_from_sdh_sm_v1 (pdu *out, device *idev) {
  uint32_t  r      = *(idev->block_r.shm_r);
  uint32_t  w      = *(idev->block_r.shm_w);
  int       index  = get_next_index(r, w, 1);
  int       len    = (idev->block_r.shm_l)[r];

  log_fatal("Read Shared Memory (%s %s): [r=%d R=%d w=%d] wo=0x%x ro=0x%x", idev->id, idev->path, r, index, w, idev->addr_off_w, idev->addr_off_r);
  out->data_len = len;
  tag_cp(&(out->psel.tag), &(idev->block_r.tag[r]));
  out->data = (idev->block_r.shm_d)[index];
  log_fatal("%p %0x02x %d\n", out->data, (idev->block_r.shm_d)[index][0], len);
  log_devs_debug(idev, __func__);
  return (len);
}

/* Write PDU message (in) into odev (Shared Memory) with tag 'otag' */
int pdu_into_sdh_sm_v1(device *odev, pdu *in, gaps_tag *otag) {
  struct timeval tv;
  uint64_t       ns_now;
  uint32_t       index = *(odev->block_w.shm_w);
  int            len = in->data_len;

//  sdh_shm_print(odev);
  gettimeofday(&tv,NULL);
  ns_now = (tv.tv_usec * 1000) + (tv.tv_sec  * 1000000000);
  log_trace("Write buffer into Shared Memory (%s %s) at index=%d: time in ns = (c=%lu o=%lu)\n", odev->id, odev->path, index, ns_now, odev->local_w.shm_t[index]);
  memcpy((odev->block_w.shm_d)[index], (uint8_t *) in->data, len);
  odev->block_w.shm_l[index] = len;
  tag_cp(&(odev->block_w.tag[index]), otag);
  mod_inc(odev->block_w.shm_w);
  log_devs_debug(odev, __func__);
  return (len);
}
