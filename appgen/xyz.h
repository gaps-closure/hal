#ifndef XYZ_HEADER_FILE
#define XYZ_HEADER_FILE

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "float.h"

#define DATA_TYP_XYZ    102

/* Data structure */
typedef struct _xyz_datatype {
    double    x;
    double    y;
    double    z;
} xyz_datatype;

/* Data structure: Output */
typedef struct _xyz_output {
    uint64_t  x;
    uint64_t  y;
    uint64_t  z;
} xyz_output;


extern void xyz_print (xyz_datatype *);
extern void xyz_data_encode (void *, size_t *, void *, size_t *);
extern void xyz_data_decode (void *, size_t *, void *, size_t *);
// Guard Provisioning calls to be added here

#endif
