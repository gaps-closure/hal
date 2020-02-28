#ifndef PNT_HEADER_FILE
#define PNT_HEADER_FILE

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define DATA_TYP_PNT    1

/* Data structure: PNT */
typedef struct _pnt_datatype {
    uint16_t    message_id;
    uint16_t    track_index;
    int16_t     lon;
    uint16_t    lon_frac;
    int16_t     lat;
    uint16_t    latfrac;
    int16_t     alt;
    uint16_t    altfrac;
} pnt_datatype;


extern void pnt_print (pnt_datatype *);
extern void pnt_data_encode (uint8_t *, size_t *, uint8_t *, size_t *);
extern void pnt_data_decode (uint8_t *, size_t *, uint8_t *, size_t *);
// Guard Provisioning calls to be added here

#endif
