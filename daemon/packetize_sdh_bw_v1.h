/* Define GAPS Packet Format for SDH BW */

#define PKT_G1_ADU_SIZE_MAX  1000

/* BW Compressed Mode packet */
typedef struct _sdh_bw_v1 {
  uint32_t  message_tag_ID;        /* App Mux */
  uint16_t  data_len;              /* Length (in bytes) */
  uint16_t  crc16;                 /* XXX: what is the generator polynomial? */
  uint8_t   data[PKT_G1_ADU_SIZE_MAX];
} sdh_bw_v1;

void pdu_from_sdh_bw_v1 (pdu *, uint8_t * , int);
int  pdu_into_sdh_bw_v1 (uint8_t *, pdu *, uint32_t);
