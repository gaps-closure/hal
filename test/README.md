# HAL Test Programs
This directory has example application programs and HAL conifgurations used to test the HAL daemon. 

The `master` branch contains the most recent public release software while `develop` contains bleeding-edge updates and work-in-progress features for use by beta testers and early adopters.

This repository is maintained by Perspecta Labs.

## Contents

- [HAL Loopback Testing](#HAL Loopback Testing)


## HAL Loopback Testing

First [run HAL in loopback mode](../README.md#hal-loopback-mode) in one terminal.

Second, run the test application 
```
cd ~/gaps/top-level/hal/test
source path.sh 
./test
```
Running path.sh only needs to be run once to ensure the HAL API dynamic library path is conifgured for the terminal

Below is an example of the application output from the test:
```
~/gaps/top-level/hal/test$ ./app_test 
send-tag = [1, 1, 1] recv-tag = [1, 1, 1] loop_count=1, loop_pause_us=0, receive_first=0
app tx [mux=01 sec=01 typ=01] position (len=40): -74.574489, 40.695545, 102.000000, 0, 0, 0, 0, 0
app rx [mux=01 sec=2309737967 typ=01] position (len=40): -74.574489, 40.695545, 102.000000, 0, 0, 0, 0, 0
Elapsed = 62320 us, LoopPause = 0 us, rate = 16.046213 rw/sec
~/gaps/top-level/hal/test$
```

Below is an example of the HAL daemon output from the test:
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

## Run HAL with a Simple Network Emulator 

, with HAL and a socat device
(to emulate the network).

```
