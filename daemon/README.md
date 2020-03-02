## HAL Daemon
This daemon directory contains the Hardware Abstraction Layer (HAL) Service.
The HAL Service runs as a daemon (typically started by a systemd script at boot 
time).  Based on its conifguration file (e.g., sample.cfg) HAL opens, conigures 
and manages cross domain guard devices (real or emualted). It also mediates 
the  exchange between the application and the guard devices, handling the 
encoding/decoding, multiplexing/demultiplexing, segmentation/reassembly and 
rate control, as applicable.
