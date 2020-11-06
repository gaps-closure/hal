/*
 * Convert between Internal HAL PDU and closure packet
 *   November 2020, Perspecta Labs
 *
 * v2 is the November 2020 EOP packet format.
 * Similar to v1, but adds DMA address + length (for packetize_sdh_be_v3)
 */

#include "hal.h"
#include "packetize_sdh_ha_v1.h"

/* Print M1 Packet */
void sda_ha_v1_print(sdh_ha_v1 *p) {
  fprintf(stderr, "%s: ", __func__);
  fprintf(stderr, "mux=%u ",  ntohl(p->tag.mux));
  fprintf(stderr, "sec=%u ",  ntohl(p->tag.sec));
  fprintf(stderr, "typ=%u ",  ntohl(p->tag.typ));
  data_print("Data", p->data, ntohl(p->data_len));
  fprintf(stderr, "\n");
}

/* Put closure packet (in buf) into internal HAL PDU structure (*p) */
void pdu_from_sdh_ha_v1 (pdu *out, uint8_t *in) {
  sdh_ha_v1  *pkt = (sdh_ha_v1 *) in;
  
  tag_decode(&(out->psel.tag), &(pkt->tag));
  len_decode(&(out->data_len), pkt->data_len);
//  fprintf(stderr, "LEN=%ld\n", out->data_len);
  memcpy(out->data, pkt->data, out->data_len);
}

/* Put internal PDU into closure packet (in buf) */
int pdu_into_sdh_ha_v1 (uint8_t *out, pdu *in, gaps_tag *otag) {
  size_t    pkt_len;
  sdh_ha_v1  *pkt = (sdh_ha_v1 *) out;

  tag_encode(&(pkt->tag), otag);
  len_encode(&(pkt->data_len), in->data_len);
  memcpy(pkt->data, in->data, in->data_len);
  pkt_len = in->data_len + sizeof(pkt->tag) + sizeof(pkt->data_len);
  return (pkt_len);
}
