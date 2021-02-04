/*
 * Convert between HAL PDU and Security Device Hardware (SDH) BE packet
 *   January 2021, Perspecta Labs
 *
 * v3 is the November 2020 EOP Payload Mode packet format.
 */

#include "hal.h"
#include "time.h"
#include "packetize_sdh_be_v3.h"
#include "map.h"            /* get data_print */

/* Print external packet  */
void sdh_be_v3_print(pkt_sdh_be_v3 *p) {
    fprintf(stderr, "%s (l=%ld): ", __func__, sizeof(*p));
    fprintf(stderr, "mux=%u ",        ntohl(p->session_tag));
    fprintf(stderr, "sec=%u ",        ntohl(p->message_tag));
    fprintf(stderr, "typ=%u ",        ntohl(p->data_tag));
  
    fprintf(stderr, "dtyp=%08x ",     p->descriptor_type);
    fprintf(stderr, "dtag=%u ",       ntohl(p->descriptor_tag));
    fprintf(stderr, "dest=%u ",       ntohl(p->destination_tag));
    fprintf(stderr, "gaps=%x.%x ",    p->gaps_time_up,  p->gaps_time_lo);
    fprintf(stderr, "unix=%x.%x ",    p->linux_time_up, p->linux_time_lo);
    fprintf(stderr, "len=%d ",        ntohl(p->dma_data_len));
//    fprintf(stderr, "Addr=%08x-%08x ", p->dma_data_addr_up,
//                                      p->dma_data_addr_lo);
    fprintf(stderr, "A=%p ", (void *) ((uint64_t) p->dma_data_addr_lo));
    fprintf(stderr, "\n");
}

/* get size of packet (= header length + data length) */
/*   v3 (Mercury12) sends payload PLUS 256 byte packet */
int get_packet_length_sdh_be_v3(pkt_sdh_be_v3 *pkt, size_t data_len) {
  return (sizeof(*pkt) + data_len);
}

/* Put data from external packet (*in) into internal HAL PDU */
int pdu_from_sdh_be_v3 (pdu *out, uint8_t *in, int len_in) {
    pkt_sdh_be_v3  *pkt = (pkt_sdh_be_v3 *) in;
    uint8_t        *data_in;

//    fprintf(stderr, "%s: ", __func__); sdh_be_v3_print(pkt);
    out->data_len     = ntohl(pkt->dma_data_len);
    if (get_packet_length_sdh_be_v3(pkt, 0) > len_in)  return (-1);  /* incomplete packet */
  
    out->psel.tag.mux = ntohl(pkt->session_tag);
    out->psel.tag.sec = ntohl(pkt->message_tag);
    out->psel.tag.typ = ntohl(pkt->data_tag);
    data_in = (uint8_t *) pkt + sizeof(*pkt);
//   memcpy (out->data, data_in, out->data_len);    /* TODO_PDU_PTR */
    out->data = data_in;    /* TODO_PDU_PTR */
  return (get_packet_length_sdh_be_v3(pkt, out->data_len));
}

/* Put data into external packet (*out) from internal HAL PDU */
/* Returns length of external packet */
int pdu_into_sdh_be_v3 (uint8_t *out, pdu *in, gaps_tag *otag) {
    pkt_sdh_be_v3  *pkt = (pkt_sdh_be_v3 *) out;
    
    // a) Copy tag info and descriptor into packet
    pkt->session_tag        = htonl(otag->mux);
    pkt->message_tag        = htonl(otag->sec);
    pkt->descriptor_tag     = htonl(otag->typ);
    
    // Data tag is the payload data tag distinct from mux/sec/typ
    // High order bit must be 1
    pkt->data_tag           = htonl((otag->typ) | 0x80000000);

    /* CDG Allow rule is mux, sec, type , but Redact rule is mux, sec, desc */
    // Descriptor_type determines immediate vs. payload mode
    pkt->descriptor_type    = (1u & 0xF) << 28;
  
    // b) Timestamps set by driver (linux) or ILIP (gaps)
//    pkt->gaps_time_lo = htonl(0x01234567);  /* XXX: Just set for testing */
//    pkt->gaps_time_up = htonl(0x89abcdef);  /* XXX: Just set for testing */
    pkt->gaps_time_lo       = 0;
    pkt->gaps_time_up       = 0;
    pkt->linux_time_lo      = 0;
    pkt->linux_time_up      = 0;
    

    /* c1) Immediate data: can be copied into packet, but disable for now */
    pkt->imm_data_len       = htonl(0);
    pkt->destination_tag    = 0;
    /* c2) Payload data: ILIP driver uses DMA to get data stored in PDU */
    pkt->dma_data_len       = htonl(in->data_len);
    pkt->dma_data_addr_lo   = (uint64_t) in->data;
  
//  log_buf_trace((char *) __func__, (char *) out, sizeof(*pkt) + in->data_len);
//  data_print("Data",  (uint8_t *) pkt->dma_data_addr_lo, in->data_len);
//  sdh_be_v3_print(pkt); // exit(1);
//    return (sizeof(*pkt));       /* v2+ always sends 256 byte packet */
    return (get_packet_length_sdh_be_v3(pkt, in->data_len));
}
