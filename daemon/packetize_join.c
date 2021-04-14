/*
 * Combine packets from a sequence of input receive buffers
 *   April 2021, Perspecta Labs
 *
 * Based on the Generic Framing Protocol (GFP) using CRC to check for
 * start of frame (no escape sequences, so has low fixed overhead).
 * Robust to packet loos (clears buffer when a new PDU arrives),
 * but assumes Start of PDU is always at the start of a packet
 */

#include "hal.h"
#include "packetize_join.h"

/* Save and combine partial packets
 * If packet completed, then return its length and set sbuf = prs->buf (not input packet)
 */
int packet_combiner(uint8_t *in, int len_in, int len_pkt, uint8_t **sbuf, pkt_reaas *prs) {
  if (len_pkt < 0)  prs->buf_len = 0;         // Clear old partial packet
  else if (len_pkt > 0) {                     // X2) Create new partial packet
    memcpy (prs->buf, in, len_in);
    prs->buf_len   = len_in;
    prs->pkt_len   = len_pkt;
    log_debug("Saving incomplete packet (%d of %d bytes)", prs->buf_len, prs->pkt_len);
  }
  else {
    if (prs->buf_len > 0) {                   // Add to existing partial packet
      memcpy ((prs->buf) + (prs->buf_len), in, len_in);
      prs->buf_len += len_in;
      log_debug("Adding to saved packet (X3, X4): now have %d of %d bytes", prs->buf_len, prs->pkt_len);
      if (prs->buf_len >= prs->pkt_len) {     // X3) packet is completes
        *sbuf = prs->buf;                    // Point to saved buffer (not default input packet)
        prs->buf_len = 0;                     // Clear old partial packet
        return (prs->pkt_len);
      }
    }
    log_debug("Saved packet (len=%d of %d) still incomplete (X4) or no saved packet (X5) ", prs->buf_len, prs->pkt_len);
  }
  return (-1);                                // Saved packet is incomplete
}

/* Parse input packet: X1=Good, X2=start, X3=end, X4=middle, X5=bad */
int packet_parser(uint8_t *in, int len_in, int len_pkt, uint8_t **sbuf, bool start_of_packet, pkt_reaas *prs) {
  if (start_of_packet) {
    if (len_pkt <= len_in) {                                  // X1) input packet is complete
      packet_combiner((uint8_t *) NULL, -1, -1, (uint8_t **) NULL, prs);  // Clear old partial packet
      return (len_pkt);
    }
    return (packet_combiner(in, len_in, len_pkt, sbuf, prs)); // X2) New saved pkt
  }
  return (packet_combiner(in, len_in, 0, sbuf, prs));         // X3-X5) Add to saved pkt
}
