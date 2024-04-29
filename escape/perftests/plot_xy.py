#!/usr/bin/env python3
import argparse
import matplotlib.pyplot as plt
import sys
import csv
from pprint import pprint

# Plot memory copy perormance: v1 (March 6, 2023)
# Requires csv file input from ./memtest output. Example line:
#   E.g., host-heap (app) > host-heap, 16, memcpy1, 0.762, 2
COL_E = 0   # Experiment Description column
COL_L = 1   # Copy Length (Bytes) column
COL_C = 2   # Copy Type Name column
COL_T = 3   # Throughput (Gbps) column
COL_I = 4   # Test Interations

# Read results from input file (CSV format) into 'rows' (and 'fields') list
def read_values(input_filename):
  if args.verbose: print('Processing', input_filename)
  with open(input_filename, 'r') as csvfile:
    csvreader = csv.reader(csvfile)  # creating a csv reader object
    fields = next(csvreader)         # extract field names from first row
    rows = [row for row in csvreader if len(row)]     # non-empty rows only
  if args.verbose: print('Fields:', fields)
  csvfile.close()
  return(fields, rows)
  
# Load results (in rows and fields) into x and y arrays for given copy type
def get_xy_arrays(cpy_type):
  if args.verbose: print('c =', cpy_type)
  x_array = [  int(row[COL_L]) for row in plot_rows if cpy_type in row[COL_C]]    # Copy Length
  y_array = [float(row[COL_T]) for row in plot_rows if cpy_type in row[COL_C]]    # Throughput
  return (x_array, y_array)

def get_unique(index):
  return {row[index] for row in rows}   # Unique elements in column 'index'
  
def plot_init(mem_type):
  if args.verbose: print('m =', mem_type)
  plt.clf()               # allow multiple plots by clearing old one
  plt.grid(True)
  plt.title(mem_type + ' (runs=' + rows[1][COL_I] + ')')
  plt.xlabel(fields[COL_L])
  plt.ylabel(fields[COL_T])
  plt.xscale("log")
  plt.yscale("log")

def plot_display_and_save(mem_type):
  plt.legend()
  plt.savefig('fig_' + mem_type.replace(" ", "_" ) + '.png')
  if (args.display): plt.show()

if __name__=='__main__':
  parser = argparse.ArgumentParser(description='Plot memory copy throughput against copy size')
  parser.add_argument('-d', '--display', help='Display results', action='store_true')
  parser.add_argument('-i', '--input_filename', help='Input filename (prefix)', type=str, default='results.csv')
  parser.add_argument('-v', '--verbose', help='Print debug', action='store_true')
  args = parser.parse_args()
  
  fields, rows = read_values(args.input_filename)
  for mem_type in get_unique(COL_E):
    plot_init(mem_type)
    plot_rows = [row for row in rows if mem_type in row[COL_E]]
    if args.verbose: pprint(plot_rows)
    for cpy_type in get_unique(COL_C):
      x_array, y_array = get_xy_arrays(cpy_type)
      plt.plot(x_array, y_array, linestyle='-', marker='o', label=cpy_type)
    plt.plot(x_array, [23.46] * len(x_array), linestyle='-', label="mem B/W limit")
    plot_display_and_save(mem_type)
