/* Define GAPS Shared Memory Format (does not define a packet) */

/* Shared memory block (for one flow) shared with receiver */
typedef struct _sdh_sm_v1 {
  uint32_t  index_read_oldest;   /* Index (r) of oldest available page */
  uint32_t  index_write_next;    /* Index (w) for next page written */
  uint32_t  data_len[PAGES_MAX];  /* Data Length (in bytes) for each page */
  gaps_tag  tag[PAGES_MAX];       /* gaps_tag for each page */
  uint8_t   page_data[PAGES_MAX][PAGE_LEN_MAX];  /* data pages (up to PAGE_LEN_MAX) */
} sdh_sm_v1;

int  sdh_shm_poll (device *idev, uint32_t *next);
int  pdu_from_sdh_sm_v1 (pdu *out, device *idev);
int  pdu_into_sdh_sm_v1(device *odev, pdu *in, gaps_tag *otag);
int  sdh_shm_get_sent_count(int r, int w);
