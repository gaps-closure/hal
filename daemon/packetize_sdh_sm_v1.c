/*
 * Copy data and control information between PDU and Shared Memory
 *   October 2021, Peraton Labs
 */

#include "hal.h"
#include "device_open.h"
#include "packetize_sdh_sm_v1.h"
#include "map.h"            /* get data_print */
#include <sys/mman.h>

#define SHM_RECYCLE_COUNT 3

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
 Shared Memory Map Functions
 =================================================================*/
/* Map Shared Memory Physical address to a Virtual Address */
void *shm_mmap(int fd, off_t target) {
  void  *shm_addr;
  off_t  offset=0;

  offset = (target & ~MAP_MASK);
//  log_trace("SHM target=0x%lx (%lu) offset=0x%lx", target, target, offset);
  if ((shm_addr = mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED,
          fd, offset)) == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }
  close(fd);   /* don't need the fd anymore, the mapping stays around */
  return (shm_addr);
}

/* open a SHM device */
void sdh_shm_open(int *fd, const char *path) {
  if ((*fd = open(path, O_RDWR)) < 0) {
    perror(path);
    exit(1);
  }
}

//  int dev_is_bidirect = strcmp(d->path_r, d->path_w);       /* 0 means bidirectional */
//    if (dev_is_bidirect == 0)  d->read_fd = d->write_fd;
/* Open SHM device */
void *shm_dev_ON(device *d, int *fd, const char *path, off_t target) {
  void *shm_addr=NULL;
  if (*fd < 0) {
    sdh_shm_open(fd, path);           /* Open and update fd */
    shm_addr = shm_mmap(*fd, target);
    log_trace("New Shared Memory address=0x%x (olds: (0x%x) (0x%x))", shm_addr, d->block_r.shm_r, d->block_w.shm_r);
  }
  return(shm_addr);
}

/* Open SHM READ device */
void *shm_dev_ON_r(device *d) {
  log_trace("Turn SHM-R %s ON (%s fdr=%d, %s fdw=%d) if not already on", d->id, d->path_r, d->read_fd, d->path_w, d->write_fd);
  return (shm_dev_ON(d, &(d->read_fd), d->path_r, d->addr_off_r));
}

/* Open SHM WRITE device */
void *shm_dev_ON_w(device *d) {
  log_trace("Turn SHM-W %s ON (%s fdr=%d, %s fdw=%d) if not already on", d->id, d->path_r, d->read_fd, d->path_w, d->write_fd);
  return (shm_dev_ON(d, &(d->write_fd), d->path_w, d->addr_off_w));
}

/* Close SHM device */
void shm_dev_OFF(device *d) {
  void *shm_addr = (void *) (d->block_r.shm_r);
  
  munmap(shm_addr, MAP_SIZE);
  d->read_fd = -1;
  log_trace("Turn SHM-R %s OFF (%s fdr=%d, %s fdw=%d)", d->id, d->path_r, d->read_fd, d->path_w, d->write_fd);
}
      
/*=================================================================
 Read Functions
 =================================================================*/
/* Poll input device and return count of number of messages (w - r) */
int sdh_shm_poll (device *d, uint32_t *next) {
  uint32_t  r, w, count;
            
  shm_dev_ON_r(d);
//  shm_sync(&(d->block_r));
  r     = *(d->block_r.shm_r);
  w     = *(d->block_r.shm_w);
  *next = get_next_index(r, w, 0);
  count = get_read_count(*next, w);
  log_trace("Poll Shared Memory (%s %s): count=%d w=%d R=%d r=%d wo=0x%x ro=0x%x", d->id, d->path, count,  w, *next, r, d->addr_off_w, d->addr_off_r);
  return (count);
}

/* Get Shared Memory message pointer and control info from input device (d) into PDU */
/* Note: Data is not copied into a buffer - PDU points into Shared Memory */
int pdu_from_sdh_sm_v1 (pdu *out, device *d) {
  uint32_t  r      = *(d->block_r.shm_r);
  uint32_t  w      = *(d->block_r.shm_w);
  int       index  = get_next_index(r, w, 1);
  int       len    = (d->block_r.shm_l)[r];

  log_trace("Read Shared Memory (%s %s): [r=%d R=%d w=%d] len=%d wo=0x%x ro=0x%x", d->id, d->path, r, index, w, len, d->addr_off_w, d->addr_off_r);
//  log_devs_debug(d, __func__);
  log_shm_trace("SHM-r", &(d->block_r), NULL);
  out->data_len = len;
  tag_cp(&(out->psel.tag), &(d->block_r.tag[r]));
  out->data = (d->block_r.shm_d)[index];
//  log_fatal("%p %0x02x %d\n", out->data, (d->block_r.shm_d)[index][0], len);
  return (len);
}

/*=================================================================
 Write Functions
 =================================================================*/
/* flush Snared Memory Block (shm_sync(&(d->block_w)) */
void shm_sync(dev_shm_ptrs *p) {
//  printf("p=%p, len=%ld, MS_SYNC=%d\n", p->shm_r, SHM_BLOCK_SIZE, MS_SYNC);
  if (msync((void *) (p->shm_r), (size_t) SHM_BLOCK_SIZE, MS_SYNC) < 0) {
    perror("msync");
    exit(1);
  }
}

/* shm_delete_page */
void shm_delete_page(device *d, uint32_t index, uint64_t c) {
  d->local_w.shm_v[index] = 0;
  d->local_w.shm_t[index] = c;
  mod_inc(d->block_w.shm_r);
}

/* remove one queue entry if expired (c - t > a) or no more free pages */
void shm_delete_page_test(device *d, uint32_t index, int w, uint32_t sent_count, uint64_t c) {
  int       v;
  uint64_t  a, t, duration;

  a = ((uint64_t) d->guard_time_aw) * 1000000;    /* after write hold time in nanoseconds */
  v = d->local_w.shm_v[index];
  t = d->local_w.shm_t[index];
  duration = c - t;
  log_debug("Test A on index %d v=%d w=%d", index, v, w);
  log_trace("duration=(c-t)=(%ld - %ld) = %ld", c, t, duration);
  log_trace("ti_delta=(d-a)=(%ld - %ld) = %ld", duration, a, (duration - a));
  if (v == 1) {       // Not already deleted
    if (duration > a) {
      log_debug("Delete expired SHM page (index=%d w=%d delta=%lu r=%d)", index, w, duration, *(d->block_w.shm_r));
      shm_delete_page(d, index, c);
      log_debug("r=%d", *(d->block_w.shm_r));
    }
    else {
      if ((sent_count == PAGES_MAX) && (index == w)) {
        log_debug("Delete oldest SHM page (index=%d w=%d delta=%lu) as no expired page", index, w, duration);
        shm_delete_page(d, index, c);
      }
    }
  }
}

/* Remove (v=0) up to the SHM_RECYCLE_COUNT (K) oldest pages in queue */
/* Return index to oldest page (w) or -1 if no pages available */
int shm_invalid_last_K_in_Q(device *d, uint64_t c) {
  int         i;
  int         r = *(d->block_w.shm_r);
  int         w = *(d->block_w.shm_w);
  uint32_t    index, sent_count;
  
  sent_count = sdh_shm_get_sent_count(r, w);
  log_trace("Counts: Wr=%d Ww=%d Rr=%d Rw=%d", *(d->block_w.shm_r), *(d->block_w.shm_w), *(d->block_r.shm_r), *(d->block_r.shm_w));
  if (r < 0) {
    log_trace("SHM queue has 0 entries - so initialize (no need to try to delete any entries)");
    *(d->block_w.shm_r) = 0;
    r = 0;
  }
  else if (PAGES_MAX - sent_count >= SHM_RECYCLE_COUNT) {
    log_trace("SHM queue has %d entries - so no need to try to delete any entries", sent_count);
  }
  else {       // less than SHM_RECYCLE_COUNT queue spots available for use
    log_trace("SHM queue has %d entries - so try to delete entries from index %d to %d", sent_count, w, (w + SHM_RECYCLE_COUNT - 1) % PAGES_MAX);
    index = w;
    for (i=0; i<SHM_RECYCLE_COUNT; i++) {
      shm_delete_page_test(d, index, w, sent_count, c);
      mod_inc(&index);
    }
  }
  log_trace("Counts: Wr=%d Ww=%d Rr=%d Rw=%d", *(d->block_w.shm_r), *(d->block_w.shm_w), *(d->block_r.shm_r), *(d->block_r.shm_w));
  shm_sync(&(d->block_w));
  return (w);
}

/* check write timing */
int shm_check_next_write_index(device *d, uint64_t c) {
  uint64_t        t, duration;
  int             write_index, v;
  
  write_index = shm_invalid_last_K_in_Q(d, c);
  if (write_index >= 0) {
    t        = d->local_w.shm_t[write_index];
    v        = d->local_w.shm_v[write_index];
    duration = c - t;
    log_trace("todo - SHM index=%d v=%d: duration (%ld ns) vs b (%d ms) ", write_index, v, duration, d->guard_time_bw);
  }
  else {
    log_warn("SHM queue has no valid index - Should not reach this code with auto-recycle policy");
  }
  return (write_index);
}

/* Write PDU message (in) into Shared Memory device (d) with tag 'otag' */
int pdu_into_sdh_sm_v1(device *d, pdu *in, gaps_tag *otag) {
  int             index, len = in->data_len;
  struct timeval  tv;
  uint64_t        c;
  
  shm_dev_ON_r(d);
  gettimeofday(&tv,NULL);
  c = (tv.tv_usec * 1000) + (tv.tv_sec  * 1000000000);
  index = shm_check_next_write_index(d, c);
  if (index >= 0) {
    log_trace("Copy len=%d data to SHM index=%d (%p -> %p) ", len, index, (uint8_t *) in->data, (d->block_w.shm_d)[index]);
    memcpy((d->block_w.shm_d)[index], (uint8_t *) in->data, len);
    log_trace("FAILS ABOVE IF PAGES_MAX=256 and INDEX=255, check block size,...");
    d->block_w.shm_l[index] = len;
    tag_cp(&(d->block_w.tag[index]), otag);
    d->local_w.shm_t[index] = c;
    d->local_w.shm_v[index] = 1;       // valid data
    mod_inc(d->block_w.shm_w);
//log_devs_debug(d, __func__);
    log_shm_trace("SHM-w", &(d->block_w), &(d->local_w));
    shm_sync(&(d->block_w));
    log_debug("HAL writes (comms=%s, format=%s) into %s: len=%d idx=%d", d->comms, d->model, d->id, len, index);
    return (len);
  }
  log_warn("SHM queue has no valid index - Should not reach this code with auto-recycle policy");
  return (0);
}
