# Hardware Abstraction Layer (HAL)
This repository hosts the open source components of HAL: a) the main daemon, b) its API to Applicaitons, and c) the codecs that define how application data is serialized for transmission. The `master` branch contains the most recent public release software while `develop` contains bleeding-edge updates and work-in-progress features for use by beta testers and early adopters.

This repository is maintained by Perspecta Labs.

## Contents

- [HAL](#daemon/README.md)
- [API](#api/README.md)
- [Codecs](#codecs/README.md)
- [Build](#build)
- [Run HAL with a Simple Network Emulator](#run-hal)
- [Run the Application](#run-application)


## Build

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

## Run HAL with a Simple Network Emulator 

, with HAL and a socat device
(to emulate the network).

```
sudo tshark -nli lo 'port 5678 or port 6789'
netstat -aut4 | grep local | grep -e 5678 -e 6789

cd ~/gaps/top-level/hal/
# 1) Start the network device 
./net.sh

# 2a) Start HAL as a loopback device
./hal sample_loopback.cfg
# 2b) Start HAL sending to device
./hal sample.cfg

# 3) Start the test APP
./app_test
```

## Run Applications

Runs one or more applicaitons (sending and receiving coded data) using the xdcomms API

```
# 1) Start APP sending the Network #1 (NET1)
./app_test 1
```
