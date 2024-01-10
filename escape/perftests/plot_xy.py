#!/usr/bin/env python3
import argparse
import matplotlib.pyplot as plt
import sys
import csv
import os
from pprint import pprint

# Plot memory copy performance: v2 (March 28, 2023) from csv file input
# generate csv file using memtest program. Frist line in csv has legend:
#   E.g., host-heap (app) > host-heap, 16, memcpy1, 0.762, 2
COL_E = 0   # Experiment Description column
COL_L = 1   # Copy Length (Bytes) column
COL_C = 2   # Copy Type Name column
COL_T = 3   # Throughput (GBps) column
COL_I = 4   # Number of Test Iterations
COL_W = 5   # Workers in thread pool

# Read results from input file (CSV format) into 'rows' and 'fields' lists
def read_values(filename_prefix):
  rows = []
  filename_list = [filename for filename in os.listdir() if filename.startswith(filename_prefix)]
  if args.verbose: print('processing:', filename_list)
  for input_filename in sorted(filename_list):
    with open(input_filename, 'r') as csvfile:
      csvreader = csv.reader(csvfile)  # creating a csv reader object
      fields = next(csvreader)         # extract field names from first row
      rows += [row for row in csvreader if len(row)]     # non-empty rows only
    csvfile.close()
  if args.verbose: print('Fields:', fields)
  return(fields, rows)

# Order parameter set (so z legend is easy to read)
def order_param_set(unique_set, col, reverse_flag=True):
  first_value = list(unique_set)[0].replace(' ', '')
  if (first_value.isdigit()):
    if (col == COL_L): unique_list = sorted(unique_set, key = int, reverse=True)
    else:              unique_list = sorted(unique_set, key = int)
  else:                unique_list = sorted(unique_set)
  if args.verbose: print ('order: test = ', first_value, 'list = ', unique_list, 'set = ', unique_set)
  return (unique_list)
      
# Get unique elements in column 'index'
def get_unique(index):
  return {row[index] for row in rows}

# Initialize plot
def plot_init(col_x, col_y, col_a, col_b, unique_value_col_a, unique_value_col_b):
  plt.clf()               # allow multiple plots by clearing old one
  plt.grid(True)
  plt.title(fields[col_a] + ' = ' + str(unique_value_col_a) + '\n' +
            fields[col_b] + ' = ' + str(unique_value_col_b))
  plt.xlabel(fields[col_x])
  plt.ylabel(fields[col_y])
  if (col_x == COL_L): plt.xscale("log")
  plt.yscale("log")

def figure_name(col_x, col_y, col_z, name_list):
  s = ''
  for name in name_list: s += name.replace(" ", "_" )
  return ('fig_' + str(col_x) + str(col_y) + str(col_z) + '_' + s + '.png')

# Save (and plossibly display) plot
def plot_save(col_x, col_y, col_z, file_name):
  plt.legend(title=fields[col_z])
  if (col_x != COL_L): plt.xlim(left=0)
  if (col_y == COL_T): plt.ylim(bottom=0.001)
#  plt.xlim(right=25)
  plt.savefig(file_name);
  if (args.display): plt.show()

# Generate one xyz plot based on data in rows_col_b (for a=unique_value_col_a and b=unique_value_col_b)
def plot_xyz(col_x, col_y, col_z, col_a, col_b, unique_value_col_a, unique_value_col_b, rows_col_b):
  x_array = []
  plot_init(col_x, col_y, col_a, col_b, unique_value_col_a, unique_value_col_b)
  for unique_value_col_z in order_param_set(get_unique(col_z), col_z):     # New line for each copy function type
    rows_col_z = [row for row in rows_col_b if (unique_value_col_z == row[col_z])]
    if args.verbose: print('col_z =', unique_value_col_z, '( len =', len(rows_col_z), ')')
    if (len(rows_col_z) > 0):
      y_array = [float(row[col_y]) for row in rows_col_z]
      x_array = [  int(row[col_x]) for row in rows_col_z]
      if args.verbose: print (x_array, y_array)
      plt.plot(x_array, y_array, linestyle='-', marker='o', label=unique_value_col_z)
  if (len(x_array) > 0):
    plt.plot(x_array, [23.46] * len(x_array), linestyle='-', label="mem B/W limit")
    plot_save(col_x, col_y, col_z, figure_name(col_x, col_y, col_z, [unique_value_col_a, unique_value_col_b]))

# Generate an xyz plot for each combination of unique values in columns a and b
def plot_per_a_b_combo(col_x, col_y, col_z, col_a, col_b):
  for unique_value_col_a in get_unique(col_a):  # New plot for each worker thread count
    rows_col_a = [row for row in rows if (unique_value_col_a == row[col_a])]
    if args.verbose: print('col_a', unique_value_col_a, '( len =', len(rows_col_a), 'of', len(rows), ')')
    for unique_value_col_b in get_unique(col_b):           # New plot for each memory type pair
      rows_col_b = [row for row in rows_col_a if (unique_value_col_b == row[col_b])]
      if args.verbose: print('col_b =', unique_value_col_b, '( len =', len(rows_col_b), ')')
      plot_xyz(col_x, col_y, col_z, col_a, col_b, unique_value_col_a, unique_value_col_b, rows_col_b)

if __name__=='__main__':
  parser = argparse.ArgumentParser(description='Plot memory copy throughput against copy size')
  parser.add_argument('-d', '--display',          help='Display results', action='store_true')
  parser.add_argument('-i', '--in_file_prefixes', help='Input filename prefixes', type=str, default='results_')
  parser.add_argument('-v', '--verbose',          help='Print debug', action='store_true')
  args = parser.parse_args()
  fields, rows = read_values(args.in_file_prefixes)
  plot_per_a_b_combo(COL_W, COL_T, COL_L, COL_E, COL_C)  # A) x=worker threads, y=Throughput, z=data length,
#  plot_per_a_b_combo(COL_W, COL_T, COL_C, COL_E, COL_L)  # B) x=worker threads, y=Throughput, z=copy func
#  plot_per_a_b_combo(COL_L, COL_T, COL_C, COL_E, COL_W)  # C) x=data length,    y=Throughput, z=copy func
