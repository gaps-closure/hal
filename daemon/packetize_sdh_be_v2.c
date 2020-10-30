/*
 * Convert between HAL PDU and SDH BE packet
 *   November 2020, Perspecta Labs
 *
 * v2 is the November 2020 EOP Immediate Mode packet format.
 * Similar to v1, but Descriptor Type field replaces TLV count + 2 new fields
 */

#include "hal.h"
#include "packetize_sdh_be_v2.h"
#include "../appgen/6month-demo/float754.h"          /* use htonll */

void linux_time_set(uint64_t *t) {
  struct timeval tv;
  uint64_t ns;
  
  gettimeofday(&tv,NULL);
  ns =  (tv.tv_usec * 1000) + (tv.tv_sec  * 1000000000);
  *t = htonll(ns);          // Convert to BIG ENDIAN
  log_trace("unix time = %lu = 0x%lx nsec\n", ns, ns);
//  fprintf(stderr, "unix time=%lu.%lu sec = %lu = 0x%lx (net=0x%lx) nsec\n", tv.tv_sec, tv.tv_usec, ns, ns, *t);
}

/* Print M1 Packet */
void sdh_be_v2_print(pkt_sdh_be_v2 *p) {
  tlv_sdh_be_v2 *tlv;
  uint64_t      *ut = (uint64_t *) &(linux_time);;
  
  fprintf(stderr, "%s (l=%d): ", __func__, sizeof(*pkt_sdh_be_v2));
  fprintf(stderr, "mux=%u ",     ntohl(p->session_tag));
  fprintf(stderr, "sec=%u ",     ntohl(p->message_tag));
  fprintf(stderr, "des=%d ",     ntohl(p->message_descriptor);
  fprintf(stderr, "typ=%u ",     ntohl(tlv->data_tag));
  fprintf(stderr, "tig=%x.%x ",  gaps_time, gaps_time_us);
  fprintf(stderr, "tiu=%lu ns ", *ut);
  fprintf(stderr, "dst==%u ",    destination_tag));
  data_print(     "Data",        tlv->data, ntohl(tlv->data_len));
  fprintf(stderr, "\n");
}

/* Put data from buf (using M1 model) into internal HAL PDU */
void pdu_from_sdh_be_v2 (pdu *out, uint8_t *in, int len) {
  pkt_sdh_be_v2  *pkt = (pkt_sdh_be_v2 *) in;
  tlv_sdh_be_v2  *tlv;
    
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
int pdu_into_sdh_be_v2 (uint8_t *out, pdu *in, gaps_tag *otag) {
    pkt_sdh_be_v2  *pkt = (pkt_sdh_be_v2 *) out;
    tlv_sdh_be_v2  *tlv;
  
    pkt->session_tag        = htonl(otag->mux);
    pkt->message_tag        = htonl(otag->sec);
    pkt->message_descriptor = 0;
    pkt->message_descriptor = 1;   /* temporary hack for testing with v1 ilip */
    pkt->data_tag           = htonl(otag->typ);
//    pkt->gaps_time = htonl(0x01234567);     /* XXX: Just set for testing */
//    pkt->gaps_time_us = htonl(0x89abcdef);  /* XXX: Just set for testing */
    pkt->gaps_time = 0;
    pkt->gaps_time_us = 0;
    linux_time_set((uint64_t *) &(tlv->linux_time));
    pkt->data_len = htonl(in->data_len);
    memcpy((char *) tlv->data, (char *) in->data, in->data_len);

    sdh_be_v2_print(pkt);
    return (sizeof(*pkt);       /* v2 always sends 256 byte packet */
}
