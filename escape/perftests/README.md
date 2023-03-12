## HAL Daemon
This README describes Speed Test using ESCAPE


## Contents

- [ESCAPE LINUX CONFIGURATION](#escape-linux-config)
- [TEST PROGRAM](#test-program)


## ESCAPE LINUX CONFIGURATION
The ...

### Grub
The 
```
$ cat /etc/default/grub
...
GRUB_DEFAULT=0
GRUB_TIMEOUT_STYLE=hidden
GRUB_TIMEOUT=0
GRUB_DISTRIBUTOR=`lsb_release -i -s 2> /dev/null || echo Debian`
GRUB_CMDLINE_LINUX_DEFAULT="quiet splash"
GRUB_CMDLINE_LINUX="memmap=16G\\\$130G"
...


## TEST Program
To see 
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
HAL Configuration currently uses a a libconfig file specified when starting the HAL daemon
(see the [Quick Start Guide](../README.md#quick-start-guide) and 
[HAL Installation and Usage](../README.md#hal-installation-and-usage)).

The HAL daemon configuration file contains two sections:
- **devices-spec**, which specifies the device configuration for each HAL interface, including:
  - Interface ID (e.g., xdd1), 
  - enable flag,
  - packet format,
  - communication mode,
  - device paths,
  - [optional] addresses and ports,
  - [optional] max packet size (HAL may perform Segment and Reassemble (SAR)),
  - [optional] max rate (bits/second).
- **halmap** routing rules and message functions applied to each allowed unidirectional link.
  - *from_* fields specifying the inbound HAL Interface ID and packet tag values,
  - *to_* fields specifying the outbound HAL Interface ID and packet tag values,
  - message functions specific to this path (e.g., ADU codec).


The [test directory](../test/) has examples of configuration files (with a .cfg) extension. Note that, if there are multiple HAL daemon instances on a node (e.g., for testing), then they must be configured with different interfaces.
