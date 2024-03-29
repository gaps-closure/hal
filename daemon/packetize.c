/*
 * Convert between HAL PDU and external packet of the Security Device Hardware (SDH)
 *   January 2021, Perspecta Labs
 */

#include "hal.h"
#include "packetize.h"

/* Write packet into internal PDU */
int pdu_from_packet(pdu *out, uint8_t *in, int len_in, device *idev) {
  int pdu_len = 0;        // PDU contents default to invalid
    
  out->psel.dev  = strdup(idev->id);
  out->psel.ctag = -1;
    log_trace("Packizer reads packet from %s of len=%d", idev->model, len_in);

    
  if      (strcmp(idev->model, "sdh_ha_v1")    == 0) pdu_len = pdu_from_sdh_ha_v1 (out, in, len_in);
  else if (strcmp(idev->model, "sdh_socat_v1") == 0) pdu_len = pdu_from_sdh_ha_v1 (out, in, len_in);
  else if (strcmp(idev->model, "sdh_be_v1")    == 0) pdu_len = pdu_from_sdh_be_v1 (out, in, len_in);
  else if (strcmp(idev->model, "sdh_be_v2")    == 0) pdu_len = pdu_from_sdh_be_v2 (out, in, len_in);
  else if (strcmp(idev->model, "sdh_be_v3")    == 0) pdu_len = pdu_from_sdh_be_v3 (out, in, len_in);
  else if (strcmp(idev->model, "sdh_bw_v1")    == 0) pdu_len = pdu_from_sdh_bw_v1 (out, in, len_in);
  else {log_fatal("%s: unknown interface model: %s", __func__, idev->model); exit(EXIT_FAILURE);}
  return (pdu_len);
}

/* Write packet from internal PDU into packet */
void pdu_into_packet(uint8_t *out, pdu *in, int *pkt_len, selector *osel, const char *dev_model) {
  if      (strcmp(dev_model, "sdh_ha_v1") == 0)  *pkt_len = pdu_into_sdh_ha_v1 (out, in, &(osel->tag));
  else if (strcmp(dev_model, "sdh_socat_v1") == 0)  *pkt_len = pdu_into_sdh_ha_v1 (out, in, &(osel->tag));
  else if (strcmp(dev_model, "sdh_be_v1") == 0)  *pkt_len = pdu_into_sdh_be_v1 (out, in, &(osel->tag));
  else if (strcmp(dev_model, "sdh_be_v2") == 0)  *pkt_len = pdu_into_sdh_be_v2 (out, in, &(osel->tag));
  else if (strcmp(dev_model, "sdh_be_v3") == 0)  *pkt_len = pdu_into_sdh_be_v3 (out, in, &(osel->tag));
  else if (strcmp(dev_model, "sdh_bw_v1") == 0)  *pkt_len = pdu_into_sdh_bw_v1 (out, in, osel->ctag);
  else {
    log_fatal("%s unknown interface model %s", __func__, dev_model);
    exit(EXIT_FAILURE);
  }
}
