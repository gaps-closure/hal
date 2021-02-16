#ifndef GMA_HEADER_FILE
#define GMA_HEADER_FILE

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define DATA_TYP_POSITION     1
#define DATA_TYP_DISTANCE     2
#define DATA_TYP_RAW          3
#define DATA_TYP_HB_ORANGE   13
#define DATA_TYP_HB_GREEN   113
#define DATA_TYP_BIG          0x01234567    /* Test Big Data Type ID (19088743) */

typedef struct _trailer_datatype {
	uint32_t seq;
	uint32_t rqr;
	uint32_t oid;
	uint16_t mid;
	uint16_t crc;
} trailer_datatype;

/* Data structure: 1) Position */
typedef struct _position_datatype {
    double x;
    double y;
    double z;
    trailer_datatype trailer;
} position_datatype;

typedef struct _position_output {
    uint64_t  x;
    uint64_t  y;
    uint64_t  z;
    trailer_datatype trailer;
} position_output;

/* Data structure: 2) Distance */
typedef struct _distance_datatype {
    double x;
    double y;
    double z;
    trailer_datatype trailer;
} distance_datatype;

typedef struct _distance_output {
    uint64_t  x;
    uint64_t  y;
    uint64_t  z;
    trailer_datatype trailer;
} distance_output;

/* Data structure: 3) raw_btyes (variable length data is after raw_datatype struct) */
typedef struct _raw_datatype {
    uint32_t data_len;
    trailer_datatype trailer;
} raw_datatype;


extern void position_print (position_datatype *position);
extern void position_data_encode (void *buff_out, void *buff_in, size_t *len_out);
extern void position_data_decode (void *buff_out, void *buff_in, size_t *len_in);

extern void distance_print (distance_datatype *distance);
extern void distance_data_encode (void *buff_out, void *buff_in, size_t *len_out);
extern void distance_data_decode (void *buff_out, void *buff_in, size_t *len_in);

extern void raw_print (raw_datatype *distance);
extern void raw_data_encode (void *buff_out, void *buff_in, size_t *len_out);
extern void raw_data_decode (void *buff_out, void *buff_in, size_t *len_in);



// Guard Provisioning calls to be added here

#endif
