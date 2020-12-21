/* Define GAPS Packet Format for SDH BE */
#define SDH_BE_V0_MAX_TLVS         4
#define SDH_BE_V0_ADU_SIZE_MAX   236

/* TLV */
typedef struct _tlv_sdh_be_v0 {
  uint32_t  data_tag;                       /* Type of data */
  uint32_t  data_len;                       /* Length (in bytes) */
  uint8_t   data[SDH_BE_V2_ADU_SIZE_MAX];   /* 256 - 5*4 bytes */
} tlv_sdh_be_v0;

/* packet */
typedef struct _pkt_sdh_be_v0 {
  uint32_t        session_tag;              /* App Mux */
  uint32_t        message_tag;              /* Security */
  uint32_t        message_tlv_count;        /* TLV count (1 for demo) */
  tlv_sdh_be_v0   tlv[SDH_BE_V0_MAX_TLVS];  /* TLV */
} pkt_sdh_be_v0;

/* exported functions */
void pdu_from_sdh_be_v0 (pdu *, uint8_t *, int);
int  pdu_into_sdh_be_v0 (uint8_t *, pdu *, gaps_tag *);
