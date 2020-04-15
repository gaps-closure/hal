#!/usr/bin/env python3

# Plot HAL Perormance

# Example:
#
# 1) Run experiment
#   jaga:~/gaps/hal/test$ LD_LIBRARY_PATH=../appgen ./halperf.py -i ipc:///tmp/halsubbegreen -o ipc:///tmp/halpubbegreen -r 2 2 1 -t 30 -s 1 1 1 500 --interval 10000 -Z
#   jaga:~/gaps/hal$ daemon/hal test/sample_6modemo_be_green.cfg
#   jaga:~/gaps/hal$ daemon/hal test/sample_6modemo_be_orange.cfg
#   jaga:~/gaps/hal/test$ LD_LIBRARY_PATH=../appgen ./halperf.py -i ipc:///tmp/halsubbeorange -o ipc:///tmp/halpubbeorange -r 1 1 1 -t 35 -s 2 2 1 500 --interval 10000 -z results_halperf.csv
#   jaga:~/gaps/hal/test$ python3 plot_halperf.py
#
# 2) generates input file example (from halperf.py):
#   RX Sequence Number , TX Sequence Number, time now, time sent
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
    return(fields)

# Create plot from x_array, y_array numbers (and field string values)
def create_plot(x_array, y_array):

  # a) Choose plot type
#  plt.plot(x_array, y_array)
#  plt.legend()
  plt.scatter(x_array, y_array)
  
  # b) Configurure x axes
  plt.xticks(np.arange(0, max(x_array), int(max(x_array)/10)))
  plt.xlim(left=0)
  plt.xlim(right=max(x_array))
  if args.sn_series:
    plt.xlabel(fields[0])
  else:
    plt.xlabel(fields[3])
  
  # c) Configurure y axes and summary results
  t = round(float(rows[-1][3]) - float(rows[0][3]))   # experiment duration
  n = len(y_array)                                    # packets sent
  p = round(float(rows[-1][0])/float(t), 1)           # packets per second
  m = round(np.quantile(y_array, .50), 3)             # median packet latency
  h = round(np.quantile(y_array, .95), 3)             # 95% quantile packet latency
  if args.loss:
    plt.ylabel("Packet diff (RX-TX SN)")
  else:
    plt.ylim(bottom=0)
    plt.ylabel("message latency (milliseconds)")
    if args.csv_filename:
      print('todo: if csv summary file does not exist, add labels.  Append results to', args.csv_filename)
      print('At', p, 'pps, delay seen by', n, 'packets over', t, 'secs: Median =',
        m, 'ms, 95% quantile =', h, 'ms')
    else:
      print('At', p, 'pps, delay seen by', n, 'packets over', t, 'secs: Median =',
      m, 'ms, 95% quantile =', h, 'ms')
      
  # d) Display or save series plot
  plt.grid(True)
  plt.title('HAL with BE Loopback Devices. Performance of '
      + str(n) + ' packets over ' + str(t) + ' secs:\n' +
      'green sends <1,1,1> and orange sends <2,2,1>, each at ' + str(p) + ' pkts/sec')
  if args.pkt_filename_prefix: plt.savefig(args.pkt_filename_prefix +
    '_' + str(t) + '_' + str(int(p)) +'.png')
  else: plt.show()
#  os.system('open ' + args.output_filename)


# Load results (in rows and fields) into x and y arrays
def set_xy_values():
  if args.sn_series:
    x_array = [int(row[0])                          for row in rows]   # SN
  else:
    t0 = float(rows[0][3])
    x_array = [(float(row[3]) - t0)                 for row in rows]   # time
  if args.loss:
    y_array = [(int(row[0])-int(row[1]))            for row in rows]   # TX-RX SN (loss)
  else:
    y_array = [(float(row[2])-float(row[3]))*1000.0 for row in rows]    # convert latency to micro-seconds
    
#  print('lengths', len(rows), len(x_array), len(y_array), len(fields), fields[2], fields[3]);
  return (x_array, y_array)


if __name__=='__main__':
  rows = []
  
  # a) Get user parameters into args
  parser = argparse.ArgumentParser(description='Plot HAL delay against time')
  parser.add_argument('-c', '--csv_filename', help='summary results appended into csv file with this name (deault = print results)', type=str, default='')
  parser.add_argument('-i', '--input_filename', help='Input filename', type=str, default='results_halperf.csv')
  parser.add_argument('-l', '--loss', help='Y-axis is packet loss (RX-TX SN)', action='store_true')
  parser.add_argument('-p', '--pkt_filename_prefix', help='Save packet results in .png file (deault = display results)', type=str, default='')
  parser.add_argument('-s', '--sn_series', help='X-axis is sequence number (default = time series)', action='store_true')
  args = parser.parse_args()
  
  # b) Process CSV input file, create xy arrays and plot
  fields = read_values()
  #  print(rows, fields)
  x_array, y_array = set_xy_values()
  create_plot(x_array, y_array)
