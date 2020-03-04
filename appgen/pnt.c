/*
 * PNT.C
 *   PNT Datatype functions
 *
 * February 2020
 * A. McAuley, Perspecta Labs (amcauley@perspectalabs.com)
 *
 * For description and revision history see README.txt
 */

#include "pnt.h"

void pnt_print (pnt_datatype *pnt) {
  fprintf(stderr, "PNT id=%d, tr=%d, lo=(%d.%d), lat=(%d.%d), alt=(%d.%d)\n",
  pnt->message_id,
  pnt->track_index,
  pnt->lon,
  pnt->lon_frac,
  pnt->lat,
  pnt->latfrac,
  pnt->alt,
  pnt->altfrac);
}

/*
 * Convert Data (TODO, Use DFDL schema)
 */
void pnt_data_encode (uint8_t *buff_out, size_t *len_out, uint8_t *buff_in, size_t *len_in) {
  pnt_datatype *p1 = (pnt_datatype *) buff_in;
  pnt_datatype *p2 = (pnt_datatype *) buff_out;
    
  p2->message_id  = htons(p1->message_id);
  p2->track_index = htons(p1->track_index);
  p2->lon         = htons(p1->lon);
  p2->lon_frac    = htons(p1->lon_frac);
  p2->lat         = htons(p1->lat);
  p2->latfrac     = htons(p1->latfrac);
  p2->alt         = htons(p1->alt);
  p2->altfrac     = htons(p1->altfrac);
  *len_out = *len_in;
}
  
/*
 * Convert Data (TODO, Use DFDL schema)
 */
void pnt_data_decode (uint8_t *buff_out, size_t *len_out, uint8_t *buff_in, size_t *len_in) {
  pnt_datatype *p1 = (pnt_datatype *) buff_out;
  pnt_datatype *p2 = (pnt_datatype *) buff_in;
    
  p2->message_id  = ntohs(p1->message_id);
  p2->track_index = ntohs(p1->track_index);
  p2->lon         = ntohs(p1->lon);
  p2->lon_frac    = ntohs(p1->lon_frac);
  p2->lat         = ntohs(p1->lat);
  p2->latfrac     = ntohs(p1->latfrac);
  p2->alt         = ntohs(p1->alt);
  p2->altfrac     = ntohs(p1->altfrac);
  *len_in = *len_out;
}
