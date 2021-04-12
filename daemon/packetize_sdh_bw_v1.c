/*
 * Convert between Internal HAL PDU and SDH BW packet
 *   April 2021, Perspecta Labs
 */

#include "hal.h"
#include "crc.h"
#include "packetize_sdh_bw_v1.h"
#include "map.h"            /* get data_print */
#include "packetize_join.h"

/* calculate packet's crc */
uint16_t sdh_bw_v1_crc_calc(sdh_bw_v1 *pkt) {
  return (crc16((uint8_t *) pkt, sizeof(pkt->message_tag_ID) + sizeof (pkt->data_len)));
}

/* get size of packet (header length + data length) */
int get_packet_length_sdh_bw_v1(sdh_bw_v1 *pkt) {
  return (sizeof(pkt->message_tag_ID) + sizeof(pkt->data_len) + sizeof(pkt->crc16) + ntohs(pkt->data_len));
}

/* Copy from packet (input or saved) into internal PDU */
void packet_2_pdu(pdu *out, sdh_bw_v1 *in) {
  out->data_len  = ntohs(in->data_len);
  out->psel.ctag = ntohl(in->message_tag_ID);
  out->data      = in->data;
}

/* Put data from buf (using sdh_bw_v1 model) into internal HAL PDU */
int pdu_from_sdh_bw_v1 (pdu *out, uint8_t *in, int len_in) {
  sdh_bw_v1  *pkt_ptr = (sdh_bw_v1 *) in;
  uint8_t    *sbuf = in;
  uint16_t    crc_pkt, crc_regen;
  int         len_pkt, frame_delim, rv;
  bool        start_of_packet;
  
  crc_pkt         = ntohs(pkt_ptr->crc16);
  crc_regen       = sdh_bw_v1_crc_calc(pkt_ptr);
  frame_delim     = (ntohl(pkt_ptr->message_tag_ID)) & 0xff000000;
  len_pkt         = get_packet_length_sdh_bw_v1(pkt_ptr);
  start_of_packet = (crc_pkt == crc_regen) && (frame_delim == 0);
  if (start_of_packet)  log_trace("start=%d, len [in=%d pkt=%d], crc [pkt=%02x out=%02x]. delim=%d\n", start_of_packet, len_in, len_pkt, crc_pkt, crc_regen, frame_delim);
  else                  log_trace("start=%d, len [in=%d pkt=XX]\n", start_of_packet, len_in);
  rv = packet_parser(in, len_in, len_pkt, out, &sbuf, start_of_packet);
  if (rv > 0) packet_2_pdu(out, (sdh_bw_v1 *) sbuf);
  return (rv);
}

/* Put data into buf (using sdh_bw_v1 model) from internal HAL PDU */
/* Returns length of buffer */
int pdu_into_sdh_bw_v1 (uint8_t *out, pdu *in, uint32_t ctag) {
  sdh_bw_v1    *pkt_ptr = (sdh_bw_v1 *) out;
  uint16_t  len = (uint16_t) in->data_len;

  pkt_ptr->message_tag_ID = htonl(ctag);
  pkt_ptr->data_len = htons(len);
  pkt_ptr->crc16 = htons(sdh_bw_v1_crc_calc(pkt_ptr));
  memcpy((char *) pkt_ptr->data, (char *) in->data, in->data_len);
  return (get_packet_length_sdh_bw_v1(pkt_ptr));
}
