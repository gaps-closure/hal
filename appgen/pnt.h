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
    uint16_t    MessageID;
    uint16_t    TrackIndex;
    int16_t     Lon;
    uint16_t    LonFrac;
    int16_t     Lat;
    uint16_t    LatFrac;
    int16_t     Alt;
    uint16_t    AltFrac;
} pnt_datatype;



extern void pnt_print (pnt_datatype *);
extern void pnt_data_encode (uint8_t *, size_t *, uint8_t *, size_t *);
extern void pnt_data_decode (uint8_t *, size_t *, uint8_t *, size_t *);
// Guard Provisioning calls to be added here

#endif
