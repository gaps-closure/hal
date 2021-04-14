/*
 * Convert between to a serial Line comms packet (with framing)
 *   April 2021, Perspecta Labs
 */

#include "hal.h"
#include "packetize_sdh_sc_v1.h"
#include "map.h"            /* get data_print */
#include "packetize_join.h"
#include "crc.h"

/* Print M1 Packet */
void sdh_sc_v1_print(sdh_sc_v1 *p) {
  fprintf(stderr, "%s: ", __func__);
  fprintf(stderr, "mux=%u ",  ntohl(p->tag.mux));
  fprintf(stderr, "sec=%u ",  ntohl(p->tag.sec));
  fprintf(stderr, "typ=%u ",  ntohl(p->tag.typ));
  fprintf(stderr, "del=%u ",  ntohs(p->delim));
  fprintf(stderr, "crc=%04x ",  ntohs(p->crc16));
  data_print("Data", p->data, ntohl(p->data_len));
  fprintf(stderr, "\n");
}

/* calculate packet's crc */
uint16_t sdh_sc_v1_crc_calc(sdh_sc_v1 *pkt) {
  uint8_t   *tag = (uint8_t *) &(pkt->tag);
  uint8_t   *dat = pkt->data;
//  log_trace("CRC: %p - %p = %d\n", dat, tag, (int) (dat - tag));
  return (crc16(tag, (int) (dat - tag)));
}

/* get size of packet (= header length + data length) */
int get_packet_length_sdh_sc_v1(sdh_sc_v1  *pkt) {
  return (sizeof(*pkt) - ADU_SIZE_MAX_C + ntohl(pkt->data_len));
}

void packet_2_pdu_sdh_sc_v1(pdu *out, sdh_sc_v1 *in) {
  out->data_len     = ntohl(in->data_len);
  out->psel.tag.mux = ntohl(in->tag.mux);
  out->psel.tag.sec = ntohl(in->tag.sec);
  out->psel.tag.typ = ntohl(in->tag.typ);
  out->data         = in->data;
}

/* Put closure packet (in) into internal HAL PDU structure (out) */
int pdu_from_sdh_sc_v1 (pdu *out, uint8_t *in, int len_in) {
  sdh_sc_v1        *pkt = (sdh_sc_v1 *) in;
  uint8_t          *sbuf = in;
  uint16_t          crc_pkt, crc_regen;
  int               len_pkt, frame_delim, rv;
  bool              start_of_packet;
  static pkt_reaas  prs;            /* Packet reassembly structure */
  
  frame_delim     = ntohs(pkt->delim);
  crc_pkt         = ntohs(pkt->crc16);
  crc_regen       = crc_pkt;
  len_pkt         = get_packet_length_sdh_sc_v1(pkt);
  start_of_packet = (crc_pkt == crc_regen) && (frame_delim == SDH_DELIM);
  log_trace("start=%d, len [in=%d pkt=%d], crc [pkt=%02x out=%02x]. delim=%04x\n", start_of_packet, len_in, len_pkt, crc_pkt, crc_regen, frame_delim);
  rv = packet_parser(in, len_in, len_pkt, &sbuf, start_of_packet, &prs);
  if (rv > 0) packet_2_pdu_sdh_sc_v1(out, (sdh_sc_v1 *) sbuf);
  return (rv);
}

/* Put internal PDU (in) into closure packet (out) */
int pdu_into_sdh_sc_v1 (uint8_t *out, pdu *in, gaps_tag *otag) {
  sdh_sc_v1  *pkt = (sdh_sc_v1 *) out;

  pkt->data_len = htonl(in->data_len);
  memcpy(pkt->data, in->data, in->data_len);
  pkt->tag.mux  = htonl(in->psel.tag.mux);
  pkt->tag.sec  = htonl(in->psel.tag.sec);
  pkt->tag.typ  = htonl(in->psel.tag.typ);
  pkt->delim    = htons(SDH_DELIM);
  pkt->crc16    = htons(sdh_sc_v1_crc_calc(pkt));
//  sdh_sc_v1_print(pkt);
  return (get_packet_length_sdh_sc_v1(pkt));

}
