/* Convert between Internal HAL PDU and any external packet */

#include "hal.h"
#include "packetize.h"

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

/* Put closure packet (in buf) into internal HAL PDU structure (*p) */
void pdu_from_pkt_c (pdu *out, uint8_t *in) {
  pkt_c  *pkt = (pkt_c *) in;
  
//  fprintf(stderr, "%s: ", __func__); c_print(pkt);
  tag_decode(&(out->psel.tag), &(pkt->tag));
  len_decode(&(out->data_len), pkt->data_len);
//  fprintf(stderr, "LEN=%ld\n", out->data_len);
  memcpy(out->data, pkt->data, out->data_len);
}

/* Write packet into internal PDU */
void pdu_from_packet(pdu *out, uint8_t *in, int len_in, device *idev) {
  out->psel.dev = strdup(idev->id);
  if      (strcmp(idev->model, "pkt_c")  == 0)  pdu_from_pkt_c  (out, in);
  else if (strcmp(idev->model, "pkt_m1") == 0)  pdu_from_pkt_m1 (out, in, len_in);
  else if (strcmp(idev->model, "pkt_m2") == 0)  pdu_from_pkt_m1 (out, in, len_in);
  else if (strcmp(idev->model, "pkt_g1") == 0)  pdu_from_pkt_g1 (out, in, len_in);
  else {fprintf(stderr, "%s: unknown interface model: %s\n", __func__, idev->model); exit(EXIT_FAILURE);}
//  return (out);
}

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

/* Write packet from internal PDU into packet */
void pdu_into_packet(uint8_t *out, pdu *in, int *pkt_len, gaps_tag *otag, const char *dev_model) {
  if      (strcmp(dev_model, "pkt_c")  == 0)  *pkt_len = pdu_into_pkt_c  (out, in, otag);
  else if (strcmp(dev_model, "pkt_m1") == 0)  *pkt_len = pdu_into_pkt_m1 (out, in, otag);
  else if (strcmp(dev_model, "pkt_m2") == 0)  *pkt_len = pdu_into_pkt_m2 (out, in, otag);
  else if (strcmp(dev_model, "pkt_g1") == 0)  *pkt_len = pdu_into_pkt_g1 (out, in, otag);
  else {fprintf(stderr, "%s unknown interface model %s", __func__, dev_model); exit(EXIT_FAILURE);}
}
