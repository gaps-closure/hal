#define PKT_M1_MAX_TLVS         4
#define PKT_M1_ADU_SIZE_MAX   236
#define PKT_M2_ADU_SIZE_MAX   220
#define PKT_G1_ADU_SIZE_MAX  1000

/* HAL PDU */
typedef struct _pdu {
  selector  psel;                   /* Input device and tag info */
  size_t    data_len;
  uint8_t   data[ADU_SIZE_MAX_C];   /* opaque to HAL - serialized by APP */
} pdu;

/* Define HAL Network Packet Formats */

/* Mercury TLV */
typedef struct _tlv_m1 {
  uint32_t  data_tag;     /* Type (e.g., DATA_PAYLOAD_1) */
  uint32_t  data_len;     /* Length (in bytes) */
  uint8_t   data[PKT_M1_ADU_SIZE_MAX];    /* Value (up to 236 (256 - 5*4) bytes of payload) */
} tlv_m1;

/* Mercury packet */
typedef struct _pkt_m1 {
  uint32_t  session_tag;           /* App Mux */
  uint32_t  message_tag;           /* Security */
  uint32_t  message_tlv_count;     /* TLV count (1 for demo) */
  tlv_m1    tlv[PKT_M1_MAX_TLVS];  /* TLV */
} pkt_m1;

/* Mercury TLV */
typedef struct _tlv_m2 {
  uint32_t  data_tag;         /* Type (e.g., DATA_PAYLOAD_1) */
  /* Avoid uint64_t for time fields in order to align struct */
  uint32_t  gaps_time;        /* ILIP GAPS Time (set by ILIP) */
  uint32_t  gaps_time_us;     /* ILIP GAPS Time (set by ILIP) */
  uint32_t  linux_time;       /* Secs since 1970 - Used/Set by Application */
  uint32_t  linux_time_us;    /* micro-seconds  */
  uint32_t  data_len;         /* Length (in bytes) */
  uint8_t   data[PKT_M2_ADU_SIZE_MAX];    /* Value (up to 220) */
} tlv_m2;

/* Mercury packet */
typedef struct _pkt_m2 {
  uint32_t  session_tag;           /* App Mux */
  uint32_t  message_tag;           /* Security */
  uint32_t  message_tlv_count;     /* TLV count (1 for demo) */
  tlv_m2    tlv[PKT_M1_MAX_TLVS];  /* TLV */
} pkt_m2;

/* GE Compressed Mode packet */
typedef struct _pkt_g1 {
  uint32_t  message_tag_ID;        /* App Mux */
  uint16_t  data_len;              /* Length (in bytes) */
  uint16_t  crc16;                 /* XXX: what is the generator polynomial? */
  uint8_t   data[PKT_G1_ADU_SIZE_MAX];
} pkt_g1;

extern void pdu_from_packet(pdu *, uint8_t *, int, device *);
extern void pdu_into_packet(uint8_t *, pdu *, int *, gaps_tag *, const char *);
