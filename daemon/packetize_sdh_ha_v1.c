/*
 * Convert between Internal HAL PDU and closure packet
 *   January 2021, Perspecta Labs
 */

#include "hal.h"
#include "packetize_sdh_ha_v1.h"
#include "map.h"            /* get data_print */

/* Print M1 Packet */
void sda_ha_v1_print(sdh_ha_v1 *p) {
  fprintf(stderr, "%s: ", __func__);
  fprintf(stderr, "mux=%u ",  ntohl(p->tag.mux));
  fprintf(stderr, "sec=%u ",  ntohl(p->tag.sec));
  fprintf(stderr, "typ=%u ",  ntohl(p->tag.typ));
  data_print("Data", p->data, ntohl(p->data_len));
  fprintf(stderr, "\n");
}

/* get size of packet (= header length + data length) */
int get_packet_length_sdh_ha_v1(sdh_ha_v1  *pkt, size_t data_len) {
  return (sizeof(pkt->tag) + sizeof(pkt->data_len) + data_len);
}

/* Put closure packet (in) into internal HAL PDU structure (out) */
int pdu_from_sdh_ha_v1 (pdu *out, uint8_t *in, int len_in) {
  sdh_ha_v1  *pkt = (sdh_ha_v1 *) in;
  
  tag_decode(&(out->psel.tag), &(pkt->tag));
  len_decode(&(out->data_len), pkt->data_len);
  if (get_packet_length_sdh_ha_v1(pkt, out->data_len) > len_in)  return (-1);   /* incomplete packet */
  out->data = pkt->data;   /* TODO_PDU_PTR */
  return (get_packet_length_sdh_ha_v1(pkt, out->data_len));
}

/* Put internal PDU (in) into closure packet (out) */
int pdu_into_sdh_ha_v1 (uint8_t *out, pdu *in, gaps_tag *otag) {
  sdh_ha_v1  *pkt = (sdh_ha_v1 *) out;

  tag_encode(&(pkt->tag), otag);
  len_encode(&(pkt->data_len), in->data_len);
  memcpy(pkt->data, in->data, in->data_len);
  return (get_packet_length_sdh_ha_v1(pkt, in->data_len));
}
