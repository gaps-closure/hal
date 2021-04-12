/*
 * Convert between Internal HAL PDU and closure packet
 *   April 2021, Perspecta Labs
 */

#include "hal.h"
#include "packetize_sdh_ha_v1.h"
#include "map.h"            /* get data_print */
#include "packetize_join.h"

/* Print M1 Packet */
void sda_ha_v1_print(sdh_ha_v1 *p) {
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

/* Put closure packet (in) into internal HAL PDU structure (out) */
int pdu_from_sdh_ha_v1 (pdu *out, uint8_t *in, int len_in) {
  sdh_ha_v1  *pkt = (sdh_ha_v1 *) in;
  uint16_t    crc_pkt, crc_regen;
  int         len_pkt, frame_delim;
  bool        start_of_packet;

  frame_delim     = pkt->version;
  crc_pkt         = pkt->crc16;
  crc_regen       = crc_pkt;
  len_pkt         = get_packet_length_sdh_ha_v1(pkt);
  start_of_packet = (crc_pkt == crc_regen) && (frame_delim == 0);
  log_trace("len [in=%d pkt=%d] crc [pkt=%02x out=%02x] v=0x%x start=%d\n", len_in, len_pkt, crc_pkt, crc_regen, frame_delim, start_of_packet);
  
  if (len_pkt > len_in)  return (-1);   /* incomplete packet */
  out->data_len = pkt->data_len;
  tag_cp(&(out->psel.tag), &(pkt->tag));
  out->data = pkt->data;   /* TODO_PDU_PTR */

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
