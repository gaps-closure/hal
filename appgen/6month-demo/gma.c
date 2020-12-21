/*
 * gma.c
 *   Datatype functions
 *
 * December 2020
 *
 * For description see README.txt
 */
#include <stdint.h>
#include <inttypes.h>

#include "float754.h"
#include "gma.h"

/* 0) Common Trailer Functions */
void trailer_print(trailer_datatype *tlr) {
  fprintf(stderr, "%d, %d, %d, %d, %d",
          tlr->seq,
          tlr->rqr,
          tlr->oid,
          tlr->mid,
          tlr->crc);
}

void trailer_into_packet(trailer_datatype *tlr_out, trailer_datatype *tlr_in) {
  tlr_out->seq = htonl(tlr_in->seq);
  tlr_out->rqr = htonl(tlr_in->rqr);
  tlr_out->oid = htonl(tlr_in->oid);
  tlr_out->mid = htons(tlr_in->mid);
  tlr_out->crc = htons(tlr_in->crc);
}

void trailer_from_packet(trailer_datatype *tlr_out, trailer_datatype *tlr_in) {
  tlr_out->seq = ntohl(tlr_in->seq);
  tlr_out->rqr = ntohl(tlr_in->rqr);
  tlr_out->oid = ntohl(tlr_in->oid);
  tlr_out->mid = ntohl(tlr_in->mid);
  tlr_out->crc = ntohl(tlr_in->crc);
}

/* 1) Position Functions */
void position_print (position_datatype *position) {
  fprintf(stderr, "position (len=%ld): %f, %f, %f, ", sizeof(*position),
          position->x,
          position->y,
          position->z);
  trailer_print(&(position->trailer));
  fprintf(stderr, "\n");
}

void position_data_encode (void *buff_out, void *buff_in, size_t *len_out) {
  position_datatype *p1 = (position_datatype *) buff_in;
  position_output   *p2 = (position_output *)   buff_out;

  p2->x  = htond(p1->x);
  p2->y  = htond(p1->y);
  p2->z  = htond(p1->z);
  trailer_into_packet(&(p2->trailer), &(p1->trailer));
  *len_out = sizeof(p1->x) * 3 + sizeof(trailer_datatype);
}

void position_data_decode (void *buff_out, void *buff_in, size_t *len_in) {
  position_output   *p1 = (position_output *)   buff_in;
  position_datatype *p2 = (position_datatype *) buff_out;

  p2->x  = ntohd(p1->x);
  p2->y  = ntohd(p1->y);
  p2->z  = ntohd(p1->z);
  trailer_from_packet(&(p2->trailer), &(p1->trailer));
}

/* 2) Distance Functions */
void distance_print (distance_datatype *distance) {
  fprintf(stderr, "distance (len=%ld): %f, %f, %f, ", sizeof(*distance),
          distance->x,
          distance->y,
          distance->z);
  trailer_print(&(distance->trailer));
  fprintf(stderr, "\n");

}

void distance_data_encode (void *buff_out, void *buff_in, size_t *len_out) {
  distance_datatype *p1 = (distance_datatype *) buff_in;
  distance_output   *p2 = (distance_output *)   buff_out;

  p2->x  = htond(p1->x);
  p2->y  = htond(p1->y);
  p2->z  = htond(p1->z);
  trailer_into_packet(&(p2->trailer), &(p1->trailer));
  *len_out = sizeof(p1->x) * 3 + sizeof(trailer_datatype);
}

void distance_data_decode (void *buff_out, void *buff_in, size_t *len_in) {
  distance_output   *p1 = (distance_output *)   buff_in;
  distance_datatype *p2 = (distance_datatype *) buff_out;

  p2->x  = ntohd(p1->x);
  p2->y  = ntohd(p1->y);
  p2->z  = ntohd(p1->z);
  trailer_from_packet(&(p2->trailer), &(p1->trailer));
}

/* 3) Raw Functions */
void raw_print (raw_datatype *raw) {
  int              i;
  uint32_t  data_len  = raw->data_len;
  uint32_t  total_len = data_len + sizeof(*raw);
  uint8_t       *d_in = (uint8_t *) (raw + 1);    /* data is after raw_datatype struct */

  fprintf(stderr, "raw (len=%d): %08X, ", total_len, data_len);
  trailer_print(&(raw->trailer));
  for (i = 0; i < data_len; i++) {
    if ((i%4)==0) fprintf(stderr, ", ");
    fprintf(stderr, "%02X", d_in[i]);
  }
  fprintf(stderr, "\n");
}

void raw_data_encode (void *buff_out, void *buff_in, size_t *len_out) {
  raw_datatype   *p1 = (raw_datatype *) buff_in;
  raw_datatype   *p2 = (raw_datatype *) buff_out;
  uint32_t  data_len = p1->data_len;
  char         *d_in = (char *) (p1 + 1);
  char        *d_out = (char *) (p2 + 1);

  p2->data_len  = htonl(data_len);
// fprintf(stderr, "%s: ", __func__); raw_print (p1);
  trailer_into_packet(&(p2->trailer), &(p1->trailer));
  memcpy(d_out, d_in, data_len);
  *len_out = sizeof(*p1) + data_len;
}

void raw_data_decode (void *buff_out, void *buff_in, size_t *len_in) {
  raw_datatype   *p1 = (raw_datatype *) buff_in;
  raw_datatype   *p2 = (raw_datatype *) buff_out;
  uint32_t  data_len = ntohl(p1->data_len);
  char         *d_in = (char *) (p1 + 1);
  char        *d_out = (char *) (p2 + 1);

  p2->data_len  = data_len;
  trailer_from_packet(&(p2->trailer), &(p1->trailer));
  memcpy(d_out, d_in, data_len);
// fprintf(stderr, "%s len: %ld->%d\n", __func__, *len_in, p2->data_len);
//raw_print (p1);
}
