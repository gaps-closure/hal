/*
 * HAL Map Lookup
 *   December 2020, Perspecta Labs
 */

#include "hal.h"

/**********************************************************************/
/* HAL Print (structure information) */
/**********************************************************************/
/* print halmap selector */
void selector_print(selector *s, FILE *fd) {
  if (fd == NULL) return;
  fprintf(fd, "%s", s->dev);
  if (s->ctag < 0) tag_print(&(s->tag), fd);
  else             fprintf(fd, "[ctag=0x%08x]      ", s->ctag);
}

/* Print raw data in Network Byte Order (Bigendian) of given length */
void data_print(const char *str, uint8_t *data, size_t data_len) {
  fprintf(stderr, "%s (len=%ld)", str, data_len);
  for (int i = 0; i < data_len; i++) {
    if ((i%4)==0) fprintf(stderr, " ");
    fprintf(stderr, "%02X", data[i]);
  }
  fprintf(stderr, "\n");
}

/* Print a information from an internal PDU */
void log_log_pdu(int level, pdu *pdu, const char *fn) {
  FILE *fd[2];
  int   i;
  
  log_get_fds(level, &fd[0], &fd[1]);
  for (i=0; i<2; i++) {
    if (fd[i] != NULL) {
      if (pdu == NULL)  log_warn("Cannot print NULL PDU");
      else {
        fprintf(fd[i], "PDU dev=%s (from %s) ", pdu->psel.dev, fn);
        selector_print(&(pdu->psel), fd[i]);
        data_print("Encoded-Data", pdu->data, pdu->data_len);
      }
    }
  }
}

/* Print a single HAL map entry for debugging */
void halmap_print_one(halmap *hm, FILE *fd) {
  if (fd == NULL) return;
  fprintf(fd, "   ");
  selector_print(&(hm->from), fd);
  fprintf(fd, "-> ");
  selector_print(&(hm->to), fd);
  fprintf(fd, ", codec=%s\n", hm->codec);
}

/* Print list of HAL map entries for debugging */
void log_log_halmap(int level, halmap *map_root, const char *fn) {
  FILE *fd[2];
  int   i;
  
  log_get_fds(level, &fd[0], &fd[1]);
  for (i=0; i<2; i++) {
    if (fd[i] != NULL) {
      fprintf(fd[i], "  HAL map list (from %s):\n", fn);
      for(halmap *hm = map_root; hm != NULL; hm = hm->next) halmap_print_one(hm, fd[i]);
      fflush (fd[i]);
    }
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

//  fprintf(stderr, "%s", __func__);
  for(halmap *hm = map_root; hm != NULL; hm = hm->next) {
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
  log_warn("Could not find tag <%d, %d, %d> from %s", tag->mux, tag->sec, tag->typ, p->psel.dev);
  return (halmap *) NULL;
}
