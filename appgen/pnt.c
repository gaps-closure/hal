/*
 * PNT Datatype API functions (hardcoded)
 *   March 2020, Perspecta Labs
 */

#include "pnt.h"

void pnt_print (pnt_datatype *pnt) {
  fprintf(stderr, "PNT (len=%ld): id=%d, tr=%d, lo=(%d.%d), lat=(%d.%d), alt=(%d.%d)\n", sizeof(*pnt), 
  pnt->MessageID,
  pnt->TrackIndex,
  pnt->Lon,
  pnt->LonFrac,
  pnt->Lat,
  pnt->LatFrac,
  pnt->Alt,
  pnt->AltFrac);
}

/*
 * Convert Data (TODO, Use DFDL schema)
 */
void pnt_data_encode (uint8_t *buff_out, size_t *len_out, uint8_t *buff_in, size_t *len_in) {
  pnt_datatype *p1 = (pnt_datatype *) buff_in;
  pnt_datatype *p2 = (pnt_datatype *) buff_out;
    
  p2->MessageID  = htons(p1->MessageID);
  p2->TrackIndex = htons(p1->TrackIndex);
  p2->Lon         = htons(p1->Lon);
  p2->LonFrac    = htons(p1->LonFrac);
  p2->Lat         = htons(p1->Lat);
  p2->LatFrac     = htons(p1->LatFrac);
  p2->Alt         = htons(p1->Alt);
  p2->AltFrac     = htons(p1->AltFrac);
  *len_out = *len_in;
}
  
/*
 * Convert Data (TODO, Use DFDL schema)
 */
void pnt_data_decode (uint8_t *buff_out, size_t *len_out, uint8_t *buff_in, size_t *len_in) {
  pnt_datatype *p1 = (pnt_datatype *) buff_out;
  pnt_datatype *p2 = (pnt_datatype *) buff_in;
    
  p2->MessageID  = ntohs(p1->MessageID);
  p2->TrackIndex = ntohs(p1->TrackIndex);
  p2->Lon         = ntohs(p1->Lon);
  p2->LonFrac     = ntohs(p1->LonFrac);
  p2->Lat         = ntohs(p1->Lat);
  p2->LatFrac     = ntohs(p1->LatFrac);
  p2->Alt         = ntohs(p1->Alt);
  p2->AltFrac     = ntohs(p1->AltFrac);
  *len_in = *len_out;
}
