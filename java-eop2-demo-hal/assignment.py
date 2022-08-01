#!/usr/bin/env python3

import itertools
import pprint

def flatten(xss): return [x for xs in xss for x in xs]
def reqresp(x):   return ['REQUEST_' + x.upper(), 'RESPONSE_' + x.upper()]
def cartneq(lst): return [x for x in itertools.product(lst,lst) if x[0] != x[1]]

xdcall   = [ 'start', 'getRequest', 'send' ]
xdmesg   = flatten(map(reqresp,xdcall))
enc      = {"Purple_E": "purple", "Orange_E": "orange", "Green_E": "green"}

# Arbitrary offsets for starting values of mux,sec,typ -- can be 0,0,0
MUXBASE  = 100       
SECBASE  = 400
TYPBASE  = 700

enclaves = enc.keys()
levels   = set(enc.values())

mux      = {y: x+1+MUXBASE for x,y in enumerate(cartneq(enclaves))}
sec      = {y: x+1+SECBASE for x,y in enumerate(cartneq(levels))}
print(xdmesg)
typ      = {y: x+1+TYPBASE for x,y in enumerate(xdmesg)}

# mux([enc1,enc2]) gives mux value for enc1,enc2 for enc1!=enc2
# sec([lvl1,lvl2]) gives lvl value for lvl1,lvl2 for lvl1!=lvl2
# typ(dt)          gives typ value for dt

print("mux")
pprint.pprint(mux, sort_dicts=False)
print("sec")
pprint.pprint(sec, sort_dicts=False)
print("typ")
pprint.pprint(typ, sort_dicts=False)

