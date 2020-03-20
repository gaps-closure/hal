import argparse
import time
from ctypes import *
from threading import Thread

# Open C shared libs to the datatypes; TODO: make spec-driven
xdc_so=CDLL('/home/mkaplan/gaps/build/src/hal/api/libxdcomms.so')
gma_so=CDLL('/home/mkaplan/gaps/build/src/hal/appgen/libgma.so')

class GapsTag(Structure):
    _fields_ = [("mux", c_uint),
                ("sec", c_uint),
                ("typ", c_uint)]
    
class ClosureTrailer(Structure):
    _fields_ = [('seq', c_uint),
                ('rqr', c_uint),
                ('old', c_uint),
                ('mid', c_ushort),
                ('crc', c_ushort)]
    
class Position(Structure):
    _fields_ = [("x", c_double),
                ("y", c_double),
                ("z", c_double),
                ("t", ClosureTrailer)]

class Distance(Structure):
    _fields_ = [("x", c_double),
                ("y", c_double),
                ("z", c_double),
                ("t", ClosureTrailer)]
                  
def xdflow_send(m, s, t, r):
    while(1):
        tag = GapsTag(int(m),int(s),int(t))
        print(f'm={tag.mux} s={tag.sec} t={tag.typ}')
        tailer = ClosureTrailer(0,0,0,0,0)
        pos = Position(1.0, 2.0, 3.0, ClosureTrailer(0,0,0,0,0))
        xdc_so.xdc_asyn_send(pointer(pos), pointer(tag))
        busy_sleep(1.0/int(r))
    
def busy_sleep(s):
    start = time.time()
    while (time.time() <  start + s):
        pass

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--xdflow', nargs=4, action='append', metavar=('MUX', 'SEC', 'TYP', 'RATE'), help='define cross-domain flow MUX, SEC, TYP and RATE in Hz')
    args = parser.parse_args()
    print(args)

    # Register encode/decode functions; TODO: make spec-driven
    xdc_so.xdc_register(gma_so.position_data_encode, gma_so.position_data_decode, 1)
#    xdc_so.xdc_register(gma_so.distance_data_encode, gma_so.distance_data_decode, 2)
    
    for xdf in args.xdflow:
        t = Thread(args=xdf, target=xdflow_send);
        t.start()




