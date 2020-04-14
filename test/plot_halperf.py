#!/usr/bin/env python3

# Plot HAL Perormance

# Example:
#   jaga:~/gaps/hal/test$ LD_LIBRARY_PATH=../appgen ./halperf.py -i ipc:///tmp/halsubbegreen -o ipc:///tmp/halpubbegreen -r 2 2 1 -t 30 -s 1 1 1 500 --interval 10000 -Z
#   jaga:~/gaps/hal$ daemon/hal test/sample_6modemo_be_green.cfg
#   jaga:~/gaps/hal$ daemon/hal test/sample_6modemo_be_orange.cfg
#   jaga:~/gaps/hal/test$ LD_LIBRARY_PATH=../appgen ./halperf.py -i ipc:///tmp/halsubbeorange -o ipc:///tmp/halpubbeorange -r 1 1 1 -t 35 -s 2 2 1 500 --interval 10000 -z plot_sn_data.csv
#   jaga:~/gaps/hal/test$ python3 plot_halperf.py

# Input file example (from halperf.py):
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
  
  # b) Configurure x and y axes
  plt.xticks(np.arange(0, max(x_array), int(max(x_array)/10)))
  plt.xlim(left=0)
  plt.xlim(right=max(x_array))
  plt.grid(True)
  
  if args.sn_series:
    plt.xlabel(fields[0])
  else:
    plt.xlabel(fields[3])
  t = round(float(rows[-1][3]) - float(rows[0][3]))
  n = len(y_array)
  p = round(float(rows[-1][0])/float(t), 1)
  m = round(np.quantile(y_array, .50), 3)
  h = round(np.quantile(y_array, .95), 3)
  
  if args.loss:
    plt.ylabel("Packet diff (RX-TX SN)")
  else:
    plt.ylim(bottom=0)
    plt.ylabel("message latency (milliseconds)")
    print('At', p, 'pps, delay seen by', n, 'packets over', t, 'secs: Median =',
      m, 'ms, 95% quantile =', h, 'ms')
      
  # c)  Configurure overall plot

  
  plt.title('HAL with BE Loopback Devices. Performance of '
    + str(n) + ' packets over ' + str(t) + ' secs:\n' +
    'green sends <1,1,1> and orange sends <2,2,1>, each at ' +
    str(p) + ' pkts/sec')
    
    

  # d) display and save plot
  plt.savefig('zz.png')
  plt.savefig(args.output_filename)
  if args.plot:  plt.show()
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
  parser.add_argument('-i', '--input_filename', help='Input filename', type=str, default='plot_sn_data.csv')
  parser.add_argument('-o', '--output_filename', help='Output filename', type=str, default='plot_sn_data.png')
  parser.add_argument('-l', '--loss', help='Y-axis is packet loss (RX-TX SN)', action='store_true')
  parser.add_argument('-p', '--plot', help='do not display results (deault = display)', action='store_false')
  parser.add_argument('-s', '--sn_series', help='X-axis is sequence number (default = time series)', action='store_true')
  args = parser.parse_args()
  
  # b) Process CSV input file, create xy arrays and plot
  fields = read_values()
  #  print(rows, fields)
  x_array, y_array = set_xy_values()
  create_plot(x_array, y_array)
