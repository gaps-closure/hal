/*
 * Convert between Internal HAL PDU and closure packet
 *   November 2020, Perspecta Labs
 *
 * Nov 2020: Removed redundant ntohl conversions of tag and len
 */

#include "hal.h"
#include "packetize_sdh_ha_v1.h"
#include "map.h"            /* get data_print */

/* Print M1 Packet */
void sda_ha_v1_print(sdh_ha_v1 *p) {
  fprintf(stderr, "%s: ", __func__);
  fprintf(stderr, "mux=%u ",  p->tag.mux);
  fprintf(stderr, "sec=%u ",  p->tag.sec);
  fprintf(stderr, "typ=%u ",  p->tag.typ);
  data_print("Data", p->data, p->data_len);
  fprintf(stderr, "\n");
}

/* Put closure packet (in) into internal HAL PDU structure (out) */
void pdu_from_sdh_ha_v1 (pdu *out, uint8_t *in) {
  sdh_ha_v1  *pkt = (sdh_ha_v1 *) in;
  
  tag_cp(&(out->psel.tag), &(pkt->tag));
  out->data_len = pkt->data_len;
//  fprintf(stderr, "LEN=%ld\n", out->data_len);
  memcpy(out->data, pkt->data, out->data_len);   /* TODO_PDU_PTR */
}

/* Put internal PDU (in) into closure packet (out) */
int pdu_into_sdh_ha_v1 (uint8_t *out, pdu *in, gaps_tag *otag) {
  size_t    pkt_len;
  sdh_ha_v1  *pkt = (sdh_ha_v1 *) out;

  tag_cp(&(pkt->tag), otag);
  pkt->data_len = in->data_len;
  memcpy(pkt->data, in->data, in->data_len);
  pkt_len = in->data_len + sizeof(pkt->tag) + sizeof(pkt->data_len);
  return (pkt_len);
}
