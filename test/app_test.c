/*
 * APP_TEST.C
 *   Sample test application v1, using HAL API to communicate via GAPS hardware.
 *   Sends and receives (or revieves and sends) one data type.
 *
 * March 2020, Perspecta Labs
 */

#include "../api/xdcomms.h"
#include "../appgen/pnt.h"

void pnt_set (uint8_t *adu, size_t *len) {
  pnt_datatype  *pnt = (pnt_datatype *) adu;
  pnt->MessageID  = 130;
  pnt->TrackIndex = 165;
  pnt->Lon        = 100;
  pnt->LonFrac    = 32768;
  pnt->Lat        = 67;
  pnt->LatFrac    = 49152;
  pnt->Alt        = 2;
  pnt->AltFrac    = 0;
  *len=(size_t) sizeof(*pnt);
  pnt_print(pnt);
}

int main(int argc, char **argv) {
  int           mode=0;               /* send first */
  uint8_t       adu[ADU_SIZE_MAX_C];
  size_t        adu_len;
  gaps_tag      t_tag, r_tag;
  uint32_t      t_mux=1, t_sec=1, typ=DATA_TYP_PNT, r_mux=1, r_sec=1;

  pnt_set(adu, &adu_len);
  if (argc >= 2) { t_mux = atoi(argv[1]); t_sec = t_mux; r_mux = t_mux; r_sec = t_mux; }
  if (argc >= 3) { r_mux = atoi(argv[2]); r_sec = r_mux; }
  if (argc >= 4) { mode = 1; }       /* receive first */
  tag_write(&t_tag, t_mux, t_sec, typ);  tag_write(&r_tag, r_mux, r_sec, typ);
  /* Low level function uses manually created encode and decode functions (per data type). This will be replaced by xdc_generate) */
  xdc_register(pnt_data_encode, pnt_data_decode, typ);
  if (mode == 0) {xdc_asyn_send(adu,  adu_len,  t_tag);}
                  xdc_asyn_recv(adu, &adu_len, &r_tag);
  fprintf(stderr, "app rx "); tag_print(&r_tag); pnt_print((pnt_datatype *)adu);
  if (mode == 1) {xdc_asyn_send(adu,  adu_len,  t_tag);}
  return (0);
}
