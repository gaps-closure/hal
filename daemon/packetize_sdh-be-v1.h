/* Define GAPS Packet Format for SDH BE */

#define PKT_M1_MAX_TLVS         4
#define PKT_M1_ADU_SIZE_MAX   236
#define PKT_M2_ADU_SIZE_MAX   220

/* v1 TLV */
typedef struct _tlv_m1 {
  uint32_t  data_tag;     /* Type (e.g., DATA_PAYLOAD_1) */
  uint32_t  data_len;     /* Length (in bytes) */
  uint8_t   data[PKT_M1_ADU_SIZE_MAX];    /* Value (up to 236 (256 - 5*4) bytes of payload) */
} tlv_m1;

/* v1 packet */
typedef struct _pkt_m1 {
  uint32_t  session_tag;           /* App Mux */
  uint32_t  message_tag;           /* Security */
  uint32_t  message_tlv_count;     /* TLV count (1 for demo) */
  tlv_m1    tlv[PKT_M1_MAX_TLVS];  /* TLV */
} pkt_m1;

/* v2 TLV */
typedef struct _tlv_m2 {
  uint32_t  data_tag;         /* Type (e.g., DATA_PAYLOAD_1) */
  /*  */
  uint32_t  gaps_time;        /* ILIP GAPS Time (set by ILIP) */
  uint32_t  gaps_time_us;     /*  -- Not uint64_t to avoid struct padding */
  uint32_t  linux_time;       /* Secs since 1970 - Used/Set by Application */
  uint32_t  linux_time_us;    /* micro-seconds  */
  uint32_t  data_len;         /* Length (in bytes) */
  uint8_t   data[PKT_M2_ADU_SIZE_MAX];    /* Value (up to 220) */
} tlv_m2;

/* v2 packet */
typedef struct _pkt_m2 {
  uint32_t  session_tag;           /* App Mux */
  uint32_t  message_tag;           /* Security */
  uint32_t  message_tlv_count;     /* TLV count (1 for demo) */
  tlv_m2    tlv[PKT_M1_MAX_TLVS];  /* TLV */
} pkt_m2;


void pdu_from_pkt_m1 (pdu *, uint8_t *, int);
void pdu_from_pkt_m2 (pdu *, uint8_t *, int);
int  pdu_into_pkt_m1 (uint8_t *, pdu *, gaps_tag *);
int  pdu_into_pkt_m2 (uint8_t *, pdu *, gaps_tag *);
