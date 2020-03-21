# Hardware Abstraction Layer
This repository hosts the open source components of the Hardware Abstraction Layer (HAL). HAL provides applications within a security enclave with a simple high-level interface to communicate with application in other enclaves. Based only on the application specified *tag*, HAL routes *cross-domain* communication to its destination via Cross Domain Guard (CDG) hardware (where provisioned security policies are enforced).

The `master` branch contains the most recent public release software while `develop` contains bleeding-edge updates and work-in-progress features for use by beta testers and early adopters.

This repository is maintained by Perspecta Labs.

## Contents


- [HAL Components](#HAL-Components)
- [Build](#build)
- [Run](#run)

## HAL Components
HAL is implemented as a single daemon running on the host. As shown in the figure below, its left interface connects to various applications, while its right interfaces connect (through the host's network interfaces) to the CDGs (residing either as a *bookend* on the same host as HAL or as a *bump-in-the-wire*).

![HAL interfaces between applications and Network Interfaces.](hal_api.png)

The HAL daemon has the following major components:
- [API](api/) to applications (*xdcomms C library*), which provide the high-level interface used by Applications to: a) send and receive Application Data Units (ADUs), and b) describe the ADU configuration.
- [Application generated Codecs](appgen/), which provide ADUs serialization and deserialization functions (based on the ADU configuration description) for the xdcomms library.
- [Data Plane Switch](daemon/), which forwards data based based on HAL configuration file mapping (**halmap**) rules.
- [Packetizer](daemon/), which converts between the internal HAL format (containing tag and ADU) and the different packet formats. Each CDG packet format has a separate sub-components that performs the encoding and decoding to and from the HAL internal format.
- [Device read and write](daemon/), which wait for packets on all the opened read devices and forward them based on the halmap forwarding table specified in the configuration file.
- [Device Manager](daemon/), which opens the devices specified in the configuration file. It also provisions the CDGs with security policies. 


Also included in the HAL directory are [test](test/) programs, which includes:
- **Application test**, which provides an example of sending and receiving different data types through HAL.
- **Halperf**, which emulates an application sending and receiving data through HAL at a specified rate. It also collects performance statistics.
- **HAL configuration files**, which a) define the supported device configurations, and b) define the halmap forwarding rules.
- **Simple network emulation**, which emulate the HAL devices and the remote HAL.


## Build

Install the HAL pre-requisite libraries.
```
sudo apt install -y libzmq3-dev
sudo apt install -y libconfig-dev
```

We use the Make file to compile HAL, together with its libraries [API](api/) and [codecs](codecs/)) and test programs:
```
cd ~/gaps/top-level/hal/
make clean; make
```
Some devices may also require installation into the kernel.

## Run

HAL must be started with a specified configuration file. At its simplest, we can start HAL in echo mode, where HAL receives packets on its application interface and routes them immediately back to the application interface. HAL can be started as a loopback device by using the loopback configuration file.
```
cd ~/gaps/top-level/hal/
daemon/hal test/sample_loopback.cfg
```

To start HAL sending to real network device:
- First, emulate the devices in one window
```
cd ~/gaps/top-level/hal/test
bash net.sh
```
- Second, start hal in a separate window
```
cd ~/gaps/top-level/hal/
daemon/hal -v test/sample.cfg
```

Note that the network emulation script configurations [net.sh](test/net.sh) must match the devices configuration specified in the HAL configuration script [sample.cfg](test/sample.cfg). 
