## HAL API
The defines the data and control-plane APIs between HAL and the 
partitioned application programs.

### HAL Data-Plane API

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
For the send function, the tag specifies the packet tag information that 
accompanies the data; for the receive function, the tag both specifies the desired
packets and returns the tag field in the received packet.

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
  uint32_t      mux=1, sec=2, typ=DATA_TYP_PNT;

  /* a) Create CLOSURE inputs (data and tag) */
  pnt1_len=(size_t) sizeof(pnt1);
  pnt_set(&pnt1); pnt_print(&pnt1);
  if (argc >= 2)  mux = atoi(argv[1]);
  tag_write(&tag, mux, sec, typ);
  /* b) Encode data and send to CLOSURE */
  gaps_data_encode(adu, &adu_len, (uint8_t *) &pnt1, &pnt1_len, typ);
  gaps_asyn_send(adu,  adu_len,  tag);
  /* c) Receive data from CLOSURE and decode */
  gaps_asyn_recv(adu, &adu_len, &tag);
  gaps_data_decode(adu, &adu_len, (uint8_t *) &pnt1, &pnt1_len, typ);
  fprintf(stderr, "app received "); tag_print(&tag); pnt_print(&pnt1);
  return (0);
}

```
### HAL Control-Plane API
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
