/* Define GAPS Packet Format for SDH BW */
/* Note: structure sdh_ha_v1 is defined in ../api/xdcomms.h */

void pdu_from_sdh_ha_v1 (pdu *, uint8_t *);
int  pdu_into_sdh_ha_v1 (uint8_t *, pdu *, gaps_tag *);
