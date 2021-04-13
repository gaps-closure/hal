#define SDH_DELIM  0x72ca

int  packet_parser(uint8_t *in, int len_in, int len_pkt, pdu *out, uint8_t **sbuf, bool start_of_packet);
