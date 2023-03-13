## Escape Performance Testing
This directory has software and example results of raw memory copy performance on the ESCAPE Box, without CLOSURE/HAL. 

## Contents

- [TESTBED SETUP](#testbed-setup)
- [TESTBED CONFIGURATION](#testbed-configuration)
- [TEST PROGRAM](#test-program)
- [RUN TEST PROGRAM AND PLOT SCRIPT](#run-test-program-and-plot-script)
- [PLOT TEST RESULTS](#plot-test-results)

## TESTBED SETUP
The testbed uses the Intel *Extended Secure Capabilities Architecture Platform and Evaluation* System (ESCAPE Box). 
The system consists of two (Trenton single blade servers) laptops *escape-green* and *escape-orange*. 

[1] Intel, "Extended Secure Capabilities Architecture Platform and Evaluation (ESCAPE) System Bring Up Document," February 17, 2022.

Each laptop can access 16GB of shared memory on an FPGA card. 
The FPGA shared memory access is controled through a rule table within the FPGA, which can be configured to control which areas of memory that can be read and written by each laptop.

## TESTBED CONFIGURATION
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

## TEST PROGRAM
The test program can run both a single ESCAPE box or between the two ESCAPE box machines (*escape-green* and *escape-orange*). It test memory throughput for varying:
1. Memory Pair Types
2. Payload Lengths.
3. Copy Functions.

There are currently six Memory pair types. The application data is always on the host heap (created using glibc malloc()). 
The applicaiton will read (or write) from (or to) one of three memory types:
1. Host heap: using malloc() from host memory.
2. Host mmap: using mmap() from host memory.
3. ESCAPE mmap: using mmap() from FPGA memory. 

Payload length are (by default) a range of lengths up to 16 GB (except for host mmap memory, which linux limits the allowed size)

The test program uses three copy functions:
1. glibc memory copy: memcpy().
2. naive memory copy: using incrementing unsigned long C pointers (*d++ = *s++).
3. Apex memory copy: https://www.codeproject.com/Articles/1110153/Apex-memmove-the-fastest-memcpy-memmove-on-x-x-EVE


It compares ESCAPE performance with the performance when: a) using heap or mmapped memory on a single linux desktop, and b) the maximum memory bandwidth.

## RUN TEST PROGRAM
The ESCAPE test program is in a singlefile: [memcpy_test.c](memcpy_test.c)
It links with the apex memory copy files: [apex_memmove.c](apex_memmove.c) [apex_memmove.h](apex_memmove.h)

To run the test program and plot the results type:
```
make && sudo ./memcpy_test  

# Run test with greater sampling
make && sudo ./memcpy_test -r 100

# Run quick tests on just Escape Memory (write and read) with lower sampling
make && sudo ./memcpy_test -r 2 4 5
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
 -n : number of length tests (default=9, maximum = 10)
 -o : source data initialization offset value (before writing)
 -r : number of test runs for each a) memory pair type, b) payload length and c) copy function
Experiment IDs (default runs all experiments):
   0 = write to host heap
   1 = read from host heap
   2 = write to host mmap
   3 = read from host mmap
   4 = write to shared escape mmap
   5 = read from shared escape mmap
```

The test results are printed on the terminal. Below shows an example for a small run:
```
amcauley@escape-orange:~/gaps/build/hal/escape/perftests$ sudo ./memcpy_test -n 3 0 1
PAGE_MASK=0x00000fff data_off=0 source_init=0 payload_len_num=3 runs=5 num_mem_pairs=2 [ 0 1 ]
App Memory uses host Heap [len=0x10000000 Bytes] at virtual address 0x7fc37325a010
--------------------------------------------------------------------------------------
    sour data [len=0x10000000 bytes]: 0x fffefdfcfbfaf9f8 f7f6f5f4f3f2f1f0 ... 1716151413121110 f0e0d0c0b0a0908 706050403020100
0) App writes to host-heap (fd=-1, vir-addr=0x7fc363259010, phy-addr=0x0, len=268.435456 MB)
      16 bytes using glibc_memcpy =   0.364 GB/s (5 runs: ave delta = 0.000000044 secs)
      16 bytes using naive_memcpy =   0.556 GB/s (5 runs: ave delta = 0.000000029 secs)
      16 bytes using  apex_memcpy =   0.136 GB/s (5 runs: ave delta = 0.000000118 secs)
     256 bytes using glibc_memcpy =   3.488 GB/s (5 runs: ave delta = 0.000000073 secs)
     256 bytes using naive_memcpy =   5.470 GB/s (5 runs: ave delta = 0.000000047 secs)
     256 bytes using  apex_memcpy =   2.500 GB/s (5 runs: ave delta = 0.000000102 secs)
    1024 bytes using glibc_memcpy =  15.059 GB/s (5 runs: ave delta = 0.000000068 secs)
    1024 bytes using naive_memcpy =  14.105 GB/s (5 runs: ave delta = 0.000000073 secs)
    1024 bytes using  apex_memcpy =  18.963 GB/s (5 runs: ave delta = 0.000000054 secs)
    dest data [len=0x400 bytes]: 0x fffefdfcfbfaf9f8 f7f6f5f4f3f2f1f0 ... 1716151413121110 f0e0d0c0b0a0908 706050403020100
--------------------------------------------------------------------------------------
    sour data [len=0x10000000 bytes]: 0x fffefdfcfbfaf9f8 f7f6f5f4f3f2f1f0 ... 1716151413121110 f0e0d0c0b0a0908 706050403020100
1) App reads from host-heap (fd=-1, vir-addr=0x7fc363259010, phy-addr=0x0, len=268.435456 MB)
      16 bytes using glibc_memcpy =   1.013 GB/s (5 runs: ave delta = 0.000000016 secs)
      16 bytes using naive_memcpy =   0.305 GB/s (5 runs: ave delta = 0.000000052 secs)
      16 bytes using  apex_memcpy =   0.415 GB/s (5 runs: ave delta = 0.000000039 secs)
     256 bytes using glibc_memcpy =   7.232 GB/s (5 runs: ave delta = 0.000000035 secs)
     256 bytes using naive_memcpy =   8.205 GB/s (5 runs: ave delta = 0.000000031 secs)
     256 bytes using  apex_memcpy =   3.325 GB/s (5 runs: ave delta = 0.000000077 secs)
    1024 bytes using glibc_memcpy =  12.518 GB/s (5 runs: ave delta = 0.000000082 secs)
    1024 bytes using naive_memcpy =  14.670 GB/s (5 runs: ave delta = 0.000000070 secs)
    1024 bytes using  apex_memcpy =  27.676 GB/s (5 runs: ave delta = 0.000000037 secs)
    dest data [len=0x400 bytes]: 0x fffefdfcfbfaf9f8 f7f6f5f4f3f2f1f0 ... 1716151413121110 f0e0d0c0b0a0908 706050403020100
```

The results will also be store in a csv-formatted file: [results.csv](results.csv)


## PLOT TEST RESULTS
A simgple python script can plot the results [results.csv](results.csv) from the ESCAPE test program.
The script [plot_xy.py](plot_xy.py) will save each greaph into a separate files: e.g., [(fig_App_writes_to_escape-mmap.png](fig_App_writes_to_escape-mmap.png].

To run the plot script simply type:
```
python3 plot_xy.py 

# To display the results one at a time
python3 plot_xy.py -d
```
Current example plot result from a single ESCAPE box are shown below:

![x](fig_App_writes_to_escape-mmap.png "App writes to escape-mmap")

![x](fig_App_reads_from_escape-mmap.png "App reads from escape-mmap")

![x](fig_App_writes_to_host-heap.png "App writes to host-heap")

![x](fig_App_reads_from_host-heap.png "App reads from host-heap")

![x](fig_App_writes_to_host-mmap.png "App writes to escape-mmap")

![x](fig_App_reads_from_host-mmap.png "App reads from host-mmap")
