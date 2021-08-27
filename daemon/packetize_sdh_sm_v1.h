/* Define GAPS 'Packet' Format for Shared Memory */

#define PAGE_LEN_MAX 4096
#define PAGES_MAX    64

/* Control Information about one way connection */
typedef struct _shm_ctl_info {
  uint64_t  time_guard;          /* Guard time before writing (nanoseconds) */
  uint64_t  time_read_allow;     /* Min time data avaiable to read (nanoseconds) */
  uint32_t  index_max;           /* Number of available pages */
  uint32_t  index_oldest;        /* Index for oldest, still available, page */
  uint32_t  index_newest;        /* Index for last page written */
} shm_ctl_info;

/* Control Information for one Page */
typedef struct _shm_page_info {
  uint8_t   *addr_off;           /* Address offset to start of this page */
  uint32_t  data_len;            /* Data Length (in bytes) for this page */
  uint64_t  timestamp;           /* time in nsnoseconds when written */
} _shm_page_info;

/* Shared memory (control and data) for one flow */
typedef struct _sdh_sm_v1 {
  shm_ctl_info    ctl;
  shm_page_info   page_info[PAGES_MAX];
  uint8_t         page_data[PAGES_MAX][PAGE_LEN_MAX];
} sdh_sm_v1;

/* Map from tag to Shared Memory address (when device is opened) */
typedef struct _sdh_sm_map {
  gaps_tag    tag;        /* Flow tag */
  sdh_sm_v1  *shm_addr;
  sdh_sm_map *next;       /* saved as a linked list */
} sdh_sm_map;

                                    
int  pdu_from_sdh_sm_v1 (pdu *, uint8_t *, int);
int  pdu_into_sdh_sm_v1 (uint8_t *, pdu *, gaps_tag *);
