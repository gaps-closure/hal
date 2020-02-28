#include "hal.h"


/* Print M1 Packet */
void c_print(pkt_c *p) {
  
//  data_print("Data", (uint8_t *) p, 20);
  fprintf(stderr, "%s: ", __func__);
  fprintf(stderr, "mux=%u ",  ntohl(p->tag.mux));
  fprintf(stderr, "sec=%u ",  ntohl(p->tag.sec));
  fprintf(stderr, "typ=%u ",  ntohl(p->tag.typ));
  data_print("Data", p->data, ntohl(p->data_len));
  fprintf(stderr, "\n");
}

/* Print M1 Packet */
void m1_print(pkt_m1 *p) {
  tlv_m1 *tlv;
  
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

/* Print G1 Packet */
void g1_print(pkt_g1 *p) {
  fprintf(stderr, "%s: ", __func__);
  fprintf(stderr, "mux=%u ", ntohl(p->message_tag_ID));
  fprintf(stderr, "crc=%02x ", ntohs(p->crc16));
  data_print("Data", p->data, ntohs(p->data_len));
  fprintf(stderr, "\n");
}

/**********************************************************************/
/* PDU Decoding from packet arriving at HAL */
/*********t************************************************************/
/* Put closure packet (in buf) into internal HAL PDU structure (*p) */
void pdu_from_pkt_c (pdu *out, uint8_t *in) {
  pkt_c  *pkt = (pkt_c *) in;
  
//  fprintf(stderr, "%s: ", __func__); c_print(pkt);
  tag_decode(&(out->psel.tag), &(pkt->tag));
  len_decode(&(out->data_len), pkt->data_len);
//  fprintf(stderr, "LEN=%ld\n", out->data_len);
  memcpy(out->data, pkt->data, out->data_len);
}

/* Put data from buf (using M1 model) into internal HAL PDU */
void pdu_from_pkt_m1 (pdu *out, uint8_t *in, int len) {
  pkt_m1  *pkt = (pkt_m1 *) in;
  tlv_m1  *tlv;
    
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

/* Put data from buf (using G1 model) into internal HAL PDU */
void pdu_from_pkt_g1 (pdu *out, uint8_t *in , int len) {
  pkt_g1    *pkt = (pkt_g1 *) in;
  // if(hal_verbose) { fprintf(stderr, "HAL put packet (len = %d) into internal PDU: ", len); g1_print(pkt);
  out->psel.tag.mux = ntohl(pkt->message_tag_ID);
  out->psel.tag.sec = 0;         /* 0 is a don't care */
  out->psel.tag.typ = 0;         /* 0 is a don't care */
  out->data_len     = ntohs(pkt->data_len);
  memcpy (out->data, pkt->data, out->data_len);
}

/* Write packet into internal PDU */
void pdu_from_packet(pdu *out, uint8_t *in, int len_in, device *idev) {
  out->psel.dev = strdup(idev->id);
  if      (strcmp(idev->model, "pkt_c")  == 0)  pdu_from_pkt_c  (out, in);
  else if (strcmp(idev->model, "pkt_m1") == 0)  pdu_from_pkt_m1 (out, in, len_in);
  else if (strcmp(idev->model, "pkt_g1") == 0)  pdu_from_pkt_g1 (out, in, len_in);
  else {fprintf(stderr, "%s: unknown interface model: %s\n", __func__, idev->model); exit(EXIT_FAILURE);}
//  return (out);
}

/**********************************************************************/
/* PDU Encoding into packet sent by HAL */
/*********t************************************************************/
/* Put internal PDU into closure packet (in buf) */
int pdu_into_pkt_c (uint8_t *out, pdu *in, gaps_tag *otag) {
  size_t    pkt_len;
  pkt_c    *pkt = (pkt_c *) out;

  tag_encode(&(pkt->tag), otag);
  len_encode(&(pkt->data_len), in->data_len);
  memcpy(pkt->data, in->data, in->data_len);
  pkt_len = in->data_len + sizeof(pkt->tag) + sizeof(pkt->data_len);
  return (pkt_len);
}

/* Put data into buf (using M1 model) from internal HAL PDU */
/* Returns length of buffer */
int pdu_into_pkt_m1 (uint8_t *out, pdu *in, gaps_tag *otag) {
  pkt_m1  *pkt = (pkt_m1 *) out;
  tlv_m1  *tlv;

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

/* Put data into buf (using bkend model) from internal HAL PDU */
/* Returns length of buffer */
int pdu_into_pkt_g1 (uint8_t *out, pdu *in, gaps_tag *otag) {
  pkt_g1    *pkt = (pkt_g1 *) out;
  uint16_t  len = (uint16_t) in->data_len;

  pkt->message_tag_ID = htonl(otag->mux);
  pkt->data_len = htons(len);
  pkt->crc16 = 0;   /* XXX what polynomial? */
  memcpy((char *) pkt->data, (char *) in->data, in->data_len);
  return (sizeof(pkt->message_tag_ID) + sizeof(pkt->data_len) + sizeof(pkt->crc16) + in->data_len);
}

/* Write packet from internal PDU into packet */
void pdu_into_packet(uint8_t *out, pdu *in, int *pkt_len, gaps_tag *otag, const char *dev_model) {
  if      (strcmp(dev_model, "pkt_c")  == 0)  *pkt_len = pdu_into_pkt_c  (out, in, otag);
  else if (strcmp(dev_model, "pkt_m1") == 0)  *pkt_len = pdu_into_pkt_m1 (out, in, otag);
  else if (strcmp(dev_model, "pkt_g1") == 0)  *pkt_len = pdu_into_pkt_g1 (out, in, otag);
  else {fprintf(stderr, "%s unknown interface model %s", __func__, dev_model); exit(EXIT_FAILURE);}
}
