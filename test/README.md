# HAL Test Programs
This directory has example application programs and HAL conifgurations used to test the HAL daemon. 

The `master` branch contains the most recent public release software while `develop` contains bleeding-edge updates and work-in-progress features for use by beta testers and early adopters.

This repository is maintained by Perspecta Labs.

## Contents

- [HAL Loopback](#Run Application with HAL in Loopback mode)
- [Network emulation](api/)
- [Starting HAL](codecs/)
- [Simple Application](daemon/)
- [Run HAL with a Simple Network Emulator](#run-hal)
- [Run the Application](#run-application)


## Run Application with HAL in Loopback mode

[I'm a relative reference to a repository file](../blob/master/LICENSE)

Make sure to install HAL pre-requisites.
```
sudo apt install -y libzmq3-dev
sudo apt install -y libconfig-dev
```

Compile HAL, together with the closure libarary (libclosure.a), HAL utilities, and application (e.g., app_test.c)
```
cd ~/gaps/top-level/hal/
make clean; make
```
Start the HAL daemon
```
cd ~/gaps/top-level/hal/
daemon/
```

## Run HAL with a Simple Network Emulator 

, with HAL and a socat device
(to emulate the network).

```
sudo tshark -nli lo 'port 5678 or port 6789'
netstat -aut4 | grep local | grep -e 5678 -e 6789

# 1) Start the network device 
cd ~/gaps/top-level/hal/test/
/kill_my_hal.sh ; ./net.sh

# 2) Start HAL as a loopback device
./hal sample_loopback.cfg
# 2b) Start HAL sending to device
cd ~/gaps/top-level/hal/
daemon/hal -v test/sample.cfg

# 3) Start the test APP
cd ~/gaps/top-level/hal/test/
./app_test
```

## Run Applications

Runs one or more applicaitons (sending and receiving coded data) using the xdcomms API

```
# 1) Start APP sending the Network #1 (NET1)
./app_test 1
```
