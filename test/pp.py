#!/usr/bin/python3
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('-f', metavar='filename')

args = parser.parse_args()
first = {}
last  = {}
cnt = {}

f = open(args.f, 'r')
line = f.readline()
while line:
    if len(line.split(' ')) > 2 and line.split(' ')[1] == 'recv:':
        t=line.split(' ')[0]
        mst = line.split(' ')[2]
        if mst not in first:
            first[mst] = t
            last[mst] = t
            cnt[mst] = 1
        else:
            last[mst] = t
            cnt[mst] += 1
    line = f.readline()

for k in first:
    rate = float(cnt[k]) / (float(last[k]) - float(first[k])) 
    print('%s recv at %f Hz' % (k, rate))
    
