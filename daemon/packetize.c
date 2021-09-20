/*
 * Convert between HAL PDU (see hal.h) and an packet or shared memory
 *   HAL communicates with a book-end (sdh_be_*), bump-in-the-wire (shh_bw_*),
 *   shared-memory (sdh_sm_*), or a direct serial link (sdh_socat_*) Security
 *   Device Hardware (sdh). HAL also communicates with applications (sdh_ha_*).
 *   Packet or shared memory formats are definied in packetize_sdh_*.h structures
 *
 * September 2021, Perspecta Labs
 */

#include "hal.h"
#include "packetize.h"

/* Write into internal PDU (out) from formated packet (in) or from a shared memory device (idev) */
int pdu_from_packet(pdu *out, uint8_t *in, int len_in, device *idev) {
  int pdu_len = 0;        // PDU contents default to invalid
  out->psel.dev  = strdup(idev->id);
  out->psel.ctag = -1;
  if      (strcmp(idev->model, "sdh_ha_v1")    == 0) pdu_len = pdu_from_sdh_ha_v1 (out, in, len_in);
  else if (strcmp(idev->model, "sdh_socat_v1") == 0) pdu_len = pdu_from_sdh_ha_v1 (out, in, len_in);
  else if (strcmp(idev->model, "sdh_be_v1")    == 0) pdu_len = pdu_from_sdh_be_v1 (out, in, len_in);
  else if (strcmp(idev->model, "sdh_be_v2")    == 0) pdu_len = pdu_from_sdh_be_v2 (out, in, len_in);
  else if (strcmp(idev->model, "sdh_be_v3")    == 0) pdu_len = pdu_from_sdh_be_v3 (out, in, len_in);
  else if (strcmp(idev->model, "sdh_bw_v1")    == 0) pdu_len = pdu_from_sdh_bw_v1 (out, in, len_in);
  else if (strcmp(idev->model, "sdh_sm_v1")    == 0) pdu_len = pdu_from_sdh_sm_v1 (out, idev);
  else {log_fatal("%s: unknown interface model: %s", __func__, idev->model); exit(EXIT_FAILURE);}
  return (pdu_len);
}

/* Write internal PDU (in) into packet (out) or directiy to a device (odev) */
void pdu_into_packet(uint8_t *out, pdu *in, int *pkt_len, selector *osel, device *odev) {
  const char *dev_model = odev->model;
  gaps_tag   *otag      = &(osel->tag);
  if      (strcmp(dev_model, "sdh_ha_v1") == 0)     *pkt_len = pdu_into_sdh_ha_v1 (out,  in, otag);
  else if (strcmp(dev_model, "sdh_socat_v1") == 0)  *pkt_len = pdu_into_sdh_ha_v1 (out,  in, otag);
  else if (strcmp(dev_model, "sdh_be_v1") == 0)     *pkt_len = pdu_into_sdh_be_v1 (out,  in, otag);
  else if (strcmp(dev_model, "sdh_be_v2") == 0)     *pkt_len = pdu_into_sdh_be_v2 (out,  in, otag);
  else if (strcmp(dev_model, "sdh_be_v3") == 0)     *pkt_len = pdu_into_sdh_be_v3 (out,  in, otag);
  else if (strcmp(dev_model, "sdh_bw_v1") == 0)     *pkt_len = pdu_into_sdh_bw_v1 (out,  in, osel->ctag);
  else if (strcmp(dev_model, "sdh_sm_v1") == 0)     *pkt_len = pdu_into_sdh_sm_v1 (odev, in, otag);
  else {log_fatal("%s unknown interface model %s", __func__, dev_model); exit(EXIT_FAILURE);}
}
