# HAL Test Programs
This directory has example application programs, scripts, and HAL configuration files used to test the HAL daemon. 

The `master` branch contains the most recent public release software while `develop` contains bleeding-edge updates and work-in-progress features for use by beta testers and early adopters.

This repository is maintained by Perspecta Labs.

## Contents

- [Testing with HAL Loopback](#HAL-Loopback-Tests)
- [Testing with Device and Network emulation](#HAL-Single-Node-Tests)
- [Testing between two nodes](#HAL-Running-on-a-Pair-of-Nodes)
- [Test APP conifguration options](#test-app-configuration-options)

## Test APP Configuration Options

## HAL Loopback Tests

First, [run HAL in loopback mode](../README.md#hal-loopback-mode) in one terminal; then, in a second terminal, run the test application:

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

After calling the send function, the test application calls the [HAL API recv function](../api/), with a tag mux value set to 1. This causes the echoed packet to be read and printed, then the test program exits.

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

This section describes the use of the application test program to send, then receive one message:
- Depending only on the selected tag values, the application will select what type of Application Data Unit (ADU) 
to send.  Current values supported by the HAL API include location (*typ=1*) and position (*typ=2*). 
Two other experimental data types are pnt (*typ=101*) and xyz (*typ=102*).
- HAL will route the messages to and from different real devices based on the tag *<mux, sec, typ>* 
and the *halmap* entries in the HAL configuration file.

Starting the experiment requires separate unix terminal to support network emulation, HAL and the application.

### 1) Network emulation
The [network emulation script](./net.sh) allows singe ended testing of HAL by performing several functions,
including: 
1. cleaning up processes from any previous runs.  
2. configuring any addition devices required by the enabled devices in the HAL configuration *device list* . 
3. emulating receiver server and client processes. 
4. providing the option of sending data back to the application.

To start the network emulation, run the following from a unix terminal:

```
cd ~/gaps/top-level/hal/test
bash net.sh
```

### 2) HAL daemon
After the network emulation is running, in a second terminal start the HAL daemon as described
[here](../README.md# hal-with-network-emulation).
For this experiment, we run a HAL configuration file [sample.cfg](sample.cfg), with multiple network device enabled, 
 
### 3) Application
The application can be started with many [options](#hal-command-options).
These options set the application tags used for sending and receiving data. 
The send tag's data typ decides what application data type is sent. 
For example, *typ=1* sends position information, while *typ=2* sends location information. 
Below shows an example, for an application using tag=<2,2,2> sending location information 
on device xdd6 (as specified in the *halmap* section of the [configuration file](sample.cfg) 
used to start the HAL daemon).
 
```
cd ~/gaps/top-level/hal/test
source path.sh 
./app_test 6222
```

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

For the six month demo, we have preconfigured a number of tag values into experiment number option.
Thses represent sending position and distance information using either the:
- Serial loopback devices (xdd6 and xdd7) connected to a *bookend* Cross Domain Gateway (CDG).
  - To emulate sending position information from the Green to the Orange enclave: ./app_test 6111 
  - To emulate sending position information from the Orange to the Green enclave: ./app_test 6221 (**to be finalized**)
  - To emulate sending distance information from the Orange to the Green enclave: ./app_test 6222
- Network device (xdd3) connected to a *bump-in-the-wire* CDG.
  - To emulate sending position information from the Green to the Orange enclave: ./app_test 3111 
  - To emulate sending position information from the Orange to the Green enclave: ./app_test 3221
  - To emulate sending distance information from the Orange to the Green enclave: ./app_test 3222
Other options experiment number values have also been [defined](#hal-command-options).

Other than the xdd6 and xdd7 loopback devices, the returned message must be types in the
network emulation terminal.  The value chosen should match the experiment numbers.
For example, below is an example of the application output from the test:
```
~/gaps/top-level/hal/test$ ./app_test 3111
exp=3111: send-tag = [11, 11, 1] recv-tag = [11, 11, 1] loop_count=1, loop_pause_us=0, receive_first=0
app tx [mux=11 sec=11 typ=01] position (len=40): -74.574489, 40.695545, 102.000000, 0, 0, 0, 0, 0
app rx [mux=11 sec=11 typ=01] position (len=40): -74.574489, 40.695545, 102.000000, 0, 0, 0, 0, 0
Elapsed = 6488493 us, LoopPause = 0 us, rate = 0.154119 rw/sec
```

Below is the corresponding HAL daemon output from the test:
```
~/gaps/top-level/hal$ daemon/hal test/sample.cfg 
HAL device list:
... (same devices and halmap as shown above)

HAL Waiting for input on fds, 3, 4, 7, 8, 9, 11

HAL reads sdh_ha_v1 from xdd0, fd=03: (len=56) 0000000B 0000000B 00000001 00000028 5ABA826D C4A452C0 BBF2599E 07594440 00000000 00805940 00000000 00000000 00000000 00000000
creating static uint16_t fcstab[256]: 0=0000 1=1189 2=2312 ... 255=0f78
Good FCS
HAL writes sdh_bw_v1 onto xdd3, fd=05: (len=48) 00010101 002817E6 5ABA826D C4A452C0 BBF2599E 07594440 00000000 00805940 00000000 00000000 00000000 00000000
HAL reads sdh_bw_v1 from xdd3, fd=07: (len=48) 00010101 002817E6 5ABA826D C4A452C0 BBF2599E 07594440 00000000 00805940 00000000 00000000 00000000 00000000
HAL put packet (len = 48) into internal PDU: sdh_bw_v1_print: ctag=65793 crc=17e6 Data (len=40) 5ABA826D C4A452C0 BBF2599E 07594440 00000000 00805940 00000000 00000000 00000000 00000000

pdu_from_sdh_bw_v1: ctag=65793 crc: in=17e6 recalc=17e6
HAL writes sdh_ha_v1 onto xdd0, fd=06: (len=56) 0000000B 0000000B 00000001 00000028 5ABA826D C4A452C0 BBF2599E 07594440 00000000 00805940 00000000 00000000 00000000 00000000

```

Below is the corresponding network emulation output from the test:
```
~/gaps/top-level/hal/test$ ./net.sh 
Listening on /dev/vcom1 (127.0.0.1:12345) and lo (127.0.0.1:udp:50000, 127.0.0.1:tcp:6787)

Type APP mux_to tag to send data back to HAL (or wait for more input)
0000000 00 01 01 01 00 28 17 e6 5a ba 82 6d c4 a4 52 c0
0000020 bb f2 59 9e 07 59 44 40 00 00 00 00 00 80 59 40
0000040 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
3111
Sending to lo IP=127.0.0.1 HAL listening port=6788 prot=UDP: 00 01 01 01 00 28 17 e6 5a ba 82 6d c4 a4 52 c0 bb f2 59 9e 07 59 44 40 00 00 00 00 00 80 59 40 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

Type APP mux_to tag to send data back to HAL (or wait for more input)

```

## HAL Running on a Pair of Nodes
xxx

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
