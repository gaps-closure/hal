/*
 * uint64_t Conversion Macros
 *   Test (1==htonl(1)) determines (at runtime sadly) if we need byte-swapping.
 *   If byte-swapping, then swap each 32-bits of uint64_t using htonl (and swap the two 32=bit words).
 */
#define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))

/* Exported double packing functions */
extern uint64_t      pack754_be(long double);
extern long double unpack754_be(uint64_t);
