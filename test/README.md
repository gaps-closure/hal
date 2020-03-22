# HAL Test Programs
This directory has example application programs, scripts, and HAL conifguration files used to test the HAL daemon. 

The `master` branch contains the most recent public release software while `develop` contains bleeding-edge updates and work-in-progress features for use by beta testers and early adopters.

This repository is maintained by Perspecta Labs.

## Contents

- [Testing with HAL Loopback](#HAL-Loopback-Tests)
- [Testing with Device and Network emulation](#HAL-Single-Node-Tests)


## HAL Loopback Tests

First [run HAL in loopback mode](../README.md#hal-loopback-mode) in one terminal. In a second terminal run the test application:

```
cd ~/gaps/top-level/hal/test
source path.sh 
./app_test
```
By default, [without any conifguration option](#test-app-configuration-options), the [test application](./app_test.c) calls the [HAL API send function](../api/) with: a) some example POSITION information, and b) a from-tag <*mux, sec, typ*> set to <1,1,1>, where:
- *mux=1* identiies this test application session.
- *sec=1* identiies the security policies that must be applied.
- *typ=1* means it will send POSITION data type, with the x, y, z information represented using *doubles*.
Note that, running path.sh only needs to be done once, in order to ensure the HAL API dynamic library path is conifgured for the terminal.

The HAL [loopback conifguraion script](sample_loopback.cfg) has only a single device (*xdd0*) and a single *halmap* entry. The *halmap* entry tells HAL to route packets from the application read interface (*xdd0*) back to the application write interface (*xdd0*). For loopback data, HAL adds a 50ms delay.

After calling the send function, the test application calls the [HAL API recv function](../api/), with a tag mux value set to 1. This causes the echoed packet to be read and printed.

Below is an example of the application output from the test:
```
~/gaps/top-level/hal/test$ ./app_test 
send-tag = [1, 1, 1] recv-tag = [1, 1, 1] loop_count=1, loop_pause_us=0, receive_first=0
app tx [mux=01 sec=01 typ=01] position (len=40): -74.574489, 40.695545, 102.000000, 0, 0, 0, 0, 0
app rx [mux=01 sec=2309737967 typ=01] position (len=40): -74.574489, 40.695545, 102.000000, 0, 0, 0, 0, 0
Elapsed = 62320 us, LoopPause = 0 us, rate = 16.046213 rw/sec
~/gaps/top-level/hal/test$
```

Below is the corresponding HAL daemon output from the test:

```
~/gaps/top-level/hal$ daemon/hal test/sample_loopback.cfg 
HAL device list:
 xdd0 [v=1 d=./zc/zc m=sdh_ha_v1 c=ipc mi=sub mo=pub fr=3 fw=6]
HAL map list (0x563fc1004150):
 xdd0 [mux=01 sec=01 typ=01] ->  xdd0 [mux=01 sec=2309737967 typ=01] , codec=NULL

HAL Waiting for input on fds, 3

HAL reads sdh_ha_v1 from xdd0, fd=03: (len=56) 00000001 00000001 00000001 00000028 5ABA826D C4A452C0 BBF2599E 07594440 00000000 00805940 00000000 00000000 00000000 00000000
HAL writes sdh_ha_v1 onto xdd0, fd=06: (len=56) 00000001 89ABCDEF 00000001 00000028 5ABA826D C4A452C0 BBF2599E 07594440 00000000 00805940 00000000 00000000 00000000 00000000
```

## HAL Single Node Tests

This section describes the use of the application test program to generate and singe send and receive
on with different devices, tags and data types.  

, with HAL and a socat device
(to emulate the network).

Below is an example of the application output from the test:
````
~/gaps/top-level/hal/test$ ./app_test 6222
exp=6222: send-tag = [2, 2, 2] recv-tag = [2, 2, 2] loop_count=1, loop_pause_us=0, receive_first=0
app tx [mux=02 sec=02 typ=02] position (len=40): -1.021000, 2.334000, 0.500000, 0, 0, 0, 0, 0
app rx [mux=02 sec=02 typ=02] position (len=40): -1.021000, 2.334000, 0.500000, 0, 0, 0, 0, 0
Elapsed = 14440 us, LoopPause = 0 us, rate = 69.252078 rw/sec
````

Below is the corresponding HAL daemon output from the test:
````
~/gaps/top-level/hal$ daemon/hal test/sample.cfg 
HAL device list:
 xdd0 [v=1 d=./zc/zc m=sdh_ha_v1 c=ipc mi=sub mo=pub fr=3 fw=6]
 xdd1 [v=1 d=/dev/vcom1 m=sdh_be_v1 c=tty fr=4 fw=4]
 xdd2 [v=0 d=/dev/vcom1 m=sdh_be_v2 c=tty]
 xdd3 [v=1 d=lo m=sdh_bw_v1 c=udp ai=127.0.0.1 ao=127.0.0.1 pi=6788 po=50000 fr=7 fw=5]
 xdd4 [v=1 d=lo m=sdh_bw_v1 c=tcp ao=127.0.0.1 po=6787 fr=8 fw=8]
 xdd6 [v=1 d=/dev/gaps_ilip_0_root m=sdh_be_v1 c=ilp fr=9 fw=10 dr=/dev/gaps_ilip_1_read dw=/dev/gaps_ilip_1_write mx=1]
 xdd7 [v=1 d=/dev/gaps_ilip_0_root m=sdh_be_v1 c=ilp fr=11 fw=12 dr=/dev/gaps_ilip_2_read dw=/dev/gaps_ilip_2_write mx=2]
HAL map list (0x560f1b6d42b0):
 xdd0 [mux=01 sec=01 typ=01] ->  xdd6 [mux=01 sec=01 typ=01] , codec=NULL
 xdd7 [mux=02 sec=02 typ=01] ->  xdd0 [mux=02 sec=02 typ=01] , codec=NULL
 xdd7 [mux=02 sec=02 typ=02] ->  xdd0 [mux=02 sec=02 typ=02] , codec=NULL
 xdd0 [mux=11 sec=11 typ=01] ->  xdd3 [ctag=0x00010101]      , codec=NULL
 xdd3 [ctag=0x00020201]      ->  xdd0 [mux=12 sec=12 typ=01] , codec=NULL
 xdd3 [ctag=0x00020202]      ->  xdd0 [mux=12 sec=12 typ=02] , codec=NULL
 xdd6 [mux=01 sec=01 typ=01] ->  xdd0 [mux=01 sec=01 typ=01] , codec=NULL
 xdd0 [mux=02 sec=02 typ=01] ->  xdd7 [mux=02 sec=02 typ=01] , codec=NULL
 xdd0 [mux=02 sec=02 typ=02] ->  xdd7 [mux=02 sec=02 typ=02] , codec=NULL
 xdd3 [ctag=0x00010101]      ->  xdd0 [mux=11 sec=11 typ=01] , codec=NULL
 xdd0 [mux=12 sec=12 typ=01] ->  xdd3 [ctag=0x00020201]      , codec=NULL
 xdd0 [mux=12 sec=12 typ=02] ->  xdd3 [ctag=0x00020202]      , codec=NULL
 xdd0 [mux=01 sec=01 typ=101] ->  xdd6 [mux=01 sec=01 typ=03] , codec=NULL
 xdd7 [mux=02 sec=03 typ=03] ->  xdd0 [mux=02 sec=03 typ=101] , codec=NULL
 xdd0 [mux=05 sec=05 typ=101] ->  xdd1 [mux=05 sec=05 typ=101] , codec=NULL
 xdd0 [mux=06 sec=06 typ=102] ->  xdd4 [ctag=0x00060666]      , codec=NULL
 xdd6 [mux=01 sec=01 typ=03] ->  xdd0 [mux=01 sec=01 typ=101] , codec=NULL
 xdd0 [mux=02 sec=03 typ=101] ->  xdd7 [mux=02 sec=03 typ=03] , codec=NULL
 xdd1 [mux=05 sec=05 typ=101] ->  xdd0 [mux=05 sec=05 typ=101] , codec=NULL
 xdd4 [ctag=0x00060666]      ->  xdd0 [mux=06 sec=06 typ=102] , codec=NULL

HAL Waiting for input on fds, 3, 4, 7, 8, 9, 11

HAL reads sdh_ha_v1 from xdd0, fd=03: (len=56) 00000002 00000002 00000002 00000028 BC749318 0456F0BF 79E92631 08AC0240 00000000 0000E03F 00000000 00000000 00000000 00000000
unix time = 1584899149217887000 = 0x15feb16109f37318 nsec
HAL writes sdh_be_v1 onto xdd7, fd=12: (len=76) 00000002 00000002 00000001 00000002 00000000 00000000 15FEB161 09F37318 00000028 BC749318 0456F0BF 79E92631 08AC0240 00000000 0000E03F 00000000 00000000 00000000 00000000
HAL reads sdh_be_v1 from xdd7, fd=11: (len=256) 00000002 00000002 00000001 00000002 02070000 00000000 15FEB161 09F37318 00000028 BC749318 0456F0BF 79E92631 08AC0240 00000000 0000E03F 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
HAL writes sdh_ha_v1 onto xdd0, fd=06: (len=56) 00000002 00000002 00000002 00000028 BC749318 0456F0BF 79E92631 08AC0240 00000000 0000E03F 00000000 00000000 00000000 00000000

````
## Test APP Configuration Options
Below are the general options for the app_test script

```
~/gaps/top-level/hal/test$ ./app_test -h
Test program for sending and receiving data to and from the GAPS Hardware Abstraction Layer (HAL)
Usage: ./app_test [Options] [Experiment Number]
[Options]:
 -c : Number of send & receive loops: Default = 1
 -h : Print this message
 -p : Pause time (in microseconds) after send & receive loop: Default = 0
 -r : Receive first (for each send & receive loop) 
 -m : Send multiplexing (mux) tag (integer): Default = 1
 -s : Send security (sec)     tag (integer): Default = 1
 -t : Send data type (typ)    tag (integer): Default = 1
 -M : Recv multiplexing (mux) tag (integer): Default = 1
 -S : Recv security (sec)     tag (integer): Default = 1
 -T : Recv data type (typ)    tag (integer): Default = 1
[Experiment Number]: Optional override of send and recv tags (-m -s -t -M -S -T options)
 With HAL config file sample.cfg, the numbers map to the following send/recv network tags:
  [6xxx] are for BE devices xdd6 (mux=1) & xdd7 (mux=2) with send/recv network tags:
    6111 → <1,1,1>, 6113 → <1,1,3>, 6221 → <2,2,1>, 6222 → <2,2,2>, 6233 → <2,3,3>. 
  [3xxx] are for a BW device xdd3:
    3111 → <1,1,1>, 3221 → <2,2,1>, 3222 → <2,2,2>. 
  [xxxx] are for other devices:
    1553 → <5,5,3> on xdd1, 3221 → <6,6,4> on xdd4. 
```
