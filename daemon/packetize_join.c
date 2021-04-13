/*
 * Combine packets from a sequence of input receive buffers
 *   April 2021, Perspecta Labs
 *
 * Based loosely on the Generic Framing Protocol (GFP) using CRC to check
 * start of frame, with no escape sequences, so has low fixed overhead.
 *    - Assumes start of PDU is always at the start of a packet
 *    - robust to packet loos (clears buffer when a new PDU arrives)
 */

#include "hal.h"

/* Save and combine partial packets
 * If packet completed, then return its length and a pointer to static buffer
 */
int packet_combiner(uint8_t *in, int len_in, int len_pkt, pdu *out, uint8_t **sbuf) {
  static uint8_t  saved_buf[ADU_SIZE_MAX_C];
  static int      saved_buf_len;
  static int      saved_pkt_len;
  
  if (len_pkt < 0)  saved_buf_len = 0;        // Clear old partial packet
  else if (len_pkt > 0) {                     // X2) Create new partial packet
    memcpy (saved_buf, in, len_in);
    saved_buf_len  = len_in;
    saved_pkt_len  = len_pkt;
    log_debug("Saving incomplete packet (%d of %d bytes)", saved_buf_len, saved_pkt_len);
  }
  else {
    if (saved_buf_len > 0) {                  // Add to existing partial packet
      memcpy (saved_buf + saved_buf_len, in, len_in);
      saved_buf_len += len_in;
      log_debug("Adding to saved packet (X3, X4): now have %d of %d bytes", saved_buf_len, saved_pkt_len);
      if (saved_buf_len >= saved_pkt_len) {   // X3) packet is completes
        *sbuf = saved_buf;                    // Point to saved buffer (not default input packet)
        saved_buf_len = 0;                    // Clear old partial packet
        return (saved_pkt_len);
      }
    }
    log_debug("Saved packet (len=%d) still incomplete (X4) or no saved packet (X5) ", saved_buf_len);
  }
  return (-1);                                // Saved packet is incomplete
}

/* Parse packet: X1=Good, X2=start, X3=end, X4=middle, X5=bad */
int packet_parser(uint8_t *in, int len_in, int len_pkt, pdu *out, uint8_t **sbuf, bool start_of_packet) {
  if (start_of_packet) {
    if (len_pkt <= len_in) {    // X1) Good packet that is complete
      packet_combiner((uint8_t *) NULL, -1, -1, NULL, (uint8_t **) NULL);  // Clear old partial packet
      return (len_pkt);
    }
    return (packet_combiner(in, len_in, len_pkt, NULL, sbuf));  // X2) New saved pkt
  }
  return (packet_combiner(in, len_in, 0, out, sbuf));  // X3-X5) Add to saved pkt
}
