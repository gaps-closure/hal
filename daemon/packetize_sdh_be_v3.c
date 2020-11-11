/*
 * Convert between HAL PDU and Security Device Hardware (SDH) BE packet
 *   November 2020, Perspecta Labs
 *
 * v3 is the November 2020 EOP Payload Mode packet format.
 */

#include "hal.h"
#include "time.h"
#include "packetize_sdh_be_v3.h"

/* Print external packet  */
void sdh_be_v3_print(pkt_sdh_be_v3 *p) {
    fprintf(stderr, "%s (l=%ld): ", __func__, sizeof(*p));
    fprintf(stderr, "mux=%u ",        ntohl(p->session_tag));
    fprintf(stderr, "sec=%u ",        ntohl(p->message_tag));
    fprintf(stderr, "typ=%u ",        ntohl(p->data_tag));
    fprintf(stderr, "dst=%u ",        p->destination_tag);
    fprintf(stderr, "dsc=%d ",        ntohl(p->descriptor_type));
    fprintf(stderr, "gaps=%x.%x ",    p->gaps_time_up,  p->gaps_time_lo);
    fprintf(stderr, "unix=%x.%x ",    p->linux_time_up, p->linux_time_lo);
    fprintf(stderr, "len=%d ",        ntohl(p->dma_data_len));
    fprintf(stderr, "Addr=%08x-%08x", p->dma_data_addr_up,
                                      p->dma_data_addr_lo);
    fprintf(stderr, "\n");
}

/* Check if data length tits in an external packet */
void put_dma_adresss (pkt_sdh_be_v3 *pkt, uint8_t *data_in) {
    uint64_t x = (uint64_t) data_in;
    pkt->dma_data_addr_lo = (uint32_t) (x & 0xFFFFFFFF);
    pkt->dma_data_addr_up = (uint32_t) (x >> 32);;
//    fprintf(stderr, "addr = %p %lx (%x %x)\n",  data_in, x, pkt->dma_data_addr_up, pkt->dma_data_addr_lo);
}

/* Put data from external packet (*in) into internal HAL PDU */
void pdu_from_sdh_be_v3 (pdu *out, uint8_t *in) {
    pkt_sdh_be_v3  *pkt = (pkt_sdh_be_v3 *) in;

//    fprintf(stderr, "%s: ", __func__); sdh_be_v3_print(pkt);
    out->psel.tag.mux = ntohl(pkt->session_tag);
    out->psel.tag.sec = ntohl(pkt->message_tag);
    out->psel.tag.typ = ntohl(pkt->data_tag);
//    len = check_len_sdh_be_v3(ntohl(pkt->imm_data_len));
    out->data_len     = pkt->dma_data_len;
    
    sdh_be_v3_print(pkt); exit(1);

    memcpy (out->data, pkt->imm_data, pkt->dma_data_len);
}

/* Put data into external packet (*out) from internal HAL PDU */
/* Returns length of external packet */
int pdu_into_sdh_be_v3 (uint8_t *out, pdu *in, gaps_tag *otag) {
    pkt_sdh_be_v3  *pkt = (pkt_sdh_be_v3 *) out;
    
    // a) Copy tag info and descriptor into packet
    pkt->session_tag        = htonl(otag->mux);
    pkt->message_tag        = htonl(otag->sec);
    pkt->data_tag           = htonl(otag->typ);
    pkt->destination_tag    = 0;
    pkt->descriptor_type    = htonl(1); // Payload Mode - check value and if need hton
 
    // b) Timestamps set by driver (linux) or ILIP (gaps)
//    pkt->gaps_time_lo = htonl(0x01234567);  /* XXX: Just set for testing */
//    pkt->gaps_time_up = htonl(0x89abcdef);  /* XXX: Just set for testing */
    pkt->gaps_time_lo       = 0;
    pkt->gaps_time_up       = 0;
    pkt->linux_time_lo      = 0;
    pkt->linux_time_up      = 0;
    
    /* c1) Immediate data: can be copied into packet, but disable for now */
    pkt->imm_data_len       = htonl(0);
    /* c2) Payload data: ILIP driver uses DMA to get data stored in PDU */
    pkt->dma_data_len       = htonl(in->data_len);
    put_dma_adresss(pkt, in->data);
    
//    sdh_be_v3_print(pkt); exit(1);
    return (sizeof(*pkt));       /* v2+ always sends 256 byte packet */
}
