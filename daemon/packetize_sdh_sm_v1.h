/* Define GAPS Shared Memory Format (does not define a packet) */

/* Shared memory block (for one flow) shared with receiver */
typedef struct _sdh_sm_v1 {
  uint32_t  index_read_oldest;   /* Index (r) of oldest available page */
  uint32_t  index_write_next;    /* Index (w) for next page written */
  uint32_t  data_len[PAGES_MAX];  /* Data Length (in bytes) for each page */
  gaps_tag  tag[PAGES_MAX];       /* gaps_tag for each page */
  uint8_t   page_data[PAGES_MAX][PAGE_LEN_MAX];  /* data pages (up to PAGE_LEN_MAX) */
} sdh_sm_v1;

// int  pdu_from_sdh_sm_v1 (pdu *, uint8_t *, int);
// int  pdu_into_sdh_sm_v1 (uint8_t *, pdu *, gaps_tag *);
void pdu_into_sdh_sm_v1(uint8_t *out, device *odev, int pkt_len, gaps_tag *otag);
int pdu_from_sdh_poll (device *idev);
void sdh_shm_print(device *d);
