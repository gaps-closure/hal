/*
 * Convert between HAL PDU and SDH BW packet
 *   January 2021, Perspecta Labs
 *
 * v1 (April 2020) uses compressed tags in the packet header
 */

/* Convert between Internal HAL PDU and sdh_bw_v1 packets */

#include "hal.h"
#include "crc.h"
#include "packetize_sdh_bw_v1.h"
#include "map.h"            /* get data_print */

/* Print Packet */
void sdh_bw_v1_print(sdh_bw_v1 *p) {
  fprintf(stderr, "%s: ", __func__);
  fprintf(stderr, "ctag=%u ", ntohl(p->message_tag_ID));
  fprintf(stderr, "crc=%02x ", ntohs(p->crc16));
  data_print("Data", p->data, ntohs(p->data_len));
  fprintf(stderr, "\n");
}

/* calculate packet's crc */
uint16_t sdh_bw_v1_crc_calc(sdh_bw_v1 *pkt) {
  return (crc16((uint8_t *) pkt, sizeof(pkt->message_tag_ID) + sizeof (pkt->data_len)));
}

/* get size of packet (= header length + data length) */
int get_packet_length_sdh_bw_v1(sdh_bw_v1 *pkt, size_t data_len) {
  return (sizeof(pkt->message_tag_ID) + sizeof(pkt->data_len) + sizeof(pkt->crc16) + data_len);
}

/* Put data from buf (using sdh_bw_v1 model) into internal HAL PDU */
int pdu_from_sdh_bw_v1 (pdu *out, uint8_t *in, int len_in) {
  sdh_bw_v1    *pkt = (sdh_bw_v1 *) in;

  out->data_len     = ntohs(pkt->data_len);
  if (get_packet_length_sdh_bw_v1(pkt, out->data_len) > len_in)  return (-1);
  
  out->psel.ctag = ntohl(pkt->message_tag_ID);
// fprintf(stderr, "%s: ctag=%06x crc: in=%02x recalc=%02x\n", __func__, out->psel.ctag, ntohs(pkt->crc16), sdh_bw_v1_crc_calc(pkt));
//  memcpy (out->data, pkt->data, out->data_len);    /* TODO_PDU_PTR */
  out->data = pkt->data;    /* TODO_PDU_PTR */
  return (get_packet_length_sdh_bw_v1(pkt, out->data_len));
}

/* Put data into buf (using sdh_bw_v1 model) from internal HAL PDU */
/* Returns length of buffer */
int pdu_into_sdh_bw_v1 (uint8_t *out, pdu *in, uint32_t ctag) {
  sdh_bw_v1    *pkt = (sdh_bw_v1 *) out;
  uint16_t  len = (uint16_t) in->data_len;

  pkt->message_tag_ID = htonl(ctag);
  pkt->data_len = htons(len);
  pkt->crc16 = htons(sdh_bw_v1_crc_calc(pkt));
  memcpy((char *) pkt->data, (char *) in->data, in->data_len);
  return (get_packet_length_sdh_bw_v1(pkt, in->data_len));
}
