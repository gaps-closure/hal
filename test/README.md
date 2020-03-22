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
By default, [without any conifguration option](#test-app-configuration-options), the [test application](./app_test) calls the [HAL API send function](../api/) with: a) some example POSITION information, and b) a from-tag <*mux, sec, typ*> set to <1,1,1>, where:
- *mux=1* identiies this test application session.
- *sec=1* identiies the security policies that must be applied.
- *typ=1* means it will send POSITION data type, with the x, y, z information represented using *doubles*.
Note that, running path.sh only needs to be done once, in order to ensure the HAL API dynamic library path is conifgured for the terminal.

The HAL [loopback conifguraion script](sample_loopback.cfg) has only a single device (*xdd0*) and a single *halmap* entry. The *halmap* entry tells HAL to route packets from the application read interface (*xdd0*) back to the application write interface (*xdd0*).

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

, with HAL and a socat device
(to emulate the network).



## Test APP Configuration Options
xxx


