# Hardware Abstraction Layer
This repository hosts the open source components of the Hardware Abstraction Layer (HAL). HAL provides applications within a security enclave with a simple high-level interface to communicate with application in other enclaves. Based only on the application specified *tag*, HAL routes *cross-domain* communication to its destination via Cross Domain Guard (CDG) hardware (where provisioned security policies are enforced).

The `master` branch contains the most recent public release software while `develop` contains bleeding-edge updates and work-in-progress features for use by beta testers and early adopters.

This repository is maintained by Perspecta Labs.

## Contents


- [HAL Components](#HAL-Components)
- [HAL tag](#HAL-tag)
- [Build](#build)
- [Run](#run)

## HAL Components
HAL is implemented as a single daemon running on the host. As shown in the figure below, its left interface connects to various applications, while its right interfaces connect (through the host's network interfaces) to the CDGs (residing either as a *bookend* on the same host as HAL or as a *bump-in-the-wire*).

![HAL interfaces between applications and Network Interfaces.](hal_api.png)

The HAL daemon has the following major components:
- [API](api/) to applications (*xdcomms C library*), which provide the high-level interface used by Applications to: a) send and receive Application Data Units (ADUs), and b) describe the ADU configuration.
- [Application generated Codecs](appgen/), which provide ADUs serialization and deserialization functions (based on the ADU configuration description) for the xdcomms library.
- [Data Plane Switch](daemon/), which forwards data based based on HAL configuration file mapping (**halmap**) rules.
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

There are two types of tags:
- The **Application tag**, which is used by the applicaitons and contained in the application packets (on the left side of HAL).
- The **Network tag**, which is used by the CDG components in the network (and contained in the network packets (on the right side of HAL).
At the sender, HAL will map the Applicaiton tag into the Network tag using its configuration file mapping (**halmap**) rules.
At the receiver, HAL will map the Network tag back into an Applicaiton tag using its configuration file mapping (**halmap**) rules.


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
At its simplest, we can start HAL to echo send requests made back on the application interface. Loopback mode is enabled by specifying the loopback configuration file [test/sample_loopback.cfg](test/sample_loopback.cfg)

```
cd ~/gaps/top-level/hal/
daemon/hal test/sample_loopback.cfg
```
In this case, HAL receives packets on its application read interface and routes them back to its application write interface. This requires no network devices (or network access).

Below is an example of the output from the HAL daemon, showing the configuratin of the :
- Single device called *xdd0*, using a pub/sub ipc connection (using connection mode sdh_ha_v1), with file descriptors 3 and 6 for reading and writing.
- A single HAL map (*halmap*) routing entry, which forwards application data from the application *xdd0* device with a HAL tag *<mux, sec, typ> = <1,1,1>* back to the application *xdd0* device. It also translates that tag to *<mux, sec, typ> = <1,2309737967,1>*
```
~/gaps/top-level/hal$ daemon/hal test/sample_loopback.cfg 
HAL device list:
 xdd0 [v=1 d=./zc/zc m=sdh_ha_v1 c=ipc mi=sub mo=pub fr=3 fw=6]
HAL map list (0x5597a6af8150):
 xdd0 [mux=01 sec=01 typ=01] ->  xdd0 [mux=01 sec=2309737967 typ=01] , codec=NULL

HAL Waiting for input on fds, 3
```

### HAL with UDP Network Connections and Loopback Devices
HAL can also be started with different Network Connections, incuding serial and network devices.
If the network connections are UDP and all the devices specified in the configuration script are 
installed into the host, then HAL can be started on the node. An example of such a HAL 
configuration file is the [test/sample.cfg](test/sample.cfg) file in the [test directory](test/) 

Note that all (5 of) the serial devices associated with devices xdd6 and xdd7 
in [test/sample.cfg](test/sample.cfg), must be separately installed into the kernel.
If a specific device or network emulation is not available, 
then it must be disabled in the configuration script by specifying *enabled = 0*. 

To start the HAL daemon we run:
```
cd ~/gaps/top-level/hal/
daemon/hal test/sample.cfg
```

Below is an example of the output from the HAL daemon
```
amcauley@jaga:~/gaps/top-level/hal$ daemon/hal test/sample.cfg 
HAL device list:
 xdd0 [v=1 d=./zc/zc m=sdh_ha_v1 c=ipc mi=sub mo=pub fr=3 fw=6]
 xdd1 [v=0 d=/dev/vcom1 m=sdh_be_v1 c=tty]
 xdd2 [v=0 d=/dev/vcom1 m=sdh_be_v2 c=tty]
 xdd3 [v=1 d=lo m=sdh_bw_v1 c=udp ai=127.0.0.1 ao=127.0.0.1 pi=6788 po=50000 fr=5 fw=4]
 xdd6 [v=1 d=/dev/gaps_ilip_0_root m=sdh_be_v1 c=ilp fr=7 fw=8 dr=/dev/gaps_ilip_1_read dw=/dev/gaps_ilip_1_write mx=1]
 xdd7 [v=1 d=/dev/gaps_ilip_0_root m=sdh_be_v1 c=ilp fr=9 fw=10 dr=/dev/gaps_ilip_2_read dw=/dev/gaps_ilip_2_write mx=2]
HAL map list (0x55d9ab3ea0e0):
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
 xdd6 [mux=01 sec=01 typ=03] ->  xdd0 [mux=01 sec=01 typ=101] , codec=NULL
 xdd0 [mux=02 sec=03 typ=101] ->  xdd7 [mux=02 sec=03 typ=03] , codec=NULL

HAL Waiting for input on fds, 3, 5, 7, 9
```
It shows the configuratin of the following enabled (*v=1*) devices:
- Applicaiton interfface device called *xdd0* (as described above).
- Network device (*xdd3*) with mode sdh_bw_v1 with a udp connection on the localhost (*lo*) interface: reading/listening on port pi=6788 (using read file descriptor 5) and writing to port  po=50000 (on write file descriptor 4).
- Looback Serial devices (*xdd6 and xdd7*) with mode sdh_be_v1 with ilip communications. Both devices are associated with a single root device (/dev/gaps_ilip_0_root), though each has separate read (/dev/gaps_ilip_1_read and /dev/gaps_ilip_2_read) and write (/dev/gaps_ilip_1_write mx=1, /dev/gaps_ilip_2_write mx=2) interfaces.

The HAL map (*halmap*) routing entry shows how packets are sent to and from the applicaiton (*xdd0*) to the diferent network devices based on the HAL tag *<mux, sec, typ> = <1,1,1>*.

### HAL with Network Emulation

If the devices include connections using TCP or devices that are are not already running on the host, 
then the remote netork devices must be brought up beore starting the HAL daemon.

An example of a conifguration using TCP can be found in the configuration file [test/sample_all.cfg](test/sample_all.cfg). In the case, an emulation of the remote servers
[net.sh](test/net.sh) must be brought up first. 

To start the network emulation:
```
cd ~/gaps/top-level/hal/test
bash net.sh
```

After starting the network emulation, the HAL daemon can be started(in a separate window):
```
cd ~/gaps/top-level/hal/
daemon/hal test/sample_all.cfg
```

An example of the output with sample_all.cfg is shown below:
```
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
```

### HAL End-to-End
If two network ndoes are available, then HAL can be run on each node. Each node then has a separate conifguration script. 


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
