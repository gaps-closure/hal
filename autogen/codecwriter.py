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

'''
typedef struct _position_datatype {
  double x;
  double y;
  double z;
  trailer_datatype trailer;
} position_datatype;

typedef struct _position_output {
  uint64_t  x;
  uint64_t  y;
  uint64_t  z;
  trailer_datatype trailer;
} position_output;
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
  
class CodecWriter:
  def make_dtyp_enum(self,tree):
    dtypid = 0
    appstr = ''
    for dtypnm in [l[0] for l in tree]:
      dtypid += 1
      appstr += '#define DATA_TYP_' + dtypnm.upper() + ' ' + str(dtypid) + '\n'
    return appstr + '\n'

  def make_scalar(self,f,inp=True):
    t = cintyp[f[0]] if inp else coutyp[f[0]]
    appstr = '  ' + t + ' ' + f[1] + ';' + '\n'
    return appstr

  def make_array(self,f,inp=True):
    t = cintyp[f[0]] if inp else coutyp[f[0]]
    appstr = '  ' + t + ' ' + f[1] + '[' + str(f[2]) + '];' + '\n'
    return appstr

  def make_input_struct(self,dtypnm,flds):
    appstr = ''
    d = dtypnm.lower()
    appstr += 'typedef struct _' + d + '_input {' + '\n'
    for f in flds:
      if   len(f) == 2: appstr += self.make_scalar(f,True)
      elif len(f) == 3: appstr += self.make_array(f,True)
      else:             raise Exception('Unhandled field: ' + f)
    appstr += '  trailer_datatype trailer;' + '\n'
    appstr += '} ' + d + '_input;' + '\n'
    return appstr + '\n'

  def make_output_struct(self,dtypnm,flds):
    appstr  = ''
    d = dtypnm.lower()
    appstr += 'typedef struct _' + d + '_output {' + '\n'
    for f in flds:
      if   len(f) == 2: appstr += self.make_scalar(f,False)
      elif len(f) == 3: appstr += self.make_array(f,False)
      else:             raise Exception('Unhandled field: ' + f)
    appstr += '  trailer_datatype trailer;' + '\n'
    appstr += '} ' + d + '_output;' + '\n'
    return appstr + '\n'

  def make_dtyp_struct(self,tree):
    appstr = ''
    for l in tree:
      dtypnm = l[0]
      appstr +=  self.make_input_struct(dtypnm, l[1:])
      appstr +=  self.make_output_struct(dtypnm, l[1:])
    return appstr

  def make_fn_sigs(self,tree):
    dtypid = 0
    appstr = ''
    for dtypnm in [l[0] for l in tree]:
      dtypid += 1
      d = dtypnm.lower()
      appstr += 'extern void ' + d + ' _print (' + d + '_datatype *' + d + ');' + '\n'
      appstr += 'extern void ' + d + '_data_encode (void *buff_out, void *buff_in, size_t *len_out);' + '\n'
      appstr += 'extern void ' + d + '_data_decode (void *buff_out, void *buff_in, size_t *len_in);' + '\n'
      appstr += '\n'
    return appstr

  def make_print_fn(self,l):
    appstr = ''
    d = l[0].lower()
    appstr += 'void ' + d + '_print (' + d + '_datatype *' + d + ') {' + '\n'
    '''
    fprintf(stderr, "position (len=%ld): %f, %f, %f, %d, %d, %d, %d, %d\n", sizeof(*position),
          position->x,
          position->y,
          position->z,
          position->trailer.seq,
          position->trailer.rqr,
          position->trailer.oid,
          position->trailer.mid,
          position->trailer.crc);
    '''
    appstr += '}' + '\n\n'
    return appstr

  def make_encode_fn(self,l):
    appstr = ''
    d = l[0].lower()
    appstr += 'void ' + d + '_data_encode (void *buff_out, void *buff_in, size_t *len_out) {' + '\n'
    appstr += '  ' + d + '_datatype *p1 = (' + d + '_datatype *) buff_in;' + '\n'
    appstr += '  ' + d + '_output   *p2 = (' + d + '_output *)   buff_out;' + '\n'
    '''
    p2->x  = pack754_be(p1->x);
    p2->y  = pack754_be(p1->y);
    p2->z  = pack754_be(p1->z);
    '''
    appstr += '  p2->trailer.seq = htonl(p1->trailer.seq);' + '\n'
    appstr += '  p2->trailer.rqr = htonl(p1->trailer.rqr);' + '\n'
    appstr += '  p2->trailer.oid = htonl(p1->trailer.oid);' + '\n'
    appstr += '  p2->trailer.mid = htons(p1->trailer.mid);' + '\n'
    appstr += '  p2->trailer.crc = htons(p1->trailer.crc);' + '\n'
    appstr += '  *len_out = sizeof(p1->x) * 3 + sizeof(trailer_datatype);' + '\n'
    appstr += '}' + '\n\n'
    return appstr

  def make_decode_fn(self,l):
    appstr = ''
    d = l[0].lower()
    appstr += 'void ' + d + '_data_decode (void *buff_out, void *buff_in, size_t *len_in) {' + '\n'
    appstr += '  ' + d + '_output   *p1 = (' + d + '_output *)   buff_in;' + '\n'
    appstr += '  ' + d + '_datatype *p2 = (' + d + '_datatype *) buff_out;' + '\n'
    '''
    p2->x  = unpack754_be(p1->x);
    p2->y  = unpack754_be(p1->y);
    p2->z  = unpack754_be(p1->z);
    '''
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
      print("Error in export: ", e)

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
      print("Error in export: ", e)

if __name__ == '__main__':
  pt = [['Position', ['double', 'x'], ['double', 'y'], ['double', 'z']], 
        ['Distance', ['double', 'dx'], ['double', 'dy'], ['double', 'dz']],
        ['ArrayTest', ['double', 'doubloons', '3']]]

  print('Writing test codec to codec.c/.h')
  CodecWriter().writeheader('codec', pt)
  CodecWriter().writecodecc('codec', pt)

