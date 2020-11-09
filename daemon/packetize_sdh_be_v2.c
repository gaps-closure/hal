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

/* Print external packet  */
void sdh_be_v2_print(pkt_sdh_be_v2 *p) {
  uint64_t      *ut = (uint64_t *) &(p->linux_time);
  
  fprintf(stderr, "%s (l=%ld): ", __func__, sizeof(*p));
  fprintf(stderr, "mux=%u ",     ntohl(p->session_tag));
  fprintf(stderr, "sec=%u ",     ntohl(p->message_tag));
  fprintf(stderr, "des=%d ",     ntohl(p->descriptor_type));
  fprintf(stderr, "typ=%u ",     ntohl(p->data_tag));
  fprintf(stderr, "gaps=%x.%x ",  p->gaps_time, p->gaps_time_us);
  fprintf(stderr, "unix=%lu ns ", *ut);
  fprintf(stderr, "dst==%u ",    p->destination_tag);
  data_print(     "Data",        p->imm_data, ntohl(p->imm_data_len));
  fprintf(stderr, "\n");
}

/* Check if data length tits in an external packet */
size_t check_len_sdh_be_v2 (size_t len) {
    if (len > SDH_BE_V2_ADU_SIZE_MAX) {
        log_error("Sending immediate data of len (%ld) > MAX packet len (%d)", len, SDH_BE_V2_ADU_SIZE_MAX);
        log_error("...HAL runcating data to %d Bytes", SDH_BE_V2_ADU_SIZE_MAX);
        return (SDH_BE_V2_ADU_SIZE_MAX);
    }
    return (len);
}

/* Put data from external packet (*in) into internal HAL PDU */
void pdu_from_sdh_be_v2 (pdu *out, uint8_t *in) {
    pkt_sdh_be_v2  *pkt = (pkt_sdh_be_v2 *) in;
    size_t          len;

//    fprintf(stderr, "%s: ", __func__); sdh_be_v2_print(pkt);
    out->psel.tag.mux = ntohl(pkt->session_tag);
    out->psel.tag.sec = ntohl(pkt->message_tag);
    out->psel.tag.typ = ntohl(pkt->data_tag);
    len = check_len_sdh_be_v2(ntohl(pkt->imm_data_len));
    out->data_len     = len;
    memcpy (out->data, pkt->imm_data, len);
}

/* Put data into external packet (*out) from internal HAL PDU */
/* Returns length of external packet */
int pdu_into_sdh_be_v2 (uint8_t *out, pdu *in, gaps_tag *otag) {
    pkt_sdh_be_v2  *pkt = (pkt_sdh_be_v2 *) out;
    size_t          len;
    
    pkt->session_tag        = htonl(otag->mux);
    pkt->message_tag        = htonl(otag->sec);
    pkt->descriptor_type    = htonl(0);
    log_error("HACK to test shd_be v2 with v1 ilip"); pkt->descriptor_type = htonl(1);
    pkt->data_tag           = htonl(otag->typ);
//    pkt->gaps_time = htonl(0x01234567);     /* XXX: Just set for testing */
//    pkt->gaps_time_us = htonl(0x89abcdef);  /* XXX: Just set for testing */
    pkt->gaps_time = 0;
    pkt->gaps_time_us = 0;
    linux_time_set((uint64_t *) &(pkt->linux_time));
    len = check_len_sdh_be_v2(in->data_len);
    pkt->imm_data_len = htonl(len);
    memcpy((char *) pkt->imm_data, (char *) in->data, len);
//    sdh_be_v2_print(pkt);
    return (sizeof(*pkt));       /* v2 always sends 256 byte packet */
}
