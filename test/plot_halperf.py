#!/usr/bin/env python3

# Plot HAL Perormance v0.02 (April 17, 2020)

# Example:
#
# 1) Run experiment
#   jaga:~/gaps/hal/test$ LD_LIBRARY_PATH=../appgen ./halperf.py -i ipc:///tmp/halsubbegreen -o ipc:///tmp/halpubbegreen -r 2 2 1 -t 30 --interval 10000 -Z -s 1 1 1 500
#   jaga:~/gaps/hal$ daemon/hal test/sample_6modemo_be_green.cfg
#   jaga:~/gaps/hal$ daemon/hal test/sample_6modemo_be_orange.cfg
#   jaga:~/gaps/hal/test$ LD_LIBRARY_PATH=../appgen ./halperf.py -i ipc:///tmp/halsubbeorange -o ipc:///tmp/halpubbeorange -r 1 1 1 -t 35 --interval 10000 -z results_halperf.csv -s 2 2 1 500
#   jaga:~/gaps/hal/test$ python3 plot_halperf.py
#
# 2) generates input file example (from halperf.py):
#   RX Sequence Number , TX Sequence Number, time received (secs), time sent (secs), 1400
#   1 , 1 , 1586886473.6721835 , 1586886473.6712658
#   2 , 2 , 1586886473.674395 , 1586886473.6734793
 

import argparse
import matplotlib.pyplot as plt
import os
import sys
import csv
import numpy as np

time_min=0

# Read results from input file (CSV format) into 'rows' (and 'fields') list
def read_values():
  with open(args.input_filename, 'r') as csvfile:
    csvreader = csv.reader(csvfile)  # creating a csv reader object
    fields = next(csvreader)         # extract field names from first row
    for row in csvreader: rows.append(row)
  csvfile.close()
  return(fields)


# Load results (in rows and fields) into x and y arrays
def set_xy_values():
  if args.sn_series:
    x_array = [int(row[0])                          for row in rows]   # SN
  else:
    t0 = float(rows[0][3])
    x_array = [(float(row[3]) - t0)                 for row in rows]   # time
  y_array_loss = [(int(row[0])-int(row[1]))            for row in rows]    # TX-RX SN (loss)
  y_array_late = [(float(row[2])-float(row[3]))*1000.0 for row in rows]    # Latency (msec)
  return (x_array, y_array_loss, y_array_late)


# Display or save series plot
def plot_display(mode, a, r, t):
  plt.grid(True)
  title = args.title_start + ', receives ' + str(r) + ' packets over ' + str(t) + ' secs:\n'
  title += 'green sends <1,1,1>'
#  title += 'orange sends <2,2,1>'
  if (a > 0): title += ', requesting ' + str(a) + ' pps'
  plt.title(title)
  if args.pkt_filename_prefix:
    plt.savefig(args.pkt_filename_prefix + '_' + mode + '_' + str(t) + '_' + str(a) +'.png')
  else:
    plt.show()


# histogram plot from x_array, y_array
def plot_histogram(mode, y_array, a, r, t):
  plt.clf()
  plt.hist(y_array, bins='auto', rwidth=0.85)
  plt.xlabel('latency (ms)')
  plt.xlim(left=0)
  plt.ylabel('Frequency')
  plt.ylim(bottom=0)
  plot_display(mode, a, r, t)

  
# scatter plot from x_array, y_array numbers (and field string values)
def plot_scatter(mode, x_array, y_array, a, r, t):
  plt.clf()               # allow multiple plots by clearing old one
  plt.scatter(x_array, y_array)
  # a) Configurure x axes
  m=max(x_array)
  s=max(1, int(round(m/10, 0)))
  plt.xticks(np.arange(0, m, s))
  plt.xlim(left=0)
  plt.xlim(right=max(x_array))
  if args.sn_series:
    plt.xlabel(fields[0])
  else:
    plt.xlabel(fields[3])
  # b) Configurure y axes
  if (mode=='loss'):
    plt.ylabel("Packet SN (RX-TX) differnece")
  else:
    plt.ylim(bottom=0)
    plt.ylabel("Message latency (milliseconds)")
  # c) display plot
  plot_display(mode, a, r, t)
  

# calculate latency metrics
def get_summary_stats(y_array):
  r = len(y_array)                                    # packets received
  s = int(rows[-1][1])                                # packets sent
  t = round(float(rows[-1][3]) - float(rows[0][3]))   # experiment duration
  a = int(fields[-1])                                 # requested rate
  h = round(np.quantile(y_array, .95), 3)             # 95'th percentile latency
  l = round(100*(s-r)/s, 3)                           # loss percent
  m = round(np.quantile(y_array, .50), 3)             # median packet latency
  p = round(float(rows[-1][0])/float(t), 1)           # packets per second
  v = round(np.average(y_array), 3)                   # average packet latency
  
  filename = args.csv_filename
  if filename:
    if os.path.exists(filename):  mode = 'a'   # append if already exists
    else:                         mode = 'w'   # write if new
    with open(filename, mode) as csvfile:
      csvwriter = csv.writer(csvfile)
      if (mode == 'w'): csvwriter.writerow(['Packets Tx', 'Packets Rx', 'Loss (%)', 'Duration (sec)', 'Requested packets/sec', 'Receiver Packets/sec', 'Median packet latency (ms)', '95th percentile latency (ms)'])
      csvwriter.writerow([s, r, l, t, a, p, m, h])
      csvfile.close()
  else:
    print('Delay seen by', r, 'received packets (of', s, 'sent) in', t, 'secs\n ', l,
      '% loss, pps =', p, '[reqs =', a, ']\n  Average =', v, ', Median =', m, ', 95th percentile =', h, 'ms')
  return (a, r, t)
      

if __name__=='__main__':
  rows = []
  # a) Get user parameters into args
  parser = argparse.ArgumentParser(description='Plot HAL delay against time')
  parser.add_argument('-b', '--histogram', help='Display delay histogram bar chart', action='store_true')
  parser.add_argument('-e', '--loss', help='Display packet loss (RX-TX SN)', action='store_true')
  parser.add_argument('-l', '--latency', help='Display packet latency', action='store_true')
  parser.add_argument('-c', '--csv_filename', help='Summary results appended into csv file with this name (deault = print results)', type=str, default='')
  parser.add_argument('-i', '--input_filename', help='Input filename', type=str, default='results_halperf.csv')
  parser.add_argument('-p', '--pkt_filename_prefix', help='Save packet results in .png files (deault = display selected results)', type=str, default='')
  parser.add_argument('-s', '--sn_series', help='X-axis is sequence number (default = time series)', action='store_true')
  parser.add_argument('-t', '--title_start', help='Start of title', type=str, default='HAL with BE Devices')
  args = parser.parse_args()
  
  # b) Process CSV input file, create xy arrays and plot
  fields = read_values()
#  print(rows, fields)
  x_array, y_array_loss, y_array_late = set_xy_values()
  a, r, t = get_summary_stats(y_array_late)
  if args.pkt_filename_prefix or args.latency:
    plot_scatter('latency', x_array, y_array_late, a, r, t)
  if (args.pkt_filename_prefix != '') or args.loss:
    plot_scatter('loss', x_array, y_array_loss, a, r, t)
  if args.pkt_filename_prefix or args.histogram:
    plot_histogram('histogram', y_array_late, a, r, t)
