/* Convert between Internal HAL PDU and any external packet */

#include "packetize_sdh_be_v1.h"
#include "packetize_sdh_be_v2.h"
#include "packetize_sdh_bw_v1.h"
#include "packetize_sdh_ha_v1.h"

extern void pdu_from_packet(pdu *, uint8_t *, int, device *);
extern void pdu_into_packet(uint8_t *, pdu *, int *, selector *, const char *);
