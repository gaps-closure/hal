/*
 * Convert between HAL PDU and SDH BE packet
 *   January 2021, Perspecta Labs
 *
 * v0 is oringal (2019) packet format with no timestamps
 */

#include "hal.h"
#include "packetize_sdh_be_v0.h"
//#include "../appgen/float.h"    /* no longer use uint64_t */


/* Print Packet */
void sdh_be_v0_print(pkt_sdh_be_v0 *p) {
  tlv_sdh_be_v0 *tlv;
  
  fprintf(stderr, "%s: ", __func__);
  fprintf(stderr, "mux=%u ",  ntohl(p->session_tag));
  fprintf(stderr, "sec=%u ",  ntohl(p->message_tag));
  for (int i=0; i < ntohl(p->message_tlv_count); i++) {
    tlv = &(p->tlv[i]);
    fprintf(stderr, "[ty=%u ", ntohl(tlv->data_tag));
    data_print("Data", tlv->data, ntohl(tlv->data_len));
    fprintf(stderr, "]");
  }
  fprintf(stderr, "\n");
}

/* Put data from buf (using M1 model) into internal HAL PDU */
int pdu_from_sdh_be_v0 (pdu *out, uint8_t *in, int len) {
  pkt_sdh_be_v0  *pkt = (pkt_sdh_be_v0 *) in;
  tlv_sdh_be_v0  *tlv;
    
  // fprintf(stderr, "HAL put packet (len = %d) into internal PDU: ", len); m1_print(pkt);
  out->psel.tag.mux = ntohl(pkt->session_tag);
  out->psel.tag.sec = ntohl(pkt->message_tag);
//  fprintf(stderr, "YYYY: count=%d\n", ntohl(pkt->message_tlv_count));
  for (int i=0; i < ntohl(pkt->message_tlv_count); i++) {
    tlv = &(pkt->tlv[0]);
    out->psel.tag.typ = ntohl(tlv->data_tag);
    out->data_len = ntohl(tlv->data_len);
//    memcpy (out->data, tlv->data, out->data_len);  /* TODO_PDU_PTR */
    out->data = tlv->data;  /* TODO_PDU_PTR */
  }
  return(out->data_len);
}

/* Put data into buf (using M1 model) from internal HAL PDU */
/* Returns length of buffer */
int pdu_into_sdh_be_v0 (uint8_t *out, pdu *in, gaps_tag *otag) {
  pkt_sdh_be_v0  *pkt = (pkt_sdh_be_v0 *) out;
  tlv_sdh_be_v0  *tlv;

  pkt->session_tag = htonl(otag->mux);
  pkt->message_tag = htonl(otag->sec);
  pkt->message_tlv_count = htonl(1);
  for (int i=0; i < 1; i++) {
    tlv = &(pkt->tlv[i]);
    tlv->data_tag = htonl(otag->typ);
    tlv->data_len = htonl(in->data_len);
    memcpy((char *) tlv->data, (char *) in->data, in->data_len);
  }
  /* XXX: Fix packet length to depend on message_tlv_count */
  return (sizeof(pkt->session_tag) + sizeof(pkt->message_tag) + sizeof(pkt->message_tlv_count) + sizeof(tlv->data_tag) + sizeof(tlv->data_len) + in->data_len);
}
