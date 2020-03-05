/* Convert between Internal HAL PDU and SDH BW packets */

#include "hal.h"
#include "crc.h"
#include "packetize_sdh-bw-v1.h"

/* Print G1 Packet */
void g1_print(pkt_g1 *p) {
  fprintf(stderr, "%s: ", __func__);
  fprintf(stderr, "mux=%u ", ntohl(p->message_tag_ID));
  fprintf(stderr, "crc=%02x ", ntohs(p->crc16));
  data_print("Data", p->data, ntohs(p->data_len));
  fprintf(stderr, "\n");
}

uint16_t g1_crc_calc(pkt_g1 *pkt) {
  return (crc16((uint8_t *) pkt, sizeof(pkt->message_tag_ID) + sizeof (pkt->data_len)));
}

/* Put data from buf (using g1 model) into internal HAL PDU */
void pdu_from_pkt_g1 (pdu *out, uint8_t *in , int len) {
  pkt_g1    *pkt = (pkt_g1 *) in;
  fprintf(stderr, "HAL put packet (len = %d) into internal PDU: ", len); g1_print(pkt);
  out->psel.tag.mux = ntohl(pkt->message_tag_ID);
  out->psel.tag.sec = 0;         /* 0 is a don't care */
  out->psel.tag.typ = 0;         /* 0 is a don't care */
  out->data_len     = ntohs(pkt->data_len);
  fprintf(stderr, "crc: in=%02x recalc=%02x\n", ntohs(pkt->crc16), g1_crc_calc(pkt));
  memcpy (out->data, pkt->data, out->data_len);
}

/* Put data into buf (using g1 model) from internal HAL PDU */
/* Returns length of buffer */
int pdu_into_pkt_g1 (uint8_t *out, pdu *in, gaps_tag *otag) {
  pkt_g1    *pkt = (pkt_g1 *) out;
  uint16_t  len = (uint16_t) in->data_len;

  pkt->message_tag_ID = htonl(otag->mux);
  pkt->data_len = htons(len);
  pkt->crc16 = htons(g1_crc_calc(pkt));
  memcpy((char *) pkt->data, (char *) in->data, in->data_len);
  return (sizeof(pkt->message_tag_ID) + sizeof(pkt->data_len) + sizeof(pkt->crc16) + in->data_len);
}
