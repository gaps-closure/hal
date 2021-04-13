/*
 * Convert between to a serial Line comms packet (with framing)
 *   April 2021, Perspecta Labs
 */

#include "hal.h"
#include "packetize_sdh_sc_v1.h"
#include "map.h"            /* get data_print */
#include "packetize_join.h"

/* Print M1 Packet */
void sdh_sc_v1_print(sdh_sc_v1 *p) {
  fprintf(stderr, "%s: ", __func__);
  fprintf(stderr, "mux=%u ",  p->tag.mux);
  fprintf(stderr, "sec=%u ",  p->tag.sec);
  fprintf(stderr, "typ=%u ",  p->tag.typ);
  fprintf(stderr, "del=%u ",  p->delim);
  fprintf(stderr, "crc=%u ",  p->crc16);
  data_print("Data", p->data, p->data_len);
  fprintf(stderr, "\n");
}

/* calculate packet's crc */
uint16_t sdh_sc_v1_crc_calc(sdh_sc_v1 *pkt) {
  return (crc16((uint8_t *) pkt, sizeof(*pkt) - ADU_SIZE_MAX_C);
}

/* get size of packet (= header length + data length) */
int get_packet_length_sdh_sc_v1(sdh_sc_v1  *pkt) {
  return (sizeof(*pkt) - ADU_SIZE_MAX_C + pkt->data_len);
}

void packet_2_pdu_sdh_sc_v1(pdu *out, sdh_bw_v1 *in) {
  out->data_len     = ntohs(in->data_len);
  out->psel.tag.mux = ntohl(in->session_tag);
  out->psel.tag.sec = ntohl(in->message_tag);
  out->psel.tag.typ = ntohl(in->data_tag);
  out->data         = in->data;
}

/* Put closure packet (in) into internal HAL PDU structure (out) */
int pdu_from_sdh_sc_v1 (pdu *out, uint8_t *in, int len_in) {
  sdh_sc_v1  *pkt = (sdh_sc_v1 *) in;
  uint8_t    *sbuf = in;
  uint16_t    crc_pkt, crc_regen;
  int         len_pkt, frame_delim, rv;
  bool        start_of_packet;

  frame_delim     = pkt->version;
  crc_pkt         = pkt->crc16;
  crc_regen       = crc_pkt;
  len_pkt         = get_packet_length_sdh_sc_v1(pkt);
  start_of_packet = (crc_pkt == crc_regen) && ((frame_delim == VERSION_LOCAL) || (frame_delim == VERSION_REMOTE));
  if (start_of_packet)  log_trace("start=%d, len [in=%d pkt=%d], crc [pkt=%02x out=%02x]. delim=%d\n", start_of_packet, len_in, len_pkt, crc_pkt, crc_regen, frame_delim);
  else                  log_trace("start=%d, len [in=%d pkt=XX]\n", start_of_packet, len_in);

  rv = packet_parser(in, len_in, len_pkt, out, &sbuf, start_of_packet);
  if (rv > 0) packet_2_pdu_sdh_sc_v1(out, (sdh_sc_v1 *) sbuf);
  return (rv);
}

/* Put internal PDU (in) into closure packet (out) */
int pdu_into_sdh_sc_v1 (uint8_t *out, pdu *in, gaps_tag *otag) {
  sdh_sc_v1  *pkt = (sdh_sc_v1 *) out;

  pkt->data_len = in->data_len;
  memcpy(pkt->data, in->data, in->data_len);
  tag_cp(&(pkt->tag), otag);
  return (get_packet_length_sdh_sc_v1(pkt));

}
