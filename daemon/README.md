## HAL Daemon
The daemon directory contains the Hardware Abstraction Layer (HAL) Service components.
The HAL Service runs as a daemon 
(typically started by a systemd script at boot time).  

Based on its conifguration file, the HAL daemon will:
- Open, configure and manages multiple types of interfaces.
- Routes packets between interfaces, based on the configured *halmap*.
- Translates the HAL tags (in packet headers), based on the configured interface packet model.

## HAL Architecture
HAL runs as one or more daemon on a host machine. Each daemon supports multiple applications and GAPS devices, through its interfaces. If there are multiple HAL instances on a node, each must use a unique set of interaces.
the HAL coniguratin file speciies the high-level interaces (e.g., xdd0 and xdd1), which can include one or more:
. Serial devices carrying TCP/IP packets (e.g., tty0).
. Network devices carrying either UDP or TCP packets (e.g., eth0) in client and/or server mode).
. ZeroMQ using IPC or INET (e.g., ipc:///tmp/halpub, ipc:///tmp/halsub).

In the figure below, HAL's left interface connects to the applications, while its right interfaces connect (through the host's devices) to the CDGs (residing either as a *bookend* (BE) on the same host as HAL or as a *bump-in-the-wire* (BW).

![HAL interfaces between applications and Network Interfaces.](figure_HAL_daemon.png)

The HAL daemon has the following major components:
- **Device Manager** which opens, configures and manages multiple types of interfaces  (real or emualted):
- Openning the devices specified in the configuration file, using each one's specified addressing/port and communication mode. 
  - Reading and writing packets, waiting for received packets on all the opened read interfaces and transmitting packets back out onto a write interface.
- **Data Plane Switch**, which forwards data to the correct interface (e.g., from xdd0 to xdd1) based based on the arriving packet's tag and the HAL configuration file mapping rules (**halmap**).
- **Message Functions**, which transform and control packets passing through HAL. Currenlty supported function include:
  - Conversion to and from the internal HAL format (containing tag and ADU) and the different CDG packet formats. Each CDG packet format has a separate HAL sub-component that performs the tag encoding and decoding.


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



