This directory will include the Hardware Abstraction Layer (HAL) Service which
abstracts the TA1 API for use by the cross-domain programs. The HAL Service
runs as a daemon (typically started by a systemd script at boot time).  

It allows application to send and receive cross-domain data and tags over
ZeroMQ. It opens and manages cross domain guard devices (real or emualted). It
mediates the exchange between the application and the guard devices, handling
the encoding/decoding and multiplexing/demultiplexing as applicable.

Make sure to install pre-requisites.
```
sudo apt install -y libzmq3-dev
sudo apt install -y libconfig-dev
```


**This directory needs to be moved out of emulator into its own repository, as it
is a CLOSURE component that applies to both real and emulated systems.**

