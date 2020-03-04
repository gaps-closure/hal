/* Convert between Internal HAL PDU and any external packet */

#include "packetize_sdh-be-v1.h"
#include "packetize_sdh-bw-v1.h"

extern void pdu_from_packet(pdu *, uint8_t *, int, device *);
extern void pdu_into_packet(uint8_t *, pdu *, int *, gaps_tag *, const char *);
