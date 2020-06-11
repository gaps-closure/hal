#include "codec.h"

int main (int argc, char **argv) {
  fancyarraytest_datatype p, q;
  fancyarraytest_output o;
  size_t lout, lin;

  p.a = -5;
  p.b = 254;
  p.c = -12000;
  p.d = 63000;
  p.e = -70000;
  p.f = 70000;
  p.g = -9876543210;
  p.h = 9876543210;
  p.i = -5.9345;
  p.j = 9876543210.012345;

  for(int i=0;i<4;i++) {p.k[i] = -5;}
  for(int i=0;i<4;i++) {p.l[i] = 254;}
  for(int i=0;i<4;i++) {p.m[i] = -12000;}
  for(int i=0;i<4;i++) {p.n[i] = 63000;}
  for(int i=0;i<4;i++) {p.o[i] = -70000;}
  for(int i=0;i<4;i++) {p.p[i] = 70000;}
  for(int i=0;i<4;i++) {p.q[i] = -9876543210;}
  for(int i=0;i<4;i++) {p.r[i] = 9876543210;}
  for(int i=0;i<4;i++) {p.s[i] = -5.9345;}
  for(int i=0;i<4;i++) {p.t[i] = 9876543210.012345;}
  p.trailer.seq = 0;
  p.trailer.rqr = 0;
  p.trailer.oid = 0;
  p.trailer.mid = 0;
  p.trailer.crc = 0;

  fancyarraytest_data_encode (&o, &p, &lout);
  lin = lout;
  fancyarraytest_data_decode (&q, &o, &lin);

  fancyarraytest_print (&p);
  fancyarraytest_print (&q);

  return 0;
}
