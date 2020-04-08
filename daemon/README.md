## HAL Daemon
This README describes the Hardware Abstraction Layer (HAL) daemon that will:
- Open, configure and manage interfaces.
- Route packets between interfaces.
- Provide messaging functions, including translating between packet formats.

HAL Configuration is currently done using a configuration file,
specified when starting the HAL daemon (see the 
[Quick Start Guide](../README.md#quick-start-guide) and 
[HAL Installation and Usage](../README.md#hal-installation-and-usage)).


## Contents

- [HAL Daemon Architecture](#hal-daemon-architecture)
- [HAL Interface](#hal-interfaces)
- [HAL tag](#HAL-tag)
- [HAL Daemon Command Options](#HAL-Daemon-Command-Options)
- [HAL Configuration](#HAL-Configuration)


## HAL Daemon Architecture
The HAL Service runs as a daemon, whicn can be [started manually](../README.md#configurerun-hal-on-target-hardware) or started by a systemd script at boot time. 
The figure below shows an example of the HAL daemon supporting multiple applications and Cross Domain Guards (CDGs).

![HAL interfaces between applications and Network Interfaces.](figure_HAL_daemon.png)

The figure highlights the three major HAL daemon components:
### Data Plane Switch
The **Data Plane Switch** forwards packets (containing a tag and ADU) from one interface to another (e.g., from xdd0 to xdd1). Its forwarding in based on the arriving packet's interface name, the packet's [*tag*](#HAL-tag) value, and the HAL configuration file unidirectional mapping rules (**halmap**).  

### Device Manager
The **Device Manager** opens, configures and manages the different types of interfaces (real or emulated) based on the configuration file's device specification (**devices-spec**):
- Opening the devices specified in the configuration file, using each one's specified addressing/port and communication mode. 
- Reading and writing packets. It waits for received packets on all the opened read interfaces (using a select() function) and transmits packets back out onto the halmap-specified write interface.
  
### Message Functions
The  **Message Functions** transform and control packets exchanged between the applications and guard devices: 
- *Tag translation* between the internal HAL format and the different CDG packet formats. Each CDG packet format has a separate HAL sub-component that performs the tag encoding and decoding: e.g., [packetize_sdh_bw_v1.c](packetize_sdh_bw_v1.c) and [packetize_sdh_bw_v1.h](packetize_sdh_bw_v1.h).
- *Message mediation* is not currently supported, but may include functions such as multiplexing/demultiplexing, segmentation/reassembly and rate control.
  
  
## HAL Interfaces

In the figure above, HAL's left interface (xdd0) connects to the applications, 
while its right interfaces  (e.g., xdd1) connect (through the host's devices) to the CDGs 
(residing either as a  *bookend* (BE) on the same host as HAL or as a *bump-in-the-wire* (BW).
HAL communicates with the application or guard using its host interfaces, which include: 
- Serial devices carrying TCP/IP packets (e.g., tty0).
- Network devices carrying either UDP or TCP packets (e.g., eth0) in client and/or server mode).
- ZeroMQ (0MQ) sockets using IPC or INET (e.g., ipc:///tmp/halpub, ipc:///tmp/halsub).

HAL's interface to applications is through the [HAL-API](../api/) *xdcomms C library*,
which currently supports a 0MQ pub/sub interface.
The HAL API connects to the two (a publish and a subscribe) HAL listening 0MQ sockets.


## HAL Tag
HAL packets from the application contain only the Application Data Unit (ADU) and a 
HAL packet header. The packet header contains the HAL tag, with three orthogonal 
32-bit unsigned identifiers: *<mux, sec, typ>*, where:
- **mux** is a session multiplexing handle used to identify a unidirectional application flow.
- **sec** identifies a CDG security policy used to processing an ADU. 
- **typ** identifies the type of ADU (based on DFDL xsd definition). This tells HAL how to serialize the ADU. The CDG can also use the tag *typ* (and its associated description) in order to process (e.g., downgrade) the ADU contents.

HAL uses the tag information in the HAL packet header to know how to route data 
to the correct interface, based on its configuration file mapping (**halmap**) rules.
- When sending data from the applications (on the left side of HAL in the figure above) into the network (on the right side of HAL), HAL [Message Functions](#Message-Functions) will encode (and possibly translate) the **Application tag** into a **Network tag**.
- When receiving data from the network, HAL will decode (and possibly translate) the **Network tag** back into an **Application tag**.


## HAL Daemon Command Options
To see the HAL daemon command options, run with the -h option.  Below shows the current options:
```
~/gaps/hal$ daemon/hal -h
Hardware Abstraction Layer (HAL) for GAPS CLOSURE project (version 0.11)
Usage: hal [OPTIONS]... CONFIG-FILE
OPTIONS: are one of the following:
 -f : log file name (default = no log file)
 -h : print this message
 -l : log level: 0=TRACE, 1=DEBUG, 2=INFO, 3=WARN, 4=ERROR, 5=FATAL (default = 0)
 -q : quiet: disable logging on stderr (default = enabled)
 -w : device not ready (EAGAIN) wait time in microseconds (default = 1000us): -1 exits if not ready
CONFIG-FILE: path to HAL configuration file (e.g., test/sample.cfg)
```

## HAL Configuration
The HAL daemon configuration uses a libconfig File, which contains:
- **devices-spec**, which specifies the device configuration for each HAL interface, including:
  - Device IDs (e.g., xdd1), 
  - addresses and ports
  - communication modes 
  - device paths configuration 
- **halmap** routing rules and message functions applied to each allowed unidirectional link.
  - Routing rules use the halmap *from_* and *to_* fields for the: a) HAL Interface ID and b) packet's tag value.
  -  Message functions  determine any any packet data control and transformations (e.g., ADU codec).
  - Max packet size (HAL may perform Segment and Reassemble (SAR)), 
  - Max rate (bits/second).

The [test directory](../test/) has examples of configuration files (with a .cfg) extension.  
Note that, if there are multiple HAL daemon instances on a node (e.g., for testing), then they must be configured with different interfaces.
