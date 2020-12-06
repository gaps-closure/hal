/*
 * Convert between HAL PDU and external packet of the BookEnd (BE)
 * Security Device Hardware (SDH) packet
 *   April 2020, Perspecta Labs
 *
 * v1 is April 2020 packet format with timestamps
 */

#include "hal.h"
#include "time.h"
#include "packetize_sdh_be_v1.h"
#include "map.h"            /* get data_print */

/* Print M1 Packet */
void sdh_be_v1_print(pkt_sdh_be_v1 *p) {
  tlv_sdh_be_v1 *tlv;
  uint64_t *ut;
  
  fprintf(stderr, "%s: ", __func__);
  fprintf(stderr, "mux=%u ",  ntohl(p->session_tag));
  fprintf(stderr, "sec=%u ",  ntohl(p->message_tag));
  for (int i=0; i < ntohl(p->message_tlv_count); i++) {
    tlv = &(p->tlv[i]);
    fprintf(stderr, "[ty=%u ", ntohl(tlv->data_tag));
    fprintf(stderr, "tg=%x.%x ", tlv->gaps_time, tlv->gaps_time_us);
    ut = (uint64_t *) &(tlv->linux_time);
    fprintf(stderr, "ut=%lu ns ", *ut);
    data_print("Data", tlv->data, ntohl(tlv->data_len));
    fprintf(stderr, "]");
  }
  fprintf(stderr, "\n");
}

/* Put data from buf (using M1 model) into internal HAL PDU */
void pdu_from_sdh_be_v1 (pdu *out, uint8_t *in, int len) {
  pkt_sdh_be_v1  *pkt = (pkt_sdh_be_v1 *) in;
  tlv_sdh_be_v1  *tlv;
    
  // fprintf(stderr, "HAL put packet (len = %d) into internal PDU: ", len); m1_print(pkt);
  out->psel.tag.mux = ntohl(pkt->session_tag);
  out->psel.tag.sec = ntohl(pkt->message_tag);
//  fprintf(stderr, "YYYY: count=%d\n", ntohl(pkt->message_tlv_count));
  for (int i=0; i < ntohl(pkt->message_tlv_count); i++) {
    tlv = &(pkt->tlv[0]);
    out->psel.tag.typ = ntohl(tlv->data_tag);
    out->data_len = ntohl(tlv->data_len);
//    memcpy (out->data, tlv->data, out->data_len);    /* TODO_PDU_PTR */
    out->data = tlv->data;            /* TODO_PDU_PTR */
  }
}

/* Put data into buf (using M1 model) from internal HAL PDU */
/* Returns length of buffer */
int pdu_into_sdh_be_v1 (uint8_t *out, pdu *in, gaps_tag *otag) {
  pkt_sdh_be_v1  *pkt = (pkt_sdh_be_v1 *) out;
  tlv_sdh_be_v1  *tlv;
  
  pkt->session_tag = htonl(otag->mux);
  pkt->message_tag = htonl(otag->sec);
  pkt->message_tlv_count = htonl(1);
  for (int i=0; i < 1; i++) {
    tlv = &(pkt->tlv[i]);
    tlv->data_tag = htonl(otag->typ);
//    tlv->gaps_time = htonl(0x01234567);     /* XXX: Just set for testing */
//    tlv->gaps_time_us = htonl(0x89abcdef);  /* XXX: Just set for testing */
    tlv->gaps_time = 0;
    tlv->gaps_time_us = 0;
    linux_time_set((uint64_t *) &(tlv->linux_time));
    tlv->data_len = htonl(in->data_len);
    memcpy((char *) tlv->data, (char *) in->data, in->data_len);
  }
//  sdh_be_v1_print(pkt);
  /* XXX: Fix packet length to depend on message_tlv_count */
  return (sizeof(pkt->session_tag) + sizeof(pkt->message_tag) + sizeof(pkt->message_tlv_count) + sizeof(*tlv) -  SDH_BE_V1_ADU_SIZE_MAX + in->data_len);
}
