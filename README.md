# Hardware Abstraction Layer
This repository hosts the open source components of the Hardware Abstraction Layer (HAL). HAL provides applications within an security enclave with a simple high-level interface to communicate with application in other enclaces. Based only on the application specified *tag*, HAL routes selected trafic through one of its network interfaces. It ensures all *cross-domain* comminication passes through the correct Cross Domain Guard (CDG) hardware, which enforces the provisioned security policies.

The `master` branch contains the most recent public release software while `develop` contains bleeding-edge updates and work-in-progress features for use by beta testers and early adopters.

This repository is maintained by Perspecta Labs.

## Contents


- [HAL Components](#HAL-Componenets)
- [Build](#build)

## HAL Components
![HAL interfaces between applications and Network Interfaces.](hal_api.png)
As shown in the Figure abover, 
a daemon that control communication between applicaitons in diffferent and network interfaces, b) HAL's API to applicaitons, c) the codecs that define how application data is serialized for transmission, and d) the conversion to amd from different GAPS packet formats. 
(residing either as a *bookend* on the same host as HAL or as a *bump-in-the-wire*)
## Build

Install the HAL pre-requisite libraries.
```
sudo apt install -y libzmq3-dev
sudo apt install -y libconfig-dev
```

Compile [HAL](daemon), together with its [API](api/) use by application, [codecs](codecs/) to enocde/decide application data and [examples](/test) of applications and conifgurations for testing.

Make files are used to compile the HAL daemon, its libraries and test applications:
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
