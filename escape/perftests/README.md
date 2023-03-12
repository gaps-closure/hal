## Escape Performance Testing
This directory has software to copy blocks of data between application memory and mmapped ESCAPE shared memory, both on a single linux desktop and between two linux desktops. It compares ESCAPE performance with the performance when: a) using heap or mmapped memory on a single linux desktop, and b) the maximum memory bandwidth.

## Contents

- [TESTBED SETUP](#testbed-setup)
- [TEST PROGRAM](#test-program)


## TESTBED SETUP
The testbed uses the Intel *Extended Secure Capabilities Architecture Platform and Evaluation* System (ESCAPE Box). 
The system consists of two (Trenton single blade servers) laptops *escape-green* and *escape-orange*. 

[1] Intel, "Extended Secure Capabilities Architecture Platform and Evaluation (ESCAPE) System Bring Up Document," February 17, 2022.

Each laptop can access 16GB of shared memory on an FPGA card. 
The FPGA shared memory access is controled through a rule table within the FPGA, which can be configured to control which areas of memory that can be read and written by each laptop.

### Grub
The two laptops have Ubuntu 20.04.1 OS, with a 64-bit memory bus connected to 130 GB of local DDR4 memory with Bandwidth of 2933 MT/s. 
Each laptop therefore has a memory bandwidth of 2.933 x 8 = 23.46 GB/s.
Below shows the memory configuration file to add the 16 GB of FPGA shared memory and the resulting memory layout

```
$ cat /etc/default/grub
...
GRUB_DEFAULT=0
GRUB_TIMEOUT_STYLE=hidden
GRUB_TIMEOUT=0
GRUB_DISTRIBUTOR=`lsb_release -i -s 2> /dev/null || echo Debian`
GRUB_CMDLINE_LINUX_DEFAULT="quiet splash"
GRUB_CMDLINE_LINUX="memmap=16G\\\$130G"
```

The resulting memory map for each laptop is shown in the figure below 

![x](escape_box_linux_memory_map.png "Escape Box Memory Map")

## TEST Program
The test program runs memory throughput test for varying 
1. Memory pair types.
2. Payload lengths.
3. Copy functions.

There are currently six Memory pair types. The application data is always on the host heap (using malloc()). The applicaiton will both read and write to/from one of three memory types:
1. Host heap: using malloc() from host memory.
2. Host mmap: using mmap() from host memory.
3. ESCAPE mmap: using mmap() from FPGA memory. 

Payload length are (by default) a range of lengths up to 16 GB (except for host mmap memory, which linux limits the allowed size)

The test program uses three copy functions:
1. glibc memory copy: memcpy().
2. naive memory copy: using incrementing unsigned long C pointers (*d++ = *s++).
3. Apex memory copy: https://www.codeproject.com/Articles/1110153/Apex-memmove-the-fastest-memcpy-memmove-on-x-x-EVE


## Running the Test Program and plot script
The ESCAPE test program is in a singlefile: [memory_test.c](memory_test.c)

It links with the apex memory copy files: *apex_memmove.{c,h}*

A simgple python script to plot the results from the ESCAPE test program (results.csv) is in the file: *plot_xy.py*
It both displays the plots and saves them to files.

To run the test program and plot the results type:
```
make && sudo ./memcpy_test  
python3 plot_xy.py 
```

Below shows the list of options the memory copy test program 
```
amcauley@escape-orange:~/gaps/build/hal/escape/perftests$ ./memcpy_test -h
Memory speed test for GAPS CLOSURE project
Usage: ./escape_test [OPTIONS]... [Experiment ID List]
OPTIONS: are one of the following:
 -h : print this message
 -i : which source data is initialized
   0 = all sources (default)
   1 = only if source is application - read on different node to write
 -n : number of length tests
 -o : source data initialization offset value (before writing)
Experiment IDs (default runs all experiments):
   0 = write to host heap
   1 = read from host heap
   2 = write to host mmap
   3 = read from host mmap
   4 = write to shared escape mmap
   5 = read from shared escape mmap
 -r : number of test runs for each a) memory pair type, b) payload length and c) copy function
```

## Test Program Results
Current example results from a single ESCAPE box are shown below:

![x](fig_App_writes_to_escape-mmap.png "App writes to escape-mmap")

![x](fig_App_reads_from_escape-mmap.png "App reads from escape-mmap")

![x](fig_App_writes_to_host-heap.png "App writes to host-heap")

![x](fig_App_reads_from_host-heap.png "App reads from host-heap")

![x](fig_App_writes_to_host-mmap.png "App writes to escape-mmap")

![x](fig_App_reads_from_host-mmap.png "App reads from host-mmap")
