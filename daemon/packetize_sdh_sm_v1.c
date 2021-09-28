/*
 * Copy data and control information between PDU and Shared Memory
 *   September 2021, Perspecta Labs
 */

#include "hal.h"
#include "device_open.h"
#include "packetize_sdh_sm_v1.h"
#include "map.h"            /* get data_print */
#include <sys/mman.h>

/*=================================================================
 Math Functions with special modulo index arithmentic
   Write Index w = positive int between 0 and (PAGES_MAX - 1)
   Read Index  r = same as w, except is set to -1 initially
   Count       c = positive int from 0 to PAGES_MAX
               c = (w - r) % PAGES_MAX, except:
               c = PAGES_MAX when r = w (queue full, not empty)
               c = 0 only when r = -1   (queue empty)
 =================================================================*/

/* increment index modulo the maximum page size */
void mod_inc(uint32_t *v) {
  *v = ((*v) + 1) % PAGES_MAX;
}

/* Stores next Read Index at receiver. If mode=1 increments for next time. */
uint32_t get_next_index(int r, int w, int mode) {
  static int next = 0;
         int current = next;
  
  if ((w==0) && (r < 0)) return (r);
  if (mode == 1) mod_inc((uint32_t *) (&next));
  return (current);
}

uint32_t mod_sub(int r, int w) {
  return ((w - r + PAGES_MAX) % PAGES_MAX);
}

/* w - r with positive result (special case when in initial state r=-1) */
uint32_t get_read_count(int r, int w) {
  if (r < 0) return (0);
  return (mod_sub(r, w));
}

/* Pages sent count is w - r, except when 0 (see notes above) */
int sdh_shm_get_sent_count(int r, int w) {
  int count;

  if (r < 0) return (0);
  count = (int) mod_sub(r, w);
  if (count==0) return (PAGES_MAX);
  return (count);
}

/*=================================================================
 Read and Write Functions
 =================================================================*/
/* remove one queue entry if it has expired. Return count of deleted entries */
void shm_invalid_one(device *odev, uint32_t index, int w, uint32_t sent_count, uint64_t c) {
  int       v;
  uint64_t  a, t, duration;

  a = ((uint64_t) odev->guard_time_aw) * 1000000;    /* after write hold time in nanoseconds */
  v = odev->local_w.shm_v[index];
  t = odev->local_w.shm_t[index];
  duration = c - t;
  log_debug("Test A on index %d v=%d w=%d", index, v, w);
  log_debug("duration=(c-t)=(%ld - %ld) = %ld", c, t, duration);
  log_debug("ti_delta=(d-a)=(%ld - %ld) = %ld", duration, a, (duration - a));
  if (v == 1) {
    if ((duration > a) || ((sent_count == PAGES_MAX) && (index == w))) {
      log_debug("++++++ delete index=%d", index);
      odev->local_w.shm_v[index] = 0;
      odev->local_w.shm_t[index] = c;
      *(odev->block_w.shm_r)     = index + 1;
    }
  }
}

/* Remove (v=0) up to the K oldest pages in queue */
/* Return index to oldest page available (or -1 if no pages available) */
int shm_invalid_last_K_in_Q(device *odev, uint64_t c) {
  int             i, K=2;   // todo - make K a config option
  int             r          = *(odev->block_w.shm_r);
  int             w          = *(odev->block_w.shm_w);
  uint32_t        index, sent_count = sdh_shm_get_sent_count(r, w);
  
  if (r < 0) {
    log_debug("SHM queue has 0 entries - so no need to try to delete any entries");
    *(odev->block_w.shm_r) = 0;
    r = 0;
  }
  else if (PAGES_MAX - sent_count >= K) {
    log_debug("SHM queue has %d entries - so no need to try to delete any entries", sent_count);
  }
  else {       // less than K queue spots available for use
    log_debug("SHM queue has %d entries - so try to delete entries from index %d to %d", sent_count, w, (w + K - 1) % PAGES_MAX);
    index = w;
    for (i=0; i<K; i++) {
      shm_invalid_one(odev, index, w, sent_count, c);
      mod_inc(&index);
    }
  }
  return (w);
}

/* Poll input device and return count of number of messages (w - r) */
int sdh_shm_poll (device *idev) {
  uint32_t r      = *(idev->block_r.shm_r);
  uint32_t w      = *(idev->block_r.shm_w);
  uint32_t next   = get_next_index(r, w, 0);
  uint32_t count  = get_read_count(next, w);
  
  log_trace("Poll Shared Memory (%s %s): count=%d w=%d R=%d r=%d wo=0x%x ro=0x%x", idev->id, idev->path, count,  *(idev->block_r.shm_w), next, *(idev->block_r.shm_r), idev->addr_off_w, idev->addr_off_r);
  return (count);
}

/* Get Shared Memory message pointer and control info from idev into PDU */
/* Note: Data is not copied into a buffer - PDU points into Shared Memory */
int pdu_from_sdh_sm_v1 (pdu *out, device *idev) {
  uint32_t  r      = *(idev->block_r.shm_r);
  uint32_t  w      = *(idev->block_r.shm_w);
  int       index  = get_next_index(r, w, 1);
  int       len    = (idev->block_r.shm_l)[r];

  log_trace("Read Shared Memory (%s %s): [r=%d R=%d w=%d] len=%d wo=0x%x ro=0x%x", idev->id, idev->path, r, index, w, len, idev->addr_off_w, idev->addr_off_r);
//  log_devs_debug(idev, __func__);
  log_shm_debug("SHM-r", &(idev->block_r));
  out->data_len = len;
  tag_cp(&(out->psel.tag), &(idev->block_r.tag[r]));
  out->data = (idev->block_r.shm_d)[index];
//  log_fatal("%p %0x02x %d\n", out->data, (idev->block_r.shm_d)[index][0], len);
  return (len);
}

/* check write timing */
void shm_synchronizer (device *odev, uint32_t w, uint64_t c) {
//  uint64_t        duration;
  uint64_t        t, duration;
  int             index;
   
//  uint64_t        t = odev->local_w.shm_t[w];
//  int             v = odev->local_w.shm_v[w];
//  int             a = odev->guard_time_aw;
//  int             b = odev->guard_time_bw;
//  uint32_t        count = sdh_shm_get_sent_count(r, w);
  
  index = shm_invalid_last_K_in_Q(odev, c);
  t = odev->local_w.shm_t[index];
  duration = c - t;
  log_debug("SHM index=%d duration (%ld) vs b (%d) ", index, duration, odev->guard_time_bw);
}

/* Write PDU message (in) into Shared Memory (odev) with tag 'otag' */
int pdu_into_sdh_sm_v1(device *odev, pdu *in, gaps_tag *otag) {
  int             w   = *(odev->block_w.shm_w);
  int             len = in->data_len;
  struct timeval  tv;
  uint64_t        c;
  
  gettimeofday(&tv,NULL);
  c = (tv.tv_usec * 1000) + (tv.tv_sec  * 1000000000);
  shm_synchronizer(odev, w, c);
  
  memcpy((odev->block_w.shm_d)[w], (uint8_t *) in->data, len);
  odev->block_w.shm_l[w] = len;
  tag_cp(&(odev->block_w.tag[w]), otag);
  odev->local_w.shm_t[w] = c;
  odev->local_w.shm_v[w] = 1;
  mod_inc(odev->block_w.shm_w);

//  log_devs_debug(odev, __func__);
  log_shm_debug("SHM-w", &(odev->block_w));
  return (len);
}

//if (msync((void *)odev->block_w.shm_r, MAP_SIZE, MS_SYNC) < 0) {
//  perror("msync");
//  exit(1);
//}
