#!/usr/bin/python3

#---------------------- Begin Serializer Boilerplate for GAPS ------------------------

#---------------------- End Serializer Boilerplate for GAPS   ------------------------

class CodecWriter:
  # XXX: need to fix this
  def parsetree2dfdl(self, tree):
    return ''

  def write(self, outfname, tree):
    try:
      codecstr = ''.join([])
      codecstr += self.parsetree2dfdl(tree)
      with open(outfname, 'w') as f:
        f.write(codecstr)
    except Exception as e:
      print("Error in export: ", e)

if __name__ == '__main__':
  print('Writing test DFDL to betest.dfdl.xsd')
  CodecWriter().write('codectest.c', None)
