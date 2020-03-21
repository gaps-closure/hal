# Hardware Abstraction Layer
This repository hosts the open source components of the Hardware Abstraction Layer (HAL). HAL provides applications within an security enclave with a simple high-level interface to communicate with application in other enclaces. Based only on the application specified *tag*, HAL routes all *cross-domain* comminication through the network via Cross Domain Guard (CDG) hardware (where provisioned security policies are enfforced).

The `master` branch contains the most recent public release software while `develop` contains bleeding-edge updates and work-in-progress features for use by beta testers and early adopters.

This repository is maintained by Perspecta Labs.

## Contents


- [HAL Components](#HAL-Componenets)
- [Build](#build)

## HAL Components
HAL is implemented as a single daemon running on the host. As shown in the figure below, its left interface connects to various applications, while its right interface connects (through the host's network interfaces) to the CDGs (residing either as a *bookend* on the same host as HAL or as a *bump-in-the-wire*).

![HAL interfaces between applications and Network Interfaces.](hal_api.png)

The HAL daemon has the following major components:
- [API](api/) to applicaitons (*xdcomms library*), which provide the high level inteface used by Applications to: a) send and receive Applicaiton Data Units (ADUs), and b) describe the ADU confuration.
-[Application generated Codecs](appgen/), which define how how the xdcomms library serializes ADUs for transmission (based on the ADU configuration description).
- [Packetization](daemon/), which converts between the inteal HAL format (containing tag and ADU) and the different packet formats. Each CDG packet format has a separate sub-components that performs the encoding and decoding to and from the HAL internal format.
- [Device Management](daemon/), which opens the devices specified in the configuration file. 
- [Device read and write](daemon/), whcih wait for packets on all the opened read devices and forward them based on the **halmap** forwarding table specified in the configuration file.

ALso included in the HAL directory are [test](test/) programs, which includes:
- **appplication test program**, which sends and receives different data types through HAL.
- **halperf**, whcih emulates an application sending and receiving data through HAL.
- **HAL confifguration files**, which a) define the supported device conigurations, and b) deine the halmap forwarding rules.
- **Simple network emulation**, which emulate the HAL devices and the remote HAL.


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
