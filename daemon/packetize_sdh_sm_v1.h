/* Define GAPS Shared Memory Format (does not define a packet) */

/* Control Information about SHM connection shared with receiver */
typedef struct _sm_ctl_global {
  uint32_t  index_read_oldest;   /* Index (r) of oldest available page */
  uint32_t  index_write_next;    /* Index (w) for next page written */
} sm_ctl_global;

/* Shared memory block (for one flow) shared with receiver */
typedef struct _sdh_sm_v1 {
  sm_ctl_global  ctl;                  /* Control Information */
  uint32_t       data_len[PAGES_MAX];  /* Data Length (in bytes) for each page */
  uint8_t        page_data[PAGES_MAX][PAGE_LEN_MAX];  /* data pages (up to PAGE_LEN_MAX) */
} sdh_sm_v1;

/* Control Information about one way connection local to writer */
typedef struct _shm_ctl_local {
  uint64_t    guard_time_aw;       /* Min data duration after write (milliseconds) */
  uint64_t    guard_time_bw;       /* Guard time before writing (milliseconds) */
  int         valid[PAGES_MAX];    /* Valid Flag per page (0=invalid, 1=valid) */
  uint64_t    time_write_ns[PAGES_MAX];    /* nsnoseconds since written/deleted */
  sdh_sm_v1   *shm;                /* Shared memory Block */
} shm_ctl_local;

/* Map from tag to Shared Memory address (when device is opened) */
typedef struct _sdh_sm_map {
  gaps_tag             tag;       /* Flow tag */
  shm_ctl_local       *shm_ctl;   /* Local and global shared memory */
  struct _sdh_sm_map  *next;      /* linked list pointer */
} sdh_sm_map;

// int  pdu_from_sdh_sm_v1 (pdu *, uint8_t *, int);
// int  pdu_into_sdh_sm_v1 (uint8_t *, pdu *, gaps_tag *);
void pdu_into_sdh_sm_v1(uint8_t *out, device *odev, int pkt_len);

void sdh_shm_print(device *d);
