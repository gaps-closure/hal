#ifndef GMA_HEADER_FILE
#define GMA_HEADER_FILE

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define DATA_TYP_POSITION    1
#define DATA_TYP_DISTANCE    2

/* Data structure: PNT */
typedef struct _position_datatype {
    double x;
    double y;
    double z;
} position_datatype;

typedef struct _position_output {
    uint64_t  x;
    uint64_t  y;
    uint64_t  z;
} position_output;

typedef struct _distance_datatype {
    double x;
    double y;
    double z;
} distance_datatype;

typedef struct _distance_output {
    uint64_t  x;
    uint64_t  y;
    uint64_t  z;
} distance_output;

extern void position_print (position_datatype *);
extern void position_data_encode (uint8_t *, size_t *, uint8_t *, size_t *);
extern void position_data_decode (uint8_t *, size_t *, uint8_t *, size_t *);

extern void distance_print (distance_datatype *);
extern void distance_data_encode (uint8_t *, size_t *, uint8_t *, size_t *);
extern void distance_data_decode (uint8_t *, size_t *, uint8_t *, size_t *);

// Guard Provisioning calls to be added here

#endif
