/* Define GAPS Packet Format for SDH BW */

#define PKT_G1_ADU_SIZE_MAX  1000

/* BW Compressed Mode packet */
typedef struct _pkt_g1 {
  uint32_t  message_tag_ID;        /* App Mux */
  uint16_t  data_len;              /* Length (in bytes) */
  uint16_t  crc16;                 /* XXX: what is the generator polynomial? */
  uint8_t   data[PKT_G1_ADU_SIZE_MAX];
} pkt_g1;

void pdu_from_pkt_g1 (pdu *, uint8_t * , int);
int  pdu_into_pkt_g1 (uint8_t *, pdu *, gaps_tag *);
