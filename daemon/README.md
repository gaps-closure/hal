## HAL Daemon
This daemon directory contains the Hardware Abstraction Layer (HAL) Service.
The HAL Service runs as a daemon (typically started by a systemd script at boot 
time).  Based on its conifguration file HAL:
. Opens, configures and manages multiple types of interfaces (real or emualted).
. Routes packets between any pair or interfaces (based on the configured route map).
. Translates the HAL packet headers (based on the interface packet model).

The types of intefaces supported currently include:
. Serial devices (e.g., tty0) carrying TCP/IP packets.
. Network devices (e.g., eth0) carrying either UDP or TCP packets (in client and/or server mode).
. ZeroMQ pub/sub with the applications.

The HAL directory contains two example coniguration file (based on a libconfig format):
. Loopback example, useful for local testing of the API ([sample_loopback.cfg](../sample_loopback.cfg)).
. Network example, with multiple network devices 
([sample.cfg](../sample.cfg)).

Packet formats can be spefcified per device instance (in the conifg file). 
The packet format and translation programs are contained in separate files
(packetize_XXX.c and packetize_XXX.h).

Planned HAL extensions include:
. Configuring the cross domain guards.
. Performing any ADU encoding/decoding.
. Mediating  exchange between the application and the guard devices, handling the multiplexing/demultiplexing, segmentation/reassembly and rate control, as applicable.

