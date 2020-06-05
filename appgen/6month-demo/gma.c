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
  fprintf(stderr, "position (len=%ld): %f, %f, %f, %d, %d, %d, %d, %d\n", sizeof(*position),
          position->x,
          position->y,
          position->z,
          position->trailer.seq,
          position->trailer.rqr,
          position->trailer.oid,
          position->trailer.mid,
          position->trailer.crc);
}

/*
 * Convert Data (TODO, Use DFDL schema)
 */
void position_data_encode (void *buff_out, void *buff_in, size_t *len_out) {
  position_datatype *p1 = (position_datatype *) buff_in;
  position_output   *p2 = (position_output *)   buff_out;

  p2->x  = double2net(p1->x);
  p2->y  = double2net(p1->y);
  p2->z  = double2net(p1->z);

  p2->trailer.seq = htonl(p1->trailer.seq);
  p2->trailer.rqr = htonl(p1->trailer.rqr);
  p2->trailer.oid = htonl(p1->trailer.oid);
  p2->trailer.mid = htons(p1->trailer.mid);
  p2->trailer.crc = htons(p1->trailer.crc);

  *len_out = sizeof(p1->x) * 3 + sizeof(trailer_datatype);
}

/*
 * Convert Data (TODO, Use DFDL schema)
 */
void position_data_decode (void *buff_out, void *buff_in, size_t *len_in) {
  position_output   *p1 = (position_output *)   buff_in;
  position_datatype *p2 = (position_datatype *) buff_out;

  p2->x  = net2double(p1->x);
  p2->y  = net2double(p1->y);
  p2->z  = net2double(p1->z);

  p2->trailer.seq = ntohl(p1->trailer.seq);
  p2->trailer.rqr = ntohl(p1->trailer.rqr);
  p2->trailer.oid = ntohl(p1->trailer.oid);
  p2->trailer.mid = ntohs(p1->trailer.mid);
  p2->trailer.crc = ntohs(p1->trailer.crc);
}

void distance_print (distance_datatype *distance) {
  fprintf(stderr, "distance (len=%ld): %f, %f, %f, %d, %d, %d, %d, %d\n", sizeof(*distance),
          distance->x,
          distance->y,
          distance->z,
          distance->trailer.seq,
          distance->trailer.rqr,
          distance->trailer.oid,
          distance->trailer.mid,
          distance->trailer.crc);
}

/*
 * Convert Data (TODO, Use DFDL schema)
 */
void distance_data_encode (void *buff_out, void *buff_in, size_t *len_out) {
  distance_datatype *p1 = (distance_datatype *) buff_in;
  distance_output   *p2 = (distance_output *)   buff_out;

  p2->x  = double2net(p1->x);
  p2->y  = double2net(p1->y);
  p2->z  = double2net(p1->z);

  p2->trailer.seq = htonl(p1->trailer.seq);
  p2->trailer.rqr = htonl(p1->trailer.rqr);
  p2->trailer.oid = htonl(p1->trailer.oid);
  p2->trailer.mid = htons(p1->trailer.mid);
  p2->trailer.crc = htons(p1->trailer.crc);

  *len_out = sizeof(p1->x) * 3 + sizeof(trailer_datatype);
}

/*
 * Convert Data (TODO, Use DFDL schema)
 */
void distance_data_decode (void *buff_out, void *buff_in, size_t *len_in) {
  distance_output   *p1 = (distance_output *)   buff_in;
  distance_datatype *p2 = (distance_datatype *) buff_out;

  p2->x  = net2double(p1->x);
  p2->y  = net2double(p1->y);
  p2->z  = net2double(p1->z);

  p2->trailer.seq = ntohl(p1->trailer.seq);
  p2->trailer.rqr = ntohl(p1->trailer.rqr);
  p2->trailer.oid = ntohl(p1->trailer.oid);
  p2->trailer.mid = ntohs(p1->trailer.mid);
  p2->trailer.crc = ntohs(p1->trailer.crc);
}
