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

Run make in order to compile HAL, together with its libraries [API](api/) and [codecs](appgen/)) and test programs:
```
cd ~/gaps/top-level/hal/
make clean; make
```
Some devices also require installation into the kernel.

## Run

Starting the HAL daemon requires specifying a HAL configuration file. The [test directory](test/) has examples of configuration files (with a .cfg) extension. 

### HAL Loopback Mode
At its simplest, we can start HAL to echo send requests made by the application back to the application. Loopback mode is enabled by specifying the loopback configuration file in the [test directory](test/)

```
cd ~/gaps/top-level/hal/
daemon/hal test/sample_loopback.cfg
```
In this case HAL receives packets on its application read interface and routes them back to its application write interface. This requires no network access or no devices to be enabled.

### HAL with Network Emulation

We can test most of the functions of HAL on a single node. This requires all the enabled^ devices in the device list of the configuration script to be up and running. It also requires either devices to operate in loopback mode or emulation of remote client and server functionality. If a specific device or network emulation is not available, then it must be disabled in the configuration script by specifying *enabled = 0*. 

For the devices in configuration script [sample.cfg](test/sample.cfg), the [test directory](test/) includes a simple network emulation script [net.sh](test/net.sh) that can enable a device (e.g., /dev/vcom1) and emulate the remote network server/clients.  The loopback devices, xdd6 and xdd7 in [sample.cfg](test/sample.cfg), must be separately installed into the kernel.

To start the network emulation:

```
cd ~/gaps/top-level/hal/test
bash net.sh
```

After starting the network emulation, the HAL daemon can be started(in a separate window):
```
cd ~/gaps/top-level/hal/
daemon/hal test/sample.cfg
```

### HAL command options
To see the HAL daemon command options, run with the -h option.  Below shows the current options:
```
$~/gaps/top-level/hal$ daemon/hal -h
Hardware Abstraction Layer (HAL) for gaps CLOSURE project
Usage: hal [OPTIONS]... CONFIG-FILE
OPTIONS: are one of the following:
 -h --help : print this message
 -v --hal_verbose : print debug messages to stderr
 -w --hal_verbose : device not ready (EAGAIN) wait time (microseconds) - default 1000us (-1 exits if not ready)
CONFIG-FILE: path to HAL configuration file (e.g., test/sample.cfg)
