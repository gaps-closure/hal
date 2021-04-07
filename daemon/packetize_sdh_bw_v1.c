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
int get_packet_length_sdh_bw_v1(sdh_bw_v1 *pkt) {
  return (sizeof(pkt->message_tag_ID) + sizeof(pkt->data_len) + sizeof(pkt->crc16) + ntohs(pkt->data_len));
}

/* Save partial packet (and comine each new partial packet) */

int packet_combiner(uint8_t *in, int len_in, int len_pkt, pdu *out) {
  static uint8_t  saved_buf[PKT_G1_ADU_SIZE_MAX];
  static int      saved_buf_len;
  static int      saved_pkt_len;
  sdh_bw_v1      *saved_pkt_ptr;
  
  if (len_pkt < 0) {        // Clear old partial packet
    saved_buf_len = 0;
  }
  else if (len_pkt > 0) {   // Create new partial packet
    memcpy (saved_buf, in, len_in);
    saved_buf_len   = len_in;
    saved_pkt_len  = len_pkt;
  }
  else {
    if (saved_buf_len > 0) {    // Add to existing partial packet
      memcpy (saved_buf + saved_buf_len, in, len_in);
      saved_buf_len += len_in;
      if (saved_buf_len >= saved_pkt_len) {     // packet is completes
        fprintf(stderr, "%s: X3\n", __func__);
        saved_pkt_ptr  = (sdh_bw_v1 *) saved_buf;
        out->data_len  = ntohs(saved_pkt_ptr->data_len);
        out->psel.ctag = ntohl(saved_pkt_ptr->message_tag_ID);
        out->data      =       saved_pkt_ptr->data;
        return (saved_pkt_len);
      }
    }
  }
  fprintf(stderr, "%s: Packet still incomplete (X4) or no header (X5): saved_len=%d\n", __func__, saved_buf_len);
  return (-1);
}

/* Put data from buf (using sdh_bw_v1 model) into internal HAL PDU */
int pdu_from_sdh_bw_v1 (pdu *out, uint8_t *in, int len_in) {
  sdh_bw_v1    *pkt = (sdh_bw_v1 *) in;
  uint16_t     crc_pkt, crc_out;
  int          len_pkt;
  
  crc_pkt = ntohs(pkt->crc16);
  crc_out = sdh_bw_v1_crc_calc(pkt);
  len_pkt = get_packet_length_sdh_bw_v1(pkt);
  fprintf(stderr, "%s: len [in=%d pkt=%d] crc [pkt=%02x out=%02x]\n", __func__, len_in, len_pkt, crc_pkt, crc_out);
  
  if (crc_pkt == crc_out) {                 // Valid packet header
    if (len_pkt <= len_in) {
      // X1) Good packet that is complete
      out->data_len  = ntohs(pkt->data_len);
      out->psel.ctag = ntohl(pkt->message_tag_ID);
      out->data      = pkt->data;
      packet_combiner(in, len_in, -1, NULL);    // Clear saved packet
      return (len_pkt);
    }
    else {   // X2) Good packet, but incomplete (partial packet)
      fprintf(stderr, "%s: X2\n", __func__);
      packet_combiner(in, len_in, len_pkt, NULL);  // New saved packet
      return (-1);
    }
  }
  else {                       // Not the start of a packet
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
