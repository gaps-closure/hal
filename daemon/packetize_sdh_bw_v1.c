/*
 * Convert between Internal HAL PDU and SDH BW packet
 *   April 2021, Perspecta Labs
 */

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

/* Save and combine partial packets */
int packet_combiner(uint8_t *in, int len_in, int len_pkt, pdu *out) {
  static uint8_t  saved_buf[PKT_G1_ADU_SIZE_MAX];
  static int      saved_buf_len;
  static int      saved_pkt_len;
  int             rv = 0;
  
  if (len_pkt < 0) {        // Clear old partial packet
    saved_buf_len = 0;
  }
  else if (len_pkt > 0) {   // Create new partial packet
    memcpy (saved_buf, in, len_in);
    saved_buf_len  = len_in;
    saved_pkt_len  = len_pkt;
    fprintf(stderr, "%s: Wrote incomplete (X2) packet [saved_pkt_len=%d saved_buf_len=%d]\n", __func__, saved_buf_len, saved_pkt_len);
  }
  else {
    if (saved_buf_len > 0) {    // Add to existing partial packet
      memcpy (saved_buf + saved_buf_len, in, len_in);
      saved_buf_len += len_in;
      if (saved_buf_len >= saved_pkt_len) {     // packet is completes
        fprintf(stderr, "%s: X3\n", __func__);
        packet_2_pdu(out, (sdh_bw_v1 *) saved_buf);
        rv = saved_pkt_len;
        saved_buf_len = 0;
        return (rv);
      }
    }
    fprintf(stderr, "%s: Packet still incomplete (X4) or no header (X5): saved_len=%d\n", __func__, saved_buf_len);
  }
  return (-1);
}

/* Put data from buf (using sdh_bw_v1 model) into internal HAL PDU */
int pdu_from_sdh_bw_v1 (pdu *out, uint8_t *in, int len_in) {
  sdh_bw_v1    *pkt = (sdh_bw_v1 *) in;
  uint16_t     crc_pkt, crc_regen;
  int          len_pkt;
  
  crc_pkt = ntohs(pkt->crc16);
  crc_regen = sdh_bw_v1_crc_calc(pkt);
  len_pkt = get_packet_length_sdh_bw_v1(pkt);
  fprintf(stderr, "%s: len [in=%d pkt=%d] crc [pkt=%02x out=%02x]\n", __func__, len_in, len_pkt, crc_pkt, crc_regen);
  
  if (crc_pkt == crc_regen) {                 // Valid packet header
    if (len_pkt <= len_in) {    // X1) Good packet that is complete
      packet_2_pdu(out, pkt);
      packet_combiner(in, len_in, -1, NULL);    // Clear saved packet
      return (len_pkt);
    }
    else {                      // X2) Good partial (incomplete) packet
      fprintf(stderr, "%s: X2\n", __func__);
      packet_combiner(in, len_in, len_pkt, NULL);  // New saved packet
      return (-1);
    }
  }
  else {                       // X3-X5) Not start of a packet
    return (packet_combiner(in, len_in, 0, out));  // Add to saved packet
  }
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
  return (get_packet_length_sdh_bw_v1(pkt));
}
