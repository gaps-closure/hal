#!/usr/bin/env python3

# Plot HAL Perormance v0.1 (April 22, 2020)

# one run: assuming nothing
#   python3 correlate_halperf_logs.py -a green -A green_1.log -b orange -B orange_1.log
# one run: assuming prefix green and orange:
#   python3 correlate_halperf_logs.py -A green_1.log
# one run with time receiver time skey o 1.5 ms
#   python3 correlate_halperf_logs.py -a green -A green_1.log -b orange -B orange_1.log -s 0.0015
# multiple runs: assuming prefix green and orange:
#   python3 correlate_halperf_logs.py
# multiple runs: assuming prefix green and orange, with clock skew of 1.5 ms
#   python3 correlate_halperf_logs.py -s 1.5

import argparse
import os
import sys
import csv
import glob

time_min=0

# TODO if needed
def sparse(list):
  hop = int(len(list)/args.max_points)
  print('sparse', args.max_points, len(list), hop);
  sparse = list[0::hop]
  print (sparse)
#  new = list(rows[i] for i in range(0, len(rows), hop))
#  print('xx', new)
  sys.exit()
  
  
# Read results from input file (halperf format) into 'tx' and 'rx' lists
def read_file(filename, hostname):
  with open(filename, 'r') as fp:
    for row in fp:
      t=row.split(' ', 2)
      if len(t) > 1:
        if (t[1] == 'sent_msg:'): tx.append([float(t[0]), t[-1], hostname])
        if (t[1] == 'recv_msg:'): rx.append([float(t[0]), t[-1], hostname])
  fp.close()



# Find matching contents at tx and rx sides. Example file
#   Subscribed to [2/2/1]
#   Subscribed to [2/2/2]
#   1587471194.221284 sent_msg: [1/1/1] -- (-74.574489,40.695545,102.000000)
#   1587471194.231534 sent_msg: [1/1/1] -- (-74.574489,40.695545,102.100000)
#
def correlate():
  i = 1; j = 1;
  for t in tx:
    for r in rx:
      if t[1] == r[1]:
        tag=r[1].split(' ', 1)
        results.append([j, i, r[0] + args.clock_skew, t[0], t[2], tag[0]])
        j+=1
        break     # only need first match
    i+=1


# process input file pair and put results in csv file
def process_files(fnA, fnB, fn_out):
  read_file(fnA, prefixA)
  read_file(fnB, prefixB)
  print(fnA, '+', fnB, '=', fn_out, '[ tx =', len(tx), 'rx =', len(rx), ']')
#  print ('tx=', tx, 'rx=', rx)
  correlate()
#  print (results)
  
  with open(fn_out, 'w') as result_file:
    wr = csv.writer(result_file, dialect='excel')
    wr.writerow(['RX Sequence Number', 'TX Sequence Number', 'time received (secs)', 'time sent (secs)', 0])
    wr.writerows(results)
    

# Get matching input file and a unique name for output file
def get_files(fnA):
  if os.path.exists(fnB):                          # process if matching node B results
    fileID = postfix.split('.', 1)                 # remove file type
    fn_out = args.fname_out + fileID[0] + '.csv'   # unique name for each output file
    process_files(fnA, fnB, fn_out)

# Get user parameters into args and process each result
if __name__=='__main__':
  parser = argparse.ArgumentParser(description='Plot HAL delay against time')
  parser.add_argument('-a', '--fname_pf_A', help='Node A halperf results input file prefix: default = green', type=str, default='green')
  parser.add_argument('-A', '--fname_in_A', help='Node A hostname: default = green', type=str, default='')
  parser.add_argument('-b', '--fname_pf_B', help='Node B halperf results input file (prefix): default = orange', type=str, default='orange')
  parser.add_argument('-B', '--fname_in_B', help='Node B hostname: default = orange', type=str, default='')
  parser.add_argument('-c', '--fname_out', help='csv results output file (prefix)', type=str, default='results')
  parser.add_argument('-m', '--max_points', help='Max plot points (else interpolate): default = no limit', type=int, default=0)
  parser.add_argument('-s', '--clock_skew', help='Clock skew between Node A and Node B', type=float, default=0.0);
  args = parser.parse_args()

  prefixA = args.fname_pf_A
  prefixB = args.fname_pf_B
  if args.fname_in_A: file_list1 = [args.fname_in_A]
  else:               file_list1 = glob.glob(prefixA + '*')
  for fnA in file_list1:       # for each node A file found
    tx = []; rx = []; results = []                   # initialize result table lists
    postfix = fnA.replace(prefixA, '')
    if args.fname_in_B: fnB = args.fname_in_B
    else:               fnB = prefixB + postfix
#    print('a', fnA, 'b', prefixA, 'c', postfix, 'd', fnB)
    get_files(fnA)
