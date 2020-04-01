#!/usr/bin/python3
import argparse
import os
import signal
import sys
import time
from ctypes import *
from threading import Thread, Timer, Lock

# Open C shared libs to the datatypes; TODO: make spec-driven
xdc_so = None
gma_so = None

plock = Lock()

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

class RptTimer(Timer):
    def run(self):
        while not self.finished.is_set():
            self.finished.wait(self.interval)
            self.function(*self.args, **self.kwargs)
        self.finished.set()

class Stats():
    def __init__(self):
        self.wincnt = 0
        self.totcnt = 0
        self.totsec = 0

def send(m, s, t, r, interval):
    # Context/Socket setup
    makesock = xdc_so.xdc_pub_socket
    makesock.restype = c_void_p
    sock = makesock()

    #initial values
    pos = Position(-74.574489, 40.695545, 101.9, ClosureTrailer(0,0,0,0,0))
    dis = Distance(-1.021, 2.334, 0.4)
    tag = GapsTag(int(m),int(s),int(t))
    stats = Stats()
    slock = Lock()
    
    if int(t) == 1:
        adu = Position(pos.x, pos.y, pos.z, ClosureTrailer(0,0,0,0,0))
    elif int(t) == 2:
        adu = Distance(dis.x, dis.y, dis.z, ClosureTrailer(0,0,0,0,0))
    else:
        raise Exception('unsupported data typ: ' + str(t))
    
    def task(stats,slock):
        adu.z += 0.1
        xdc_so.xdc_asyn_send(c_void_p(sock), pointer(adu), tag)
        slock.acquire()
        stats.wincnt += 1
        stats.totcnt += 1
        slock.release()

    def print_stats(stats,tag,slock):
        stats.totsec += interval
        mst = "%d/%d/%d" % (tag.mux, tag.sec, tag.typ)
        plock.acquire()
        print("{:5.2f} | send | {:8s} | {:5d} {:7.2f} Hz | {:8d} {:8.2f} Hz".format(stats.totsec, mst, stats.wincnt, stats.wincnt/interval, stats.totcnt, stats.totcnt / stats.totsec))
        plock.release()
        slock.acquire()
        stats.wincnt = 0
        slock.release()
        
    rtmr = RptTimer(1.0/float(r),task,[stats, slock])
    rtmr.start()
    stmr = RptTimer(interval, print_stats, [stats,tag, slock])
    stmr.start()
        
def recv(m, s, t, interval):
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
    stats = Stats()
    rlock = Lock()

    def print_stats(stats,tag,rlock):
        stats.totsec += interval
        mst = "%d/%d/%d" % (tag.mux, tag.sec, tag.typ)
        plock.acquire()
        print("{:5.2f} | recv | {:8s} | {:5d} {:7.2f} Hz | {:8d} {:8.2f} Hz".format(stats.totsec, mst, stats.wincnt, stats.wincnt/interval, stats.totcnt, stats.totcnt / stats.totsec))
        plock.release()
        rlock.acquire()
        stats.wincnt = 0
        rlock.release()

    stmr = RptTimer(interval,print_stats,[stats,tag,rlock])
    stmr.start()

    while True:
        xdc_so.xdc_blocking_recv(c_void_p(sock), pointer(adu), pointer(tag))
        rlock.acquire()
        stats.wincnt += 1
        stats.totcnt += 1
        rlock.release()

        
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', '--send', nargs=4, action='append', metavar=('MUX', 'SEC', 'TYP', 'RATE'), help='send cross-domain flow using MUX/SEC/TYP at RATE (Hz)')
    parser.add_argument('-r', '--recv', nargs=3, action='append', metavar=('MUX', 'SEC', 'TYP'), help='recv cross-domain flow mapped to MUX/SEC/TYP')
    parser.add_argument('-l', metavar=('PATH'), help="path to mission app shared libraries (default=../appgen)", default='../appgen')
    parser.add_argument('-x', metavar=('PATH'), help="path to libxdcomms.so (default=../api)", default='../api')
    parser.add_argument('-i', metavar=('URI'), help="in URI (default=ipc:///tmp/halpub1)", default='ipc:///tmp/halpub1')
    parser.add_argument('-o', metavar=('URI'), help="out URI (default=ipc:///tmp/halsub1)", default='ipc:///tmp/halsub1')
    parser.add_argument('--interval', help="reporting interval, default=10s", default=10)
    parser.add_argument('-t', help="duration of test in seconds, if not specified, runs indefinitely", default=0)
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

    threads = []
    if args.send:
        for s in args.send:
            s.append(float(args.interval))
            t = Thread(args=s, target=send)
            threads.append(t)
            t.start()

    if args.recv:
        for r in args.recv:
            r.append(float(args.interval))
            t = Thread(args=r, target=recv)
            threads.append(t)
            t.start()

    def self_kill():
        os.kill(os.getpid(), signal.SIGKILL)

    if float(args.t) != 0:
        tm = Timer(float(args.t), self_kill)
        tm.start()


    while True:        
        try:
            time.sleep(99999)
        except KeyboardInterrupt:
            self_kill()
            
