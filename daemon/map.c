/*
 * HAL Map Lookup
 *   March 2020, Perspecta Labs
 */

#include "hal.h"

/**********************************************************************/
/* HAL Print (structure information) */
/**********************************************************************/
/* print halmap selector */
void selector_print(selector *s) {
  fprintf(stderr, " %s ", s->dev);
  if (s->ctag < 0) tag_print(&(s->tag));
  else             fprintf(stderr, "[ctag=0x%08x]      ", s->ctag);
}

/* Print a information from an internal PDU */
void pdu_print(pdu *pdu) {
  if (pdu == NULL)  fprintf(stderr, "Cannot print NULL PDU\n");
  else {
    fprintf(stderr, "PDU dev=%s ", pdu->psel.dev);
    selector_print(&(pdu->psel));
    data_print("Encoded-Data", pdu->data, pdu->data_len);
  }
}

/* Print a single HAL map entry for debugging */
void halmap_print_one(halmap *hm) {
  selector_print(&(hm->from));
  fprintf(stderr, "-> ");
  selector_print(&(hm->to));
  fprintf(stderr, ", codec=%s\n", hm->codec);
}

/* Print list of HAL map entries for debugging */
void halmap_print_all(halmap *map_root) {
    fprintf(stderr, "HAL map list:\n");
    for(halmap *hm = map_root; hm != NULL; hm = hm->next) {
      halmap_print_one(hm);
    }
}

/**********************************************************************/
/* HAL MAP processing */
/*********t************************************************************/

/* Return halmap with from selector matching PDU selector from the halmap list */
halmap *halmap_find(pdu *p, halmap *map_root) {
  selector *hsel;
  selector *psel = &(p->psel);
  gaps_tag *tag  = &(psel->tag);
  int       ctag = psel->ctag;

//  fprintf(stderr, "%s", __func__); pdu_print(p);
  for(halmap *hm = map_root; hm != NULL; hm = hm->next) {
//    halmap_print_one(hm);
    if (strcmp(hm->from.dev, p->psel.dev) == 0) {
      hsel = &(hm->from);
      if (ctag < 0) {
         if ( (hsel->tag.mux == tag->mux)
           && (hsel->tag.sec == tag->sec)
           && (hsel->tag.typ == tag->typ)
             ) {
           return (hm);
         }
      }
      else
      {
        if (hsel->ctag == ctag) {
          return (hm);
        }
      }
    }
  }
  return (halmap *) NULL;
}
