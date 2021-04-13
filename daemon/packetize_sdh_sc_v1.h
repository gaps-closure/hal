/* Define GAPS Packet Format for serail line communication */

/* CLOSURE packet for serial line comms (e.g., socat-pty) */
typedef struct _sdh_sc_v1 {
  gaps_tag  tag;
  uint16_t  delim;                 /* start of frame marker */
  uint16_t  crc16;                 /* CRC frame check */
  uint32_t  data_len;              /* Data length */
  uint8_t   data[ADU_SIZE_MAX_C];  /* Data (up to ADU_SIZE_MAX_C bytes) */
} sdh_sc_v1;

int  pdu_from_sdh_sc_v1 (pdu *, uint8_t *, int);
int  pdu_into_sdh_sc_v1 (uint8_t *, pdu *, gaps_tag *);
