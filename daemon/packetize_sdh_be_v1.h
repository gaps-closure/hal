/* Define GAPS Packet Format for SDH BE */

#define SDH_BE_V1_MAX_TLVS         4
#define SDH_BE_V1_ADU_SIZE_MAX   220

/* TLV */
typedef struct _tlv_sdh_be_v1 {
  uint32_t  data_tag;                       /* Type (e.g., DATA_PAYLOAD_1) */
  uint32_t  gaps_time;                      /* ILIP GAPS Time (set by ILIP) */
  uint32_t  gaps_time_us;                   /*  -- uint64_t causes struct padding */
  uint32_t  linux_time;                     /* Time Used/Set by Application */
  uint32_t  linux_time_us;                  /* Secs & micro-seconds since 1970 */
  uint32_t  data_len;                       /* Length (in bytes) */
  uint8_t   data[SDH_BE_V1_ADU_SIZE_MAX];   /*  */
} tlv_sdh_be_v1;

/* packet */
typedef struct _pkt_sdh_be_v1 {
  uint32_t        session_tag;              /* App Mux */
  uint32_t        message_tag;              /* Security */
  uint32_t        message_tlv_count;        /* TLV count (1 for demo) */
  tlv_sdh_be_v1   tlv[SDH_BE_V1_MAX_TLVS];  /* TLV */
} pkt_sdh_be_v1;

/* exported functions */
int  pdu_from_sdh_be_v1 (pdu *, uint8_t *, int);
int  pdu_into_sdh_be_v1 (uint8_t *, pdu *, gaps_tag *);
