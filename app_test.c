/*
 * APP_TEST.C
 *   Sample application using the CLOSURE API to communicate via GAPS hardware
 *
 * March 2020, Perspecta Labs
 */

#include "api/xdcomms.h"
#include "appgen/pnt.h"

void pnt_set (uint8_t *adu, size_t *len) {
  pnt_datatype  *pnt = (pnt_datatype *) adu;
  pnt->message_id  = 130;
  pnt->track_index = 165;
  pnt->lon         = 100;
  pnt->lon_frac    = 32768;
  pnt->lat         = 67;
  pnt->latfrac     = 49152;
  pnt->alt         = 2;
  pnt->altfrac     = 0;
  *len=(size_t) sizeof(*pnt);
  pnt_print(pnt);
}

int main(int argc, char **argv) {
  uint8_t       adu[ADU_SIZE_MAX_C];
  size_t        adu_len;
  gaps_tag      tag;
  uint32_t      mux=1, sec=2, typ=DATA_TYP_PNT;

  pnt_set(adu, &adu_len);
  if (argc >= 2)  mux = atoi(argv[1]);
  tag_write(&tag, mux, sec, typ);
  /* Low level function uses manually created encode and decode functions (per data type). This will be replaced by xdc_generate) */
  xdc_register(pnt_data_encode, pnt_data_decode, typ);
  xdc_asyn_send(adu,  adu_len,  tag);
  xdc_asyn_recv(adu, &adu_len, &tag);
  fprintf(stderr, "app rx "); tag_print(&tag); pnt_print((pnt_datatype *)adu);
  return (0);
}
