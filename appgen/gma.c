/*
 * gma.c
 *   Datatype functions
 *
 * March 2020
 *
 * For description and revision history see README.txt
 */
#include <stdint.h>
#include <inttypes.h>

#include "float.h"
#include "gma.h"

void position_print (position_datatype *position) {
  fprintf(stderr, "position (len=%ld): %f, %f, %f\n", sizeof(*position),
  position->x,
  position->y,
  position->z);
}

/*
 * Convert Data (TODO, Use DFDL schema)
 */
void position_data_encode (uint8_t *buff_out, size_t *len_out, uint8_t *buff_in, size_t *len_in) {
  position_datatype *p1 = (position_datatype *) buff_in;
  position_output   *p2 = (position_output *)   buff_out;

  p2->x  = pack754_be(p1->x);
  p2->y  = pack754_be(p1->y);
  p2->z  = pack754_be(p1->z);
  *len_out = *len_in;
}

/*
 * Convert Data (TODO, Use DFDL schema)
 */
void position_data_decode (uint8_t *buff_out, size_t *len_out, uint8_t *buff_in, size_t *len_in) {
  position_output   *p1 = (position_output *)   buff_out;
  position_datatype *p2 = (position_datatype *) buff_in;

  p2->x  = unpack754_be(p1->x);
  p2->y  = unpack754_be(p1->y);
  p2->z  = unpack754_be(p1->z);
  *len_in = *len_out;
}

void distance_print (distance_datatype *distance) {
  fprintf(stderr, "distance (len=%ld): %f, %f, %f\n", sizeof(*distance),
  distance->x,
  distance->y,
  distance->z);
}

/*
 * Convert Data (TODO, Use DFDL schema)
 */
void distance_data_encode (uint8_t *buff_out, size_t *len_out, uint8_t *buff_in, size_t *len_in) {
  distance_datatype *p1 = (distance_datatype *) buff_in;
  distance_output   *p2 = (distance_output *)   buff_out;

  p2->x  = pack754_be(p1->x);
  p2->y  = pack754_be(p1->y);
  p2->z  = pack754_be(p1->z);
  *len_out = *len_in;
}

/*
 * Convert Data (TODO, Use DFDL schema)
 */
void distance_data_decode (uint8_t *buff_out, size_t *len_out, uint8_t *buff_in, size_t *len_in) {
  distance_output   *p1 = (distance_output *)   buff_out;
  distance_datatype *p2 = (distance_datatype *) buff_in;

  p2->x  = unpack754_be(p1->x);
  p2->y  = unpack754_be(p1->y);
  p2->z  = unpack754_be(p1->z);
  *len_in = *len_out;
}
