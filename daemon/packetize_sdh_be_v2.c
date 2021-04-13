/*
 * Convert between HAL PDU and BE Security Device Hardware (SDH) packet
 *   April 2021, Perspecta Labs
 *
 * v2 is the November 2020 EOP Immediate Mode packet format.
 * Similar to v1, but Descriptor Type replaces TLV count (+ new dst & SipHash fields)
 */

#include "hal.h"
#include "time.h"
#include "packetize_sdh_be_v2.h"
#include "map.h"            /* get data_print */

/* Print external packet  */
void sdh_be_v2_print(pkt_sdh_be_v2 *p) {
  fprintf(stderr, "%s (l=%ld): ", __func__, sizeof(*p));
  fprintf(stderr, "<m=%u ",      ntohl(p->session_tag));
  fprintf(stderr, "s=%u ",       ntohl(p->message_tag));
  fprintf(stderr, "t=%u> ",      ntohl(p->data_tag));
  fprintf(stderr, "des=%d ",     ntohl(p->descriptor_type));
  fprintf(stderr, "gaps=%x.%x ", p->gaps_time_up,  p->gaps_time_lo);
  fprintf(stderr, "unix=%x.%x ", p->linux_time_up, p->linux_time_lo);
  fprintf(stderr, "dst==%u ",    p->destination_tag);
  data_print(     "Data",        p->imm_data, ntohl(p->imm_data_len));
  fprintf(stderr, "hsh=%x.%x ",  p->desc_sip_hash_lo, p->desc_sip_hash_up);
  fprintf(stderr, "\n");
}

/* Check if data length fits in an external packet */
size_t check_len_sdh_be_v2 (size_t len) {
    if (len > SDH_BE_V2_ADU_SIZE_MAX) {
        log_error("Sending immediate data of len (%ld) > MAX packet len (%d)", len, SDH_BE_V2_ADU_SIZE_MAX);
        log_error("...HAL deletes Bad packet");
        return (0);
    }
    return (len);
}

/* Put data from external packet (*in) into internal HAL PDU */
int pdu_from_sdh_be_v2 (pdu *out, uint8_t *in, int len_in) {
    pkt_sdh_be_v2  *pkt = (pkt_sdh_be_v2 *) in;
    int             len_pkt = sizeof(*pkt);

    out->data_len     = check_len_sdh_be_v2(ntohl(pkt->imm_data_len));
    log_trace("len [in=%d pkt=%d data=%d]", len_in, len_pkt, out->data_len);
    if (len_pkt > len_in)  {
      log_warn("Partial ILIPv2 packet. If this can occur, then add packetize_join.c");
      return (-1);
    }
//    fprintf(stderr, "%s: ", __func__); sdh_be_v2_print(pkt);
    out->psel.tag.mux = ntohl(pkt->session_tag);
    out->psel.tag.sec = ntohl(pkt->message_tag);
    out->psel.tag.typ = ntohl(pkt->data_tag);
    out->data = pkt->imm_data;
    return (len_pkt);
}

/* Put data into external packet (*out) from internal HAL PDU */
/* Returns length of external packet */
int pdu_into_sdh_be_v2 (uint8_t *out, pdu *in, gaps_tag *otag) {
    pkt_sdh_be_v2  *pkt = (pkt_sdh_be_v2 *) out;
    size_t          len = check_len_sdh_be_v2(in->data_len);
    
    if (len == 0) return (0);   // PDU data too long for v2 immediate mode
    
    pkt->session_tag        = htonl(otag->mux);
    pkt->message_tag        = htonl(otag->sec);
    pkt->data_tag           = htonl(otag->typ);

    pkt->descriptor_type    = htonl(0);         // Immediate mode
//    log_error("HACK to test shd_be v2 with v1 ilip"); pkt->descriptor_type = htonl(1);
    
    pkt->imm_data_len = htonl(len);
    memcpy((char *) pkt->imm_data, (char *) in->data, len);
    
    // Set by Driver or ILIP device - Reset just to recognize any changes
    pkt->gaps_time_lo       = 0;
    pkt->gaps_time_up       = 0;
    //    pkt->gaps_time = htonl(0x01234567);     /* XXX: Just set for testing */
    //    pkt->gaps_time_us = htonl(0x89abcdef);  /* XXX: Just set for testing */
    pkt->linux_time_lo      = 0;
    pkt->linux_time_up      = 0;
    pkt->desc_sip_hash_lo   = 0;
    pkt->desc_sip_hash_up   = 0;
//    sdh_be_v2_print(pkt);

    return (sizeof(*pkt));    /* packet size always 256 Bytes in v2 (unless too big) */
}
