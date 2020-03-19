/*
* XYZ Datatype API functions (hardcoded)
*   March 2020, Perspecta Labs
*/

#include "xyz.h"

void xyz_print (xyz_datatype *xyz) {
  fprintf(stderr, "XYZ (len=%ld): %f, %f, %f\n", sizeof(*xyz),
  xyz->x,
  xyz->y,
  xyz->z);
}

/*
 * Convert Data (TODO, Use DFDL schema)
 */
void xyz_data_encode (void *buff_out, void *buff_in, size_t *len_out) {
  xyz_datatype *p1 = (xyz_datatype *) buff_in;
  xyz_output   *p2 = (xyz_output *)   buff_out;
    
  p2->x  = pack754_be(p1->x);
  p2->y  = pack754_be(p1->y);
  p2->z  = pack754_be(p1->z);

  *len_out = sizeof(double) * 3;
}
  
/*
 * Convert Data (TODO, Use DFDL schema)
 */
void xyz_data_decode (void *buff_out, void *buff_in, size_t *len_in) {
  xyz_output   *p1 = (xyz_output *)   buff_in;
  xyz_datatype *p2 = (xyz_datatype *) buff_out;
    
  p2->x  = unpack754_be(p1->x);
  p2->y  = unpack754_be(p1->y);
  p2->z  = unpack754_be(p1->z);
}
