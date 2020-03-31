#!/usr/bin/python3
import argparse
import time
from ctypes import *
from threading import Thread

# Open C shared libs to the datatypes; TODO: make spec-driven
xdc_so = None
gma_so = None 

DATA_TYP_POS = 1
DATA_TYP_DIS = 2

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
                  
def send(m, s, t, r):
    # Context/Socket setup
    makesock = xdc_so.xdc_pub_socket
    makesock.restype = c_void_p
    sock = makesock()

    #initial values
    pos = Position(-74.574489, 40.695545, 101.9, ClosureTrailer(0,0,0,0,0))
    dis = Distance(-1.021, 2.334, 0.4)

    while(float(r) != 0):
        tag = GapsTag(int(m),int(s),int(t))
        tailer = ClosureTrailer(0,0,0,0,0)
        if int(t) == 1:
            pos.z += 0.1
            adu = Position(pos.x, pos.y, pos.z, ClosureTrailer(0,0,0,0,0))
        elif int(t) == 2:
            dis.z += 0.1
            adu = Distance(dis.x, dis.y, dis.z, ClosureTrailer(0,0,0,0,0))
        else:
            raise Exception('unsupported data typ: ' + str(t))
        xdc_so.xdc_asyn_send(c_void_p(sock), pointer(adu), tag)
        print("%f sent: [%d/%d/%d] -- (%f,%f,%f)" % (time.time(), tag.mux, tag.sec, tag.typ, adu.x, adu.y, adu.z))
        busy_sleep(1.0/float(r))

def recv(m, s, t):
    if int(t) == DATA_TYP_POS:
        adu = Position()
    elif int(t) == DATA_TYP_DIS:
        adu = Distance()
    else:
        raise Exception('data type %d not supported' % (int(t)))
    print("Subscribed to [%s/%s/%s]" % (m,s,t))
    tag = GapsTag(int(m), int(s), int(t))
    makesock = xdc_so.xdc_sub_socket
    makesock.restype = c_void_p
    sock = makesock(tag)
    while(1):
        xdc_so.xdc_blocking_recv(c_void_p(sock), pointer(adu), pointer(tag))
        print('%f recv: [%d/%d/%d] -- (%f,%f,%f)' % (time.time(), tag.mux,tag.sec,tag.typ,adu.x,adu.y,adu.z))
        
def busy_sleep(s):
    start = time.time()
    while (time.time() <  start + s):
        pass

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', '--send', nargs=4, action='append', metavar=('MUX', 'SEC', 'TYP', 'RATE'), help='send cross-domain flow using MUX/SEC/TYP at RATE (Hz)')
    parser.add_argument('-r', '--recv', nargs=3, action='append', metavar=('MUX', 'SEC', 'TYP'), help='recv cross-domain flow mapped to MUX/SEC/TYP')
    parser.add_argument('-l', metavar=('PATH'), help="path to mission app shared libraries (default=../appgen)", default='../appgen')
    parser.add_argument('-x', metavar=('PATH'), help="path to libxdcomms.so (default=../api)", default='../api')
    parser.add_argument('-i', metavar=('URI'), help="in URI (default=ipc:///tmp/halpub1)", default='ipc:///tmp/halpub1')
    parser.add_argument('-o', metavar=('URI'), help="out URI (default=ipc:///tmp/halsub1)", default='ipc:///tmp/halsub1')
    args = parser.parse_args()

    xdc_so = CDLL(args.x + '/libxdcomms.so', use_errno=True)
    gma_so = CDLL(args.l + '/libgma.so')

    # Set the URIs for ZMQ
    xdc_so.xdc_ctx() 
    xdc_so.xdc_set_in(c_char_p((args.i).encode('utf-8')))
    xdc_so.xdc_set_out(c_char_p((args.o).encode('utf-8')))

    # Register encode/decode functions; TODO: make spec-driven
    xdc_so.xdc_register(gma_so.position_data_encode, gma_so.position_data_decode, DATA_TYP_POS)
    xdc_so.xdc_register(gma_so.distance_data_encode, gma_so.distance_data_decode, DATA_TYP_DIS)

    if args.send:
        for s in args.send:
            t = Thread(args=s, target=send)
            t.start()
    if args.recv:
        for r in args.recv:
            t = Thread(args=r, target=recv)
            t.start()
            
