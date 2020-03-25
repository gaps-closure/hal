## HAL API
Partitioned application programs use the HAL API to communicate data through the GAPS Devices, which we call Cross-Domain Gateways (CDGs), as well as to configure their control planes. The data-plane client API (currently implemented) is primarily of interest to cross-domain application developers, whereas the control-plane API (to be implemented) will pertain to GAPS device developers.

### HAL Data-Plane Client API

The HAL Data-Plane API abstracts the different hardware APIs used by CDGs, providing a single high-level interace to support all cross-domain communication (xdc) between security enclaves. The client API is available as a library that cross-domain applications can link to. We describe the high level API here, and several lower-level calls are available (see `xdcomms.h`).

The application needs to perform some initialization steps before it can send and receive data.

The HAL architecture will support a multitude of application communication patterns. The current version of HAL implements the 0MQ pub/sub pattern, and there are URIs associated for the 0MQ publish and subscribe endpoints bound by the HAL daemon. The application client API provides the following functions to set these endpoint URIs that the HAL daemon is configured with, so that they can be used to send and receive data.

```
extern char *xdc_set_in(char *);
extern char *xdc_set_out(char *);
```

Additionally, the application must register (de-)serialization codec functions for all the datatypes that can be sent over the CDG. The `hal/appgen` directory will include a number of such codec functions, which are generated from the application. Once registered, the correct codec will be selected and invoked when data is sent or received by HAL.

```
extern void xdc_register(codec_func_ptr encoder, codec_func_ptr decoder, int type);
```

Once the initialization steps are completed, the application can send and receive data. Since the codecs handle the (de-)serialization, applications can conveniently send and receive data using pointers to in-memory data structures. However, the application must provide the HAL application tag (`gaps_tag`) for the data item to be sent or received.

```
typedef struct _tag {
  uint32_t    mux;      /* APP ID */
  uint32_t    sec;      /* Security tag */
  uint32_t    typ;      /* data type */
} gaps_tag;
```

The `gaps_tag` structure provides three orthogonal identifiers:
. a) Session multiplexing (mux), which acts as a local handle to identify the applicaiton. The mux value is the same for the send and recv calls.
. b) CDG security (sec), selects which security policies will be used to processing sent data. It also gives the security rules that were used to process received data. 
. c) ADU type (typ) describes the data (based on DFDL xsd definition). This tells HAL how to serialize the ADU. The CDG can also use this information to process (e.g., downgrade) the ADU contents.

Although a number of communication patterns are envisioned, currently,  asynchronous send and a blocking receive (blocks until a message matching the specified tag is received) are supported. These client-side calls are mapped to the pub/sub endpoints that are supported by the HAL daemon.

```
extern void xdc_asyn_send(void *send_buf, gaps_tag tag);
extern void xdc_blocking_recv(void *recv_buf, gaps_tag *tag);
```

In future versions of this API, we plan to support additional send and receive communication patterns including asynchronous receive calls using one-shot or repeated callbacks that can be registered by the application, sending a tagged request and receiving a reply matching the tag, suport for a stream of sequenced messages with in-order delivery, etc.

In summary, the application links to the HAL data-plane client API library (`libxdcomms.a`). Upon startup, the application initializes the URIs for the 0MQ endpoints, registers codecs for the application datatypes, and then sends and receives data using pointers to in-memory data structures and associated tags. An example test program that makes uses of this client API can be found in `hal\test\halperf.py`.

### HAL Control-Plane API

Eventually we will provide a number of additional API calls to: (i) generate configuration for HAL daemon and the GAPS Devices (CDG) at provision-time; (ii) apply these configurations to HAL and the GAPS Devices. Currently these are configured offline by the system administrator, however, future versions will support auto-generation of CDG configurations, and dynamic provisioning of these configurations into the CDG. 

The HAL daemon configuration uses a libconfig File, which contains:
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

The Cross Domain Guard (CDG) provision-time configuration will include:
* DFDL schema file describing the framing for each GAPS Device including headers, application datatypes, and trailers.
* Security policy rules (e.g., pass/allow/filter), based on the DFDL-described data-plane fields (in a device-specific PDU)
* CDG hardware configuration based on system topology, and optionally, device filter pipeline (for programmable hardware)
