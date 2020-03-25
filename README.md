# Hardware Abstraction Layer
This repository hosts the open source components of the Hardware Abstraction Layer (HAL). HAL provides applications within a security enclave with a simple high-level interface to communicate with application in other enclaves. Based only on the application specified [*tag*](#HAL-tag), HAL routes *cross-domain* communication to its destination via Cross Domain Guard (CDG) hardware (where provisioned security policies are enforced).

The `master` branch contains the most recent public release software while `develop` contains bleeding-edge updates and work-in-progress features for use by beta testers and early adopters.

This repository is maintained by Perspecta Labs.

## Contents


- [HAL Components](#hal-components)
- [HAL tag](#hal-tag)
- [Build](#build)
- [Run](#run)

## HAL Components
HAL runs as a single daemon on the host, supporting multiple applications and GAPS devices, which we refer to as network interfaces in this document. Some GAPS devices HAL supports are, in fact, serial/character devices, even though we refer to them as network interfaces here. In the figure below, HAL's left interface connects to the applications, while its right interfaces connect (through the host's network interfaces) to the CDGs (residing either as a *bookend* (BE) on the same host as HAL or as a *bump-in-the-wire* (BW).

![HAL interfaces between applications and Network Interfaces.](hal_api.png)

The HAL daemon has the following major components:
- [API](api/) to applications (*xdcomms C library*), which provide the high-level interface used by Applications to: a) send and receive Application Data Units (ADUs), and b) describe the ADU configuration. Using the ADU configuration description, the API uses the Application generated [Codecs](appgen/) to serialize (or de-serialize) the ADU before sending the packet to (or after receiving a packet from) HAL.
- [Data Plane Switch](daemon/), which forwards data to the correct interface based based on the arriving packet's [**tag**](#HAL-tag) and the HAL configuration file mapping (**halmap**) rules.
- [Packetizer](daemon/), which converts between the internal HAL format (containing [tag](#HAL-tag) and ADU) and the different packet formats. Each CDG packet format has a separate sub-components that performs the encoding and decoding to and from the HAL internal format.
- [Device read and write](daemon/), which wait for packets on all the opened read devices and forward them based on the halmap forwarding table specified in the configuration file.
- [Device Manager](daemon/), which opens the devices specified in the configuration file. It also provisions the CDGs with security policies. 


Also included in the HAL directory are [test](test/) programs, which includes:
- **Application test**, which provides an example of sending and receiving different data types through HAL.
- **Halperf**, which emulates an application sending and receiving data through HAL at a specified rate. It also collects performance statistics.
- **HAL configuration files**, which a) define the supported device configurations, and b) define the halmap forwarding rules.
- **Simple network emulation**, which emulate the HAL devices and the remote HAL.

## HAL Tag
HAL communication contains only the Application Data Unit (ADU) and a small HAL tag.
The tag has three orthogonal identifiers: *<mux, sec, typ>*, where:
- **mux** is a session multiplexing handle used to identify a unidirectional applicaiton flow.
- **sec** identifies a CDG security policy used to processing an ADU. 
- **typ** identifies the type of ADU (based on DFDL xsd definition), which tells HAL how to serialize the ADU. The CDG can also use the tag *typ* (and its associated description) in order to process (e.g., downgrade) the ADU contents.

There are tags on the application side and tags on the network (GAPS device) side:
- The **Application tag**, which is used by the applicaitons and contained in the application packets (on the left side of HAL).
- The **Network tag**, which is used by the CDG components in the network, and contained in the network packets (on the right side of HAL).

HAL uses the tag to know how to route data to the correct interface using its configuration file mapping (**halmap**) rules. Also, the:
- Sending HAL will map the Applicaiton tag into the Network tag using its *halmap* rules.
- Receiving HAL will map the Network tag back into an Applicaiton tag using its *halmap* rules.


## Build

We have built and tested HAL on a Linux Ubuntu 19.10 system, and while HAL can run on other operating systems / versions, the package isntallation instructions are for that particualr OS and version.

Install the HAL pre-requisite libraries.
```
sudo apt install -y libzmq3-dev
sudo apt install -y libconfig-dev
```
See the [CLOSURE Dev Server Setup](https://github.com/gaps-closure/build/blob/master/environment_setup.md) for full listing of CLOSURE external dependencies (some of which may be required for HAL on a clean system).

Run make in order to compile HAL, together with its libraries [API](api/) and [codecs](appgen/)) and test programs:
```
git clone https://github.com/gaps-closure/hal
cd hal
make clean; make
```
Some SDH devices also require installation of a device driver via an associated kernel module.

## Run

Starting the HAL daemon requires specifying a HAL configuration file. The [test directory](test/) has examples of configuration files (with a .cfg) extension. 

### HAL Loopback Mode
At its simplest, we can start HAL to echo send requests made back on the application interface. Loopback mode is enabled by specifying the loopback configuration file [test/sample_loopback.cfg](test/sample_loopback.cfg)

```
cd hal
hal$ daemon/hal test/sample_loopback.cfg
```
In this case, HAL receives packets on its application read interface and routes them back to its application write interface. This requires no network devices (or network access).

Below is an example of the output from the HAL daemon, showing the configuratin of the :
- Single device called *xdd0*, using a pub/sub ipc connection (using connection mode sdh_ha_v1), with file descriptors 3 and 6 for reading and writing.
- A single HAL map (*halmap*) routing entry, which forwards application data from the application *xdd0* device with a HAL tag *<mux, sec, typ> = <1,1,1>* back to the application *xdd0* device. It also translates that tag to *<mux, sec, typ> = <1,2309737967,1>*
```
hal$ daemon/hal test/sample_loopback.cfg 
HAL device list:
 xdd0 [v=1 d=./zc/zc m=sdh_ha_v1 c=ipc mi=sub mo=pub fr=3 fw=6]
HAL map list (0x5597a6af8150):
 xdd0 [mux=01 sec=01 typ=01] ->  xdd0 [mux=01 sec=2309737967 typ=01] , codec=NULL

HAL Waiting for input on fds, 3
```

### HAL command options
To see the HAL daemon command options, run with the -h option.  Below shows the current options:
```
hal$ daemon/hal -h
Hardware Abstraction Layer (HAL) for gaps CLOSURE project
Usage: hal [OPTIONS]... CONFIG-FILE
OPTIONS: are one of the following:
 -h --help : print this message
 -v --hal_verbose : print debug messages to stderr
 -w --hal_verbose : device not ready (EAGAIN) wait time (microseconds) - default 1000us (-1 exits if not ready)
CONFIG-FILE: path to HAL configuration file (e.g., test/sample.cfg)
