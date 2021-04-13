/*
 * Convert between Internal HAL PDU and closure packet
 *   April 2021, Perspecta Labs
 */

#include "hal.h"
#include "packetize_sdh_ha_v1.h"
#include "map.h"            /* get data_print */
#include "packetize_join.h"

/* Print M1 Packet */
void sdh_ha_v1_print(sdh_ha_v1 *p) {
  fprintf(stderr, "%s: ", __func__);
  fprintf(stderr, "mux=%u ",  p->tag.mux);
  fprintf(stderr, "sec=%u ",  p->tag.sec);
  fprintf(stderr, "typ=%u ",  p->tag.typ);
  data_print("Data", p->data, p->data_len);
  fprintf(stderr, "\n");
}

/* get size of packet (= header length + data length) */
int get_packet_length_sdh_ha_v1(sdh_ha_v1  *pkt) {
//  return (sizeof(pkt->frame_delim) + sizeof(pkt->crc16) + sizeof(pkt->tag) + sizeof(pkt->data_len) + pkt->data_len);
  return (sizeof(*pkt) - ADU_SIZE_MAX_C + pkt->data_len);
}

/* Copy from packet (input or saved) into internal PDU */
void packet_2_pdu_sdh_ha_v1(pdu *out, sdh_ha_v1 *in) {
  out->data_len     = in->data_len;
  tag_cp(&(out->psel.tag), &(in->tag));
  out->data         = in->data;   /* TODO_PDU_PTR */
}

/* Put closure packet (in) into internal HAL PDU structure (out) */
int pdu_from_sdh_ha_v1 (pdu *out, uint8_t *in, int len_in) {
  sdh_ha_v1  *pkt = (sdh_ha_v1 *) in;
  int         len_pkt;

  len_pkt         = get_packet_length_sdh_ha_v1(pkt);
  log_trace("len [in=%d pkt=%d]\n", len_in, len_pkt);
  if (len_in >= len_pkt) packet_2_pdu_sdh_ha_v1(out, pkt);
  return (len_pkt);
}

/* Put internal PDU (in) into closure packet (out) */
int pdu_into_sdh_ha_v1 (uint8_t *out, pdu *in, gaps_tag *otag) {
  sdh_ha_v1  *pkt = (sdh_ha_v1 *) out;

  pkt->data_len = in->data_len;
  memcpy(pkt->data, in->data, in->data_len);
  tag_cp(&(pkt->tag), otag);
  return (get_packet_length_sdh_ha_v1(pkt));

}
