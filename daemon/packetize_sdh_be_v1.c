/*
 * Convert between HAL PDU and SDH BE packet
 *   March 2020, Perspecta Labs
 *
 * v1 is April 2020 packet format with timestamps
 */

#include "hal.h"
#include "packetize_sdh_be_v1.h"
//#include "../appgen/float.h"    /* no longer use uint64_t */

void linux_time_print(uint32_t *t) {
  uint32_t sec  = ntohl(*t);
  uint32_t usec = ntohl(*(t+1));
  fprintf(stderr, "tl=%u.%u ", sec, usec);
}

void linux_time_set(uint32_t *t) {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    *t     = htonl((uint32_t) tv.tv_sec);
    *(t+1) = htonl((uint32_t) tv.tv_usec);
}

/* Print M1 Packet */
void sdh_be_v1_print(pkt_sdh_be_v1 *p) {
  tlv_sdh_be_v1 *tlv;
  
  fprintf(stderr, "%s: ", __func__);
  fprintf(stderr, "mux=%u ",  ntohl(p->session_tag));
  fprintf(stderr, "sec=%u ",  ntohl(p->message_tag));
  for (int i=0; i < ntohl(p->message_tlv_count); i++) {
    tlv = &(p->tlv[i]);
    fprintf(stderr, "[ty=%u ", ntohl(tlv->data_tag));
    fprintf(stderr, "tg=%x %x", tlv->gaps_time, tlv->gaps_time_us);
    linux_time_print(&(tlv->linux_time));
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
    memcpy (out->data, tlv->data, out->data_len);
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
    tlv->gaps_time = htonl(0x01234567);     /* XXX: Just set for testing */
    tlv->gaps_time_us = htonl(0x89abcdef);  /* XXX: Just set for testing */
    linux_time_set(&(tlv->linux_time));
    tlv->data_len = htonl(in->data_len);
    memcpy((char *) tlv->data, (char *) in->data, in->data_len);
  }
  /* XXX: Fix packet length to depend on message_tlv_count */
  return (sizeof(pkt->session_tag) + sizeof(pkt->message_tag) + sizeof(pkt->message_tlv_count) + sizeof(*tlv) -  SDH_BE_V1_ADU_SIZE_MAX + in->data_len);
}
