/* uint64_t (and uint32_t) Endian Conversion Macros */

/*
 * a) htonll host to network (big endian): note htonll = ntohll
 */
#define big_end_test(x) (1==htonl(1))  /* Determine (at runtime sadly) if host is big endian */
/* If byte-swapping uint64_t, swap the two 32=bit words, then swap each 32-bit word,... */
#define swap_uint16(x) (((uint32_t)           ((x) & 0xFF)       <<  8) |            ((x) >>  8))
#define swap_uint32(x) (((uint32_t)swap_uint16((x) & 0xFFFF)     << 16) | swap_uint16((x) >> 16))
#define swap_uint64(x) (((uint64_t)swap_uint32((x) & 0xFFFFFFFF) << 32) | swap_uint32((x) >> 32))
/* Byte swapping macro */
#define htonll(x) (big_end_test(x) ? (x) : swap_uint64(x))
#define ntohll(x) htonll(x)

/*
 * b) htoxll host to x86  (little endian): note htoxll = xtohll
 */
#define htoxll(x) (big_end_test(x) ? swap_uint64(x) : (x))
#define htoxl(x)  (big_end_test(x) ? swap_uint32(x) : (x))

/* Change network endianness (TODO - use case) */
#define FLOAT_BIG_ENDIAN 1      /* Send double/float as: 0 = Little endian, 1 = Big endian */

/* Exported float and double conversion functions */
extern uint32_t    float2net(float);
extern float       net2float(uint32_t);
extern uint64_t    double2net(long double);
extern long double net2double(uint64_t);
