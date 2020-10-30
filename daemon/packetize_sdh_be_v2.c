/*
 * Convert between HAL PDU and SDH BE packet
 *   November 2020, Perspecta Labs
 *
 * v2 is the November 2020 EOP Immediate Mode packet format.
 * Similar to v1, but Descriptor Type field replaces TLV count + 2 new fields
 */

#include "hal.h"
#include "time.h"
#include "packetize_sdh_be_v2.h"

/* Print M1 Packet */
void sdh_be_v2_print(pkt_sdh_be_v2 *p) {
  uint64_t      *ut = (uint64_t *) &(p->linux_time);
  
  fprintf(stderr, "%s (l=%ld): ", __func__, sizeof(*p));
  fprintf(stderr, "mux=%u ",     ntohl(p->session_tag));
  fprintf(stderr, "sec=%u ",     ntohl(p->message_tag));
  fprintf(stderr, "des=%d ",     ntohl(p->message_descriptor));
  fprintf(stderr, "typ=%u ",     ntohl(p->data_tag));
  fprintf(stderr, "gaps=%x.%x ",  p->gaps_time, p->gaps_time_us);
  fprintf(stderr, "unix=%lu ns ", *ut);
  fprintf(stderr, "dst==%u ",    p->destination_tag);
  data_print(     "Data",        p->data, ntohl(p->data_len));
  fprintf(stderr, "\n");
}

/* Put data from buf (using M1 model) into internal HAL PDU */
void pdu_from_sdh_be_v2 (pdu *out, uint8_t *in, int len) {
  pkt_sdh_be_v2  *pkt = (pkt_sdh_be_v2 *) in;
    
//    sdh_be_v2_print(pkt);
//    fprintf(stderr, "HAL put packet (len = %d) into internal PDU: ", len); m1_print(pkt);
    out->psel.tag.mux = ntohl(pkt->session_tag);
    out->psel.tag.sec = ntohl(pkt->message_tag);
    out->psel.tag.typ = ntohl(pkt->data_tag);
    out->data_len = ntohl(pkt->data_len);
    memcpy (out->data, pkt->data, out->data_len);
}

/* Put data into buf (using M1 model) from internal HAL PDU */
/* Returns length of buffer */
int pdu_into_sdh_be_v2 (uint8_t *out, pdu *in, gaps_tag *otag) {
    pkt_sdh_be_v2  *pkt = (pkt_sdh_be_v2 *) out;
  
    pkt->session_tag        = htonl(otag->mux);
    pkt->message_tag        = htonl(otag->sec);
    pkt->message_descriptor = 0;
    pkt->message_descriptor = htonl(1);   /* temporary hack for testing with v1 ilip */
    pkt->data_tag           = htonl(otag->typ);
//    pkt->gaps_time = htonl(0x01234567);     /* XXX: Just set for testing */
//    pkt->gaps_time_us = htonl(0x89abcdef);  /* XXX: Just set for testing */
    pkt->gaps_time = 0;
    pkt->gaps_time_us = 0;
    linux_time_set((uint64_t *) &(pkt->linux_time));
    pkt->data_len = htonl(in->data_len);
    memcpy((char *) pkt->data, (char *) in->data, in->data_len);
//    sdh_be_v2_print(pkt);
    return (sizeof(*pkt));       /* v2 always sends 256 byte packet */
}
