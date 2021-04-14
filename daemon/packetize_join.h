#define SDH_DELIM  0x72ca

/* packet reassembly structure (static storage instance in each packet type read */
typedef struct _pkt_reaas {
  uint8_t buf[ADU_SIZE_MAX_C];
  int     buf_len;
  int     pkt_len;
} pkt_reaas;

int  packet_parser(uint8_t *in, int len_in, int len_pkt, uint8_t **sbuf, bool start_of_packet, pkt_reaas *prs);
