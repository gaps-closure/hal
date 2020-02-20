This directory contains the CLOSURE Hardware Abstraction Layer (HAL) Service.
The HAL Service runs as a daemon (typically started by a systemd script at boot 
time).  Based on its conifguration file (e.g., sample.cfg) HAL opens, conigures 
and manages cross domain guard devices (real or emualted). It also mediates 
the  exchange between the application and the guard devices, handling the 
encoding/decoding, multiplexing/demultiplexing, segmentation/reassembly and 
rate control, as applicable.


# HAL API
The figure below show the data and control-plane APIs between HAL and the 
partitioned application programs.

![HAL API](hal_api.png)

## HAL Data-Plane API

The HAL Data-Plane API gives single uniform high-level interface to APPs, 
abstracting the different hardware APIs used by cross-domain programs. 
The API allows applications to a) send and receive arbitary binary data and 
b) set the tag (mux, sec, typ) values that control multiplexing, security and 
data formatting: 
* The tag session ID (mux) determines which application will receive the data.
* The tag security ID (sec) determines the security policies required.
* The tag data type (typ) identifies the type of application data (and its 
  associated DFDL description). 

In order to send and receive cross-domain data via HAL, applications programs use 
CLOSURE API functions. The C-program API is available by including  "closure.h" and 
linking with the CLOSURE library (libclosure.a).  

The API provides functions to write/read the HAL tag structure from/to mux, 
sec and typ values:
```
tag_write (hal_tag *tag, uint32_t  mux, uint32_t  sec, uint32_t  typ);
tag_read  (hal_tag *tag, uint32_t *mux, uint32_t *sec, uint32_t *typ);
```
Before sending the data, the APP must also encode/decode data to/from the 
CLOSURE serialized Application Data Units (adu) format based on its type. 
For example, to send/receive PNT data  (pnt1), it sets typ=DATA_TYP_PNT:
```
gaps_data_encode(uint8_t *adu, size_t *adu_len, uint8_t *pnt1, size_t *pnt1_len, int DATA_TYP_PNT);
gaps_data_decode(uint8_t *adu, size_t *adu_len, uint8_t *pnt1, size_t *pnt1_len, int DATA_TYP_PNT);
```
The APP can then send/receive the adu based on a specified tag value. For
asynchronous communication, the CLOSURE API provides the following functions:
```
gaps-asyn-send    (uint8_t *adu, size_t  adu_len, hal_tag  tag);
gaps-asyn-receive (uint8_t *adu, size_t *adu_len, hal_tag *tag);
```

Below is an example program app_test.c
```
#include "closure.h"

void pnt_set (pnt_datatype *pnt) {
    pnt->message_id  = 130;
    pnt->track_index = 165;
    pnt->lon         = 100;
    pnt->lon_frac    = 32768;
    pnt->lat         = 67;
    pnt->latfrac     = 49152;
    pnt->alt         = 2;
    pnt->altfrac     = 0;
}

int main(int argc, char **argv) {
  uint8_t       adu[ADU_SIZE_MAX];
  size_t        adu_len, pnt1_len;
  pnt_datatype  pnt1;
  gaps_tag      tag;
  
  /* a) Create CLOSURE inputs (data and tag) */
  pnt1_len=(size_t) sizeof(pnt1);
  pnt_set(&pnt1); pnt_print(&pnt1);
  tag_write(&tag, 1, 2, DATA_TYP_PNT);
  /* b) Encode data and sent to CLOSURE */
  gaps_data_encode(adu, &adu_len, (uint8_t *) &pnt1, &pnt1_len, DATA_TYP_PNT);
  gaps_asyn_send(adu,  adu_len,  tag);
  /* c) Receive data from CLOSURE and decode */
  gaps_asyn_recv(adu, &adu_len, &tag);
  gaps_data_decode(adu, &adu_len, (uint8_t *) &pnt1, &pnt1_len, DATA_TYP_PNT);
  fprintf(stderr, "app received "); tag_print(&tag); pnt_print(&pnt1);
  return (0);
}
```
## HAL Control-Plane API
HAL Control-Plane API provisions using a libconfig File, which contains:
* HAL maps (routes):
  [fromdev, frommux, fromsec, fromtyp, todev, tomux, tosec, totyp, codec]
  
  * Determine how packets are routed from arriving device to a departing device (based on tag information).  
  Devices include hardware interfaces (e.g., serial or ethernet) and the CLOSURE API.
  * Determine packet data transformations (codec), based on data-type DFDL schema file name.

* Device configurations:
  * Device IDs (e.g., /dev/gaps1), 
  * Devices configuration (e.g., addresses, ports).
  * GAPS packet header DFDL schema file name.
  * Max packet size (HAL may perform Segment and Reassemble (SAR)), 
  * Max rate (bits/second).

* Cross Domain Guard (CDG) provisioning:
  * DFDL schema file per: a) data-type, and b) per GAPS packet header format.
  * MLS policy rules (e.g., pass/allow/filter), based on the DFDL-described data-plane fields (in a device-specific PDU).
  * CDG hardware configuration (including pipeline setup?)

* Cross Domain Guard (CDG) provisioning:

# Prerequisites

Make sure to install HAL pre-requisites.
```
sudo apt install -y libzmq3-dev
sudo apt install -y libconfig-dev
```

Compile HAL, together with the closure libarary (libclosure.a), HAL utilities, and application (e.g., app_test.c)
```
cd ~/gaps/top-level/hal/
make clean; make
```

# TESTING 
## Using a Test APP (using the CLOSURE API) with HAL

Runs the test applicaiton (send and receiving encoded data) using the CLOSURE API, with HAL and a socat device
(to emulate the network).

```
cd ~/gaps/top-level/hal/
# 1) Start the network device 
./net.sh

# 2a) Start HAL as a loopback device
./hal sample_loopback.cfg
# 2b) Start HAL sending to device
./hal sample.cfg


# 3) Start the test APP
./app_test
```

## Using a Test APP (using the CLOSURE API)  -  in isolation 

Runs the test applicaiton (send and receiving encoded data) using the CLOSURE API; but without HAL (using 'zc' utility to 
emulate HAL sub and pub. 
```
cd ~/gaps/top-level/hal/
# 1) Model the HAL subscriber:
zc/zc -b sub ipc://halsub | od -t x1 -w1 -v
# 2) Start the test APP
./app_test
# 3) Model the HAL publisher:
echo "008200A50064800043C00000020000" | xxd -r -p | zc/zc -b pub ipc://halpub
```


## Using ZC with HAL - using scripts
Runs HAL and a socat device (to emulate the network); but uses zcat to emulate a test applicaiton (without the CLOSURE API) 
Run 5 scripts (each in separate windows): to start the monitor, netwrok device, HAL, and 2 APPs.

```
# 1) Start the monoitor
sudo tshark -i lo -f "port 12345" -T fields -e data

# If you want to see MAC,IP,TCP headers, use
# sudo tshark -i lo -f "port 12345" -x 


# 2) Start the network device 
./net.sh

# 3) Start HAL (run 'make' if hal.c or zc.c has been modified)
./hal sample.cfg

# 4) Start APPs 1 and 2
./app 1
./app 2
```

## Using ZC with HAL - using raw commands

Testing with a socat device sending and receiving BKEND Format binary data

A) Setup (run each command in a separate window).
```
# 1) netcat listens on localhost port 1234 and converts packets to and from hex format
#    on stdout and stdin
stdbuf -oL xxd -r -p | netcat -4 -l -k 127.0.0.1 1234 2>&1 | od -t x1 -w1 -v

# 2) Start a serial device (/dev/vcom1) that communicates to netcat
sudo socat -d -d -lf socat_log.txt pty,link=/dev/vcom1,raw,ignoreeof,unlink-close=0,echo=0 tcp:127.0.0.1:1234,ignoreeof
sudo chmod 777 /dev/vcom1

# 3) Start HAL and 2 Applications (app1 and app2) emulationed with zc
#    Note: Make sure the socat device (/dev/vcom1) is enabled in sample.cfg.
./hal sample.cfg
zc/zc -v -dEOF pub ipc://halsub
zc/zc -v -f 'tag-app1' sub ipc://halpub
zc/zc -v -dEOF pub ipc://halsub
zc/zc -v -f 'tag-app2' sub ipc://halpub
#    Note: Publisher or app1 and app2 are the same (just use different tags - see below)
#          Add more zc pub-sub pairs (with new sub filter) to emulate further apps
```

B) Passing data ( between app and device)
```
# 1) Send from APP by typing in a zc halsub window, using a haljson formated tag and string.
#    (three '-' separated tag strings [mux, sec, typ], a space. and a data string)
#    For example, to emulate sending from app1:
tag-app1-m1-d1 ABCDEFGHIJK
#    To emulate sending from app1:
tag-app2-m2-d2 LMNOPQRST
#    Note: Output from both apps will be printed in the netcat window (in hex)

# 2) Send from device by typing in the netcat window, using a bkend formated hex data
#    (Five uint32_t (4-byte) tags [mux, sec, count, typ, len] followed by the data)
#    For example, to emulate sending to app1:
00 00 00 01 00 00 00 01 00 00 00 01 00 00 00 01 00 00 00 0c 61 62 63 64 65 66 67 68 69 6a 6b 0a
#    To emulate sending to app2:
00 00 00 02 00 00 00 02 00 00 00 01 00 00 00 02 00 00 00 0a 6c 6d 6e 6f 70 71 72 73 74 0a
#    Note: Output will be printed in the APP zc halpub window (as a string)
```


Loopback APP-side testing (without a device)
```
# 1) Start the HAL and the Application(s) send and receive emulation (using zc)
./hal sample_zc_loopback.cfg
zc/zc -v -dEOF pub ipc://halsub
zc/zc -v sub ipc://halpub
#    Note: sample_zc_loopback.cfg disables all devices and routes from zc to zc
```
