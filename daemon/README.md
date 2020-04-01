## HAL Daemon
This daemon directory contains the Hardware Abstraction Layer (HAL) Service.
The HAL Service runs as a daemon 
(typically will be started by a systemd script at boot time).  
Based on its conifguration file HAL:
. Opens, configures and manages multiple types of interfaces (real or emualted).
. Routes packets between any pair or interfaces (based on the configured route map).
. Translates the HAL packet headers (based on the interface packet model).

## HAL Architecture
HAL runs as one or more daemon on the host. Each daemon supports multiple applications and GAPS devices, which we refer to as network interfaces in this document. If there are multiple HAL instances, they must use diferent devices.
The devices can be:
 Serial devices (e.g., tty0) carrying TCP/IP packets.
. Network devices (e.g., eth0) carrying either UDP or TCP packets (in client and/or server mode).
. ZeroMQ pub/sub with the applications.

In the figure below, HAL's left interface connects to the applications, while its right interfaces connect (through the host's devices) to the CDGs (residing either as a *bookend* (BE) on the same host as HAL or as a *bump-in-the-wire* (BW).

![HAL interfaces between applications and Network Interfaces.](../hal_api.png)

The HAL daemon has the following major components:
- **Data Plane Switch**, which forwards data to the correct interface based based on the arriving packet's tag and the HAL configuration file mapping (**halmap**) rules.
- **Packetizer**, which converts between the internal HAL format (containing tag and ADU) and the different packet formats. Each CDG packet format has a separate sub-components that performs the encoding and decoding to and from the HAL internal format.
- **Device read and write**, which wait for packets on all the opened read devices and forward them based on the halmap forwarding table specified in the configuration file.
- **Device Manager**, which opens the devices specified in the configuration file. It also provisions the CDGs with security policies. 

HAL's interface to applications is through the [HAL-API](../api/). This *xdcomms C library* provides the high-level interface used by Applications to: a) send and receive Application Data Units (ADUs), and b) describe the ADU configuration. Using the ADU configuration description, the API uses the Application generated [Codecs](../appgen/) to serialize (or de-serialize) the ADU before sending the packet to (or after receiving a packet from) HAL.


#### HAL Command Options
To see the HAL daemon command options, run with the -h option.  Below shows the current options:
```
amcauley@jaga:~/gaps/hal$ daemon/hal -h
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


Packet formats can be spefcified per device instance (in the conifg file). 
The packet format and translation programs are contained in separate files
(packetize_XXX.c and packetize_XXX.h).

Planned HAL extensions include:
. Configuring the cross domain guards.
. Performing any ADU encoding/decoding.
. Mediating  exchange between the application and the guard devices, handling the multiplexing/demultiplexing, segmentation/reassembly and rate control, as applicable.



