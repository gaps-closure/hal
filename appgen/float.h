/*
 * uint64_t Conversion Macros: a) host to network (big endian), b) host to x86 (little endian)
 *   Test determines (at runtime sadly) if host is big endian).
 *   If byte-swapping, then swap each 32-bits using htonl (and swap the two 32=bit words).
 */
#define big_end_test(x) (1==htonl(1))
#define swap_uint64(x) (((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define htonll(x) (big_end_test(x) ? (x) : swap_uint64(x))
#define htoxll(x) (big_end_test(x) ? swap_uint64(x) : (x))

#define FLOAT_BIG_ENDIAN 0      /* 0 = Little endian, 1 = Big endian */

/* Exported double packing functions */
extern uint64_t      pack754_be(long double);
extern long double unpack754_be(uint64_t);
