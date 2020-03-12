## HAL API
Partitioned application programs use the HAL data and control-plane APIs to communicate through Cross-Domain Gateways (CDGs).

### HAL Data-Plane API

The HAL Data-Plane API abstracts the different hardware APIs used by 
CDGs, providing a single high-level interace to support all cross-domain 
communication (xdc) between security enclaves. 

Before any data type is sent, the application must provide HAL a description of the 
data. This desciption, contained in a DFDL file, allows HAL to automatically generate the encode and decode funtions to serialize the data. The HAL API provides a single call to generate these functions:
```
xdc_generate (uint8_t *adu, size_t  adu_len, hal_tag  tag);
xdc_asyn_recv (uint8_t *adu, size_t *adu_len, hal_tag *tag);
```


The APP can send and receive the arbitary binary data in Application 
Data Units (ADU) chunks with a user in a different security enclave.  For 
asynchronous communication, the API provides a pub/sub communication model, 
without requiring any feedback from the other enclave or CDG:
```
xdc_asyn_send (uint8_t *adu, size_t  adu_len, hal_tag  tag);
xdc_asyn_recv (uint8_t *adu, size_t *adu_len, hal_tag *tag);
```

The tag provides three orthoganol identifiers for:
. a) Session multiplexing (mux), which acts as a local handle to identify the applicaiton. The mux value is the same for the send and recv calls.
. b) CDG security (sec), selects which security policies will be used to processing sent data. It also gives the security rules that were used to process received data. 
. c) ADU type (typ) describes the data (based on DFDL xsd definition). This tells HAL how to serialize the ADU. The CDG can also use this information to process (e.g., downgrade) the ADU contents.

 
The API allows applications to a) send and receive and 
b) set 

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
