#!/usr/bin/python3

#---------------------- Begin Serializer Boilerplate for GAPS ------------------------

HHEAD='''#ifndef GMA_HEADER_FILE
#define GMA_HEADER_FILE

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "float.h"

#define id(X) (X)

typedef struct _trailer_datatype {
  uint32_t seq;
  uint32_t rqr;
  uint32_t oid;
  uint16_t mid;
  uint16_t crc;
} trailer_datatype;

'''

HTAIL='''#endif
'''

FLOATH='''/*
 * uint64_t Conversion Macros: a) host to network (big endian), b) host to x86 (little endian)
 *   Test determines (at runtime sadly) if host is big endian).
 *   If byte-swapping, then swap each 32-bits using htonl (and swap the two 32=bit words).
 */
#define big_end_test(x) (1==htonl(1))
#define swap_uint64(x) (((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define htonll(x) (big_end_test(x) ? (x) : swap_uint64(x))
#define htoxll(x) (big_end_test(x) ? swap_uint64(x) : (x))

#define FLOAT_BIG_ENDIAN 0      /* 0 = Little endian, 1 = Big endian */

/* Exported packing functions for big endian */
extern uint64_t      pack754_64_be(long double);
extern long double unpack754_64_be(uint64_t);
'''

FLOATC='''/*
 * FLOAT.C
 *   IEEE-754 uint64_t encoding/decoding, plus conversion into/from network byte order
 *
 *  Uses encoder and decoder from: https://beej.us/guide/bgnet/examples/ieee754.c
 *    uint64_t = 1-bit Sign (1=-), 11-bit Biased Exponent (BE), 52-bit Normalised Mantisa (NM)
 *    e.g., 85.125 = 1.010101001 x 2^6 => S=0, BE=(bias=1023)+(6)=10000000101, NM=010101001000000...
 *
 *  Adds conversion of encoded (host-byte ordered) double to and from big_endian network format
 */

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <arpa/inet.h>
 
#include "float.h"

#define pack754_32(f)   (pack754((f),   32, 8))
#define pack754_64(f)   (pack754((f),   64, 11))
#define unpack754_32(i) (unpack754((i), 32, 8))
#define unpack754_64(i) (unpack754((i), 64, 11))

/* Encoding into IEEE-754 encoded double */
uint64_t pack754(long double f, unsigned bits, unsigned expbits)
{
  long double fnorm;
  int shift;
  long long sign, exp, significand;
  unsigned significandbits = bits - expbits - 1; // -1 for sign bit

  if (f == 0.0) return 0; // get this special case out of the way

  // check sign and begin normalization
  if (f < 0) { sign = 1; fnorm = -f; }
  else { sign = 0; fnorm = f; }

  // get the normalized form of f and track the exponent
  shift = 0;
  while(fnorm >= 2.0) { fnorm /= 2.0; shift++; }
  while(fnorm < 1.0) { fnorm *= 2.0; shift--; }
  fnorm = fnorm - 1.0;

  // calculate the binary form (non-float) of the significand data
  significand = fnorm * ((1LL<<significandbits) + 0.5f);

  // get the biased exponent
  exp = shift + ((1<<(expbits-1)) - 1); // shift + bias

  // return the final answer
  return (sign<<(bits-1)) | (exp<<(bits-expbits-1)) | significand;
}

/* Decoding from IEEE-754 encoded double */
long double unpack754(uint64_t i, unsigned bits, unsigned expbits)
{
  long double result;
  long long shift;
  unsigned bias;
  unsigned significandbits = bits - expbits - 1; // -1 for sign bit

  if (i == 0) return 0.0;

  // pull the significand
  result = (i&((1LL<<significandbits)-1)); // mask
  result /= (1LL<<significandbits); // convert back to float
  result += 1.0f; // add the one back on

  // deal with the exponent
  bias = (1<<(expbits-1)) - 1;
  shift = ((i>>significandbits)&((1LL<<expbits)-1)) - bias;
  while(shift > 0) { result *= 2.0; shift--; }
  while(shift < 0) { result /= 2.0; shift++; }

  // sign it
  result *= (i>>(bits-1))&1? -1.0: 1.0;

  return result;
}

/* Adds conversion (of IEEE-754 encoded double) into Network Big-Endian byte ordering */
uint64_t pack754_64_be(long double f) {
  uint64_t h = pack754_64(f);
  if (FLOAT_BIG_ENDIAN != 0)  return ((htonll(h)));
  else                        return ((htoxll(h)));
}

/* Adds conversion (of IEEE-754 encoded double) from Network Big-Endian byte ordering */
long double unpack754_64_be(uint64_t i) {
  uint64_t    h;
  
  if (FLOAT_BIG_ENDIAN != 0)  h = (htonll(i));
  else                        h = (htoxll(i));
  return (unpack754_64(h));
}

//#define FLOAT_TEST
#ifdef FLOAT_TEST
/* testing */
int main(void)
{
  float    f = 3.1415926, f2;
  double   d, d2;
  uint32_t fi;
  uint64_t di, ni;

  fi = pack754_32(f);
  f2 = unpack754_32(fi);
  
  d = 3.14159265358979323;
  d = 74.574489;
  di = pack754_64(d);
  d2 = unpack754_64(di);
  ni = pack754_64_be(d);

  printf("float before   (bytes = %lu): %.7f\n",             sizeof(f), f);
  printf("float encoded  (bytes = %lu): 0x%08" PRIx32 "\n",  sizeof(fi), fi);
  printf("float after    (bytes = %lu): %.7f\n\n",           sizeof(f2), f2);
  
  printf("double before  (bytes = %lu): %.20lf\n",           sizeof(d), d);
  printf("double encoded (bytes = %lu): 0x%016" PRIx64 "\n", sizeof(di), di);
  printf("double big-end (bytes = %lu): 0x%016" PRIx64 "\n", sizeof(ni), ni);
  printf("double decoder (bytes = %lu): %.20lf [%.20lf]\n",  sizeof(d), d2, (double) unpack754_64_be(ni));

  return 0;
}
#endif
'''
#---------------------- End Serializer Boilerplate for GAPS   ------------------------

cintyp = {
  'double': 'double',
  'ffloat': 'float',
  'int8':   'int8_t',
  'uint8':  'uint8_t',
  'int16':  'int16_t',
  'uint16': 'uint16_t',
  'int32':  'int32_t',
  'uint32': 'uint32_t',
  'int64':  'int64_t',
  'uint64': 'uint64_t'
}

coutyp = {
  'double': 'uint64_t',
  'ffloat': 'uint32_t',
  'int8':   'int8_t',
  'uint8':  'uint8_t',
  'int16':  'int16_t',
  'uint16': 'uint16_t',
  'int32':  'int32_t',
  'uint32': 'uint32_t',
  'int64':  'int64_t',
  'uint64': 'uint64_t'
}

fmtstr = {
  'double': '%lf',
  'ffloat': '%f',
  'int8':   '%hd',
  'uint8':  '%hu',
  'int16':  '%hd',
  'uint16': '%hu',
  'int32':  '%d',
  'uint32': '%u',
  'int64':  '%ld',
  'uint64': '%lu'
}

encfn = {
  'double': 'pack754_64_be',
  'ffloat': 'pack754_32_be',
  'int8':   'id',
  'uint8':  'id',
  'int16':  'htons',
  'uint16': 'htons',
  'int32':  'htonl',
  'uint32': 'htonl',
  'int64':  'htonll',
  'uint64': 'htonll'
}

decfn = {
  'double': 'unpack754_64_be',
  'ffloat': 'unpack754_32_be',
  'int8':   'id',
  'uint8':  'id',
  'int16':  'ntohs',
  'uint16': 'ntohs',
  'int32':  'ntohl',
  'uint32': 'ntohl',
  'int64':  'ntohll',
  'uint64': 'ntohll'
}
  
class CodecWriter:
  def make_scalar(self,d,f,ser,inp):
    appstr  = ''
    if ser == 'header':
      t = cintyp[f[0]] if inp else coutyp[f[0]]
      appstr += '  ' + t + ' ' + f[1] + ';' + '\n'
    elif ser == 'printer':
      appstr += '  ' + 'fprintf(stderr, " ' + fmtstr[f[0]] + ',", ' + d + '->' + f[1] + ');' + '\n'
    elif ser == 'encoder':
      appstr += '  ' + 'p2->' + f[1] + ' = ' + encfn[f[0]] + '(p1->' + f[1] + ');' + '\n'
    elif ser == 'decoder':
      appstr += '  ' + 'p2->' + f[1] + ' = ' + decfn[f[0]] + '(p1->' + f[1] + ');' + '\n'
    elif ser == 'sizeof':
      appstr += 'sizeof(' + cintyp[f[0]] + ') + '
    else:
      raise Exception('Unknown serializarion: ' + ser)
    return appstr

  def make_array(self,d,f,ser,inp):
    appstr  = ''
    if ser == 'header':
      t = cintyp[f[0]] if inp else coutyp[f[0]]
      appstr += '  ' + t + ' ' + f[1] + '[' + str(f[2]) + '];' + '\n'
    elif ser == 'printer':
      appstr += '  for (int i=0; i<' + str(f[2]) + '; i++) {' + '\n'
      appstr += '    ' + 'fprintf(stderr, " ' + fmtstr[f[0]] + ',", ' + d + '->' + f[1] + '[i]);' + '\n'
      appstr += '  }' + '\n'
    elif ser == 'encoder':
      appstr += '  for (int i=0; i<' + str(f[2]) + '; i++) {' + '\n'
      appstr += '    ' + 'p2->' + f[1] + '[i] = ' + encfn[f[0]] + '(p1->' + f[1] + '[i]);' + '\n'
      appstr += '  }' + '\n'
    elif ser == 'decoder':
      appstr += '  for (int i=0; i<' + str(f[2]) + '; i++) {' + '\n'
      appstr += '    ' + 'p2->' + f[1] + '[i] = ' + encfn[f[0]] + '(p1->' + f[1] + '[i]);' + '\n'
      appstr += '  }' + '\n'
    elif ser == 'sizeof':
      appstr += 'sizeof(' + cintyp[f[0]] + ') * ' + str(f[2]) + ' +'
    else:
      raise Exception('Unknown serializarion: ' + ser)
    return appstr

  def make_dtyp_enum(self,tree):
    dtypid = 0
    appstr = ''
    for dtypnm in [l[0] for l in tree]:
      dtypid += 1
      appstr += '#define DATA_TYP_' + dtypnm.upper() + ' ' + str(dtypid) + '\n'
    return appstr + '\n'

  def make_structs(self,dtypnm,flds,inp=True):
    appstr = ''
    d = dtypnm.lower()
    istr   = 'datatype' if inp else 'output'
    appstr += 'typedef struct _' + d + '_' + istr + ' {' + '\n'
    for f in flds:
      if   len(f) == 2: appstr += self.make_scalar(d,f,'header',inp)
      elif len(f) == 3: appstr += self.make_array(d,f,'header',inp)
      else:             raise Exception('Unhandled field: ' + f)
    appstr += '  trailer_datatype trailer;' + '\n'
    appstr += '} ' + d + '_' + istr + ';' + '\n'
    return appstr + '\n'

  def make_dtyp_struct(self,tree):
    appstr = ''
    for l in tree:
      appstr +=  self.make_structs(l[0], l[1:],True)
      appstr +=  self.make_structs(l[0], l[1:], False)
    return appstr

  def make_fn_sigs(self,tree):
    dtypid = 0
    appstr = ''
    for dtypnm in [l[0] for l in tree]:
      dtypid += 1
      d = dtypnm.lower()
      appstr += 'extern void ' + d + '_print (' + d + '_datatype *' + d + ');' + '\n'
      appstr += 'extern void ' + d + '_data_encode (void *buff_out, void *buff_in, size_t *len_out);' + '\n'
      appstr += 'extern void ' + d + '_data_decode (void *buff_out, void *buff_in, size_t *len_in);' + '\n'
      appstr += '\n'
    return appstr

  def make_print_fn(self,l):
    appstr = ''
    d = l[0].lower()
    appstr += 'void ' + d + '_print (' + d + '_datatype *' + d + ') {' + '\n'
    appstr += '  fprintf(stderr, "' + d + '(len=%ld): ", sizeof(*' + d + '));' + '\n'
    for f in l[1:]:
      if   len(f) == 2: appstr += self.make_scalar(d,f,'printer',True)
      elif len(f) == 3: appstr += self.make_array(d,f,'printer',True)
      else:             raise Exception('Unhandled field: ' + f)
    appstr += '  fprintf(stderr, " %u, %u, %u, %hu, %hu\\n",' + '\n'
    appstr += '          ' + d + '->trailer.seq,' + '\n'
    appstr += '          ' + d + '->trailer.rqr,' + '\n'
    appstr += '          ' + d + '->trailer.oid,' + '\n'
    appstr += '          ' + d + '->trailer.mid,' + '\n'
    appstr += '          ' + d + '->trailer.crc);' + '\n'
    appstr += '}' + '\n\n'
    return appstr

  def make_encode_fn(self,l):
    appstr = ''
    d = l[0].lower()
    appstr += 'void ' + d + '_data_encode (void *buff_out, void *buff_in, size_t *len_out) {' + '\n'
    appstr += '  ' + d + '_datatype *p1 = (' + d + '_datatype *) buff_in;' + '\n'
    appstr += '  ' + d + '_output   *p2 = (' + d + '_output *)   buff_out;' + '\n'
    for f in l[1:]:
      if   len(f) == 2: appstr += self.make_scalar(d,f,'encoder',True)
      elif len(f) == 3: appstr += self.make_array(d,f,'encoder',True)
      else:             raise Exception('Unhandled field: ' + f)
    appstr += '  p2->trailer.seq = htonl(p1->trailer.seq);' + '\n'
    appstr += '  p2->trailer.rqr = htonl(p1->trailer.rqr);' + '\n'
    appstr += '  p2->trailer.oid = htonl(p1->trailer.oid);' + '\n'
    appstr += '  p2->trailer.mid = htons(p1->trailer.mid);' + '\n'
    appstr += '  p2->trailer.crc = htons(p1->trailer.crc);' + '\n'
    appstr += '  *len_out = '
    for f in l[1:]:
      if   len(f) == 2: appstr += self.make_scalar(d,f,'sizeof',True)
      elif len(f) == 3: appstr += self.make_array(d,f,'sizeof',True)
    appstr += 'sizeof(trailer_datatype);' + '\n'
    appstr += '}' + '\n\n'
    return appstr

  def make_decode_fn(self,l):
    appstr = ''
    d = l[0].lower()
    appstr += 'void ' + d + '_data_decode (void *buff_out, void *buff_in, size_t *len_in) {' + '\n'
    appstr += '  ' + d + '_output   *p1 = (' + d + '_output *)   buff_in;' + '\n'
    appstr += '  ' + d + '_datatype *p2 = (' + d + '_datatype *) buff_out;' + '\n'
    for f in l[1:]:
      if   len(f) == 2: appstr += self.make_scalar(d,f,'decoder',True)
      elif len(f) == 3: appstr += self.make_array(d,f,'decoder',True)
      else:             raise Exception('Unhandled field: ' + f)
    appstr += '  p2->trailer.seq = ntohl(p1->trailer.seq);' + '\n'
    appstr += '  p2->trailer.rqr = ntohl(p1->trailer.rqr);' + '\n'
    appstr += '  p2->trailer.oid = ntohl(p1->trailer.oid);' + '\n'
    appstr += '  p2->trailer.mid = ntohs(p1->trailer.mid);' + '\n'
    appstr += '  p2->trailer.crc = ntohs(p1->trailer.crc);' + '\n'
    appstr += '}' + '\n\n'
    return appstr 

  def writeheader(self, outfname, tree):
    try:
      hstr = HHEAD
      hstr += self.make_dtyp_enum(tree)
      hstr += self.make_dtyp_struct(tree)
      hstr += self.make_fn_sigs(tree)
      hstr += HTAIL
      with open(outfname + '.h', 'w') as f:
        f.write(hstr)
    except Exception as e:
      print("Error in header export: ", e)

  def writecodecc(self, outfname, tree):
    try:
      cstr = '#include "' + outfname + '.h"' + '\n\n'
      for l in tree:
        cstr += self.make_print_fn(l)
        cstr += self.make_encode_fn(l)
        cstr += self.make_decode_fn(l)
      with open(outfname + '.c', 'w') as f:
        f.write(cstr)
    except Exception as e:
      print("Error in codecc export: ", e)

  def writextras(self):
    try:
      with open('float.c', 'w') as f:
        f.write(FLOATC)
      with open('float.h', 'w') as f:
        f.write(FLOATH)
    except Exception as e:
      print("Error writing extras: ", e)

if __name__ == '__main__':
  pt = [['Position', ['double', 'x'], ['double', 'y'], ['double', 'z']], 
        ['Distance', ['double', 'dx'], ['double', 'dy'], ['double', 'dz']],
        ['ArrayTest', ['double', 'doubloons', '3']]]

  print('Writing test codec to codec.c/.h')
  CodecWriter().writeheader('codec', pt)
  CodecWriter().writecodecc('codec', pt)
  print('Creating float.c/.h')
  CodecWriter().writextras()

