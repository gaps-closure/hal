/*
 * Convert between PDU and Shared Memory
 *   August 2021, Perspecta Labs
 */

#include "hal.h"
#include "packetize_sdh_sm_v1.h"
#include "map.h"            /* get data_print */

/* Print M1 Packet */
void sdh_sm_map_print_one(sdh_sm_map *p) {
  fprintf(stderr, "%s: ", __func__);
  fprintf(stderr, "mux=%u ",  p->tag.mux);
  fprintf(stderr, "sec=%u ",  p->tag.sec);
  fprintf(stderr, "typ=%u ",  p->tag.typ);
  fprintf(stderr, "address = %p", p->shm_ctl);
  fprintf(stderr, "\n");
}

/* Print Control */
void sdh_shm_print(device *d) {
  int        index, i, j, r, w, *v;
  uint32_t   limit, len;
  uint64_t  *t;
    
  t = d->shm_t;
  v = d->shm_v;
  r = *(d->shm_r);
  w = *(d->shm_w);
  
  fprintf(stderr, "SHM: guard ms [a=%lu ", d->shm_a);
  fprintf(stderr, "b=%lu] v[t]=",               d->shm_b);
  for (i=0; i<PAGES_MAX; i++) {
    fprintf(stderr, "%d(t=%ld) ", v[i], t[i]);
  }
  fprintf(stderr, "[r=%u w=%u]", r, w);
  limit = (w-r) % PAGES_MAX;
  for (i=0; i<limit; i++) {
    index = (i + r) % PAGES_MAX;
    len = (d->shm_l)[i];
    fprintf(stderr, "[%d: l=%d ", index, len);
    for (j=0; j<len; j++) {
      fprintf(stderr, "%02x", (d->shm_d)[index][j]);
      if ((j%4)==3) fprintf(stderr, " ");
    }
    fprintf(stderr, "]\n");
  }
  fprintf(stderr, "\n");
}

/* Put closure packet (in) into internal HAL PDU structure (out) */
int pdu_from_sdh_sm_v1 (pdu *out, uint8_t *in, int len_in) {
  sdh_ha_v1  *pkt = (sdh_ha_v1 *) in;
  
  out->data_len = pkt->data_len;
  tag_cp(&(out->psel.tag), &(pkt->tag));
  out->data = pkt->data;   /* TODO_PDU_PTR */
  return (out->data_len);
}

void pdu_into_sdh_sm_v1(uint8_t *out, device *odev, int pkt_len) {
  struct timeval tv;
  uint64_t       ns_now;
  uint32_t       w;

//  sdh_shm_print(odev);
  gettimeofday(&tv,NULL);
  ns_now = (tv.tv_usec * 1000) + (tv.tv_sec  * 1000000000);
  w = *(odev->shm_w);
  log_trace("Shared Memory time (nsec) c=%lu o=%lu)\n", ns_now, odev->shm_t[w]);
  memcpy((odev->shm_d)[w], out, pkt_len);
  odev->shm_l[w] = pkt_len;
  *(odev->shm_w) = ++w;
  sdh_shm_print(odev);
}
