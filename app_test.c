/*
 * APP_TEST.C
 *   Sample application using the CLOSURE API to communicate via GAPS hardware
 *
 * February 2020, Perspecta Labs
 */

#include "closure.h"

void pnt_set (pnt_datatype *pnt) {
    pnt->message_id  = 130;
    pnt->track_index = 165;
    pnt->lon         = 100;
    pnt->lon_frac    = 32768;
    pnt->lat         = 67;
    pnt->latfrac     = 49152;
    pnt->alt         = 2;
    pnt->altfrac     = 0;
}

int main(int argc, char **argv) {
  uint8_t       adu[ADU_SIZE_MAX];
  size_t        adu_len, pnt1_len;
  pnt_datatype  pnt1;
  gaps_tag      tag;
  uint32_t      mux=1, sec=2, typ=DATA_TYP_PNT;

  /* a) Create CLOSURE inputs (data and tag) */
  pnt1_len=(size_t) sizeof(pnt1);
  pnt_set(&pnt1); pnt_print(&pnt1);
  if (argc >= 2)  mux = atoi(argv[1]);
  tag_write(&tag, mux, sec, typ);
  /* b) Encode data and send to CLOSURE */
  gaps_data_encode(adu, &adu_len, (uint8_t *) &pnt1, &pnt1_len, typ);
  gaps_asyn_send(adu,  adu_len,  tag);
  /* c) Receive data from CLOSURE and decode */
  gaps_asyn_recv(adu, &adu_len, &tag);
  gaps_data_decode(adu, &adu_len, (uint8_t *) &pnt1, &pnt1_len, typ);
  fprintf(stderr, "app received "); tag_print(&tag); pnt_print(&pnt1);
  return (0);
}
