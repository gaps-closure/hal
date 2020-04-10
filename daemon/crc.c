/*
 * 16-bit CRC using RFC 1662 Appendix C code (by Drew D. Perkins, CMU)
 *    Input: Data buffer (array of bytes) -> Output: crc16 (uint16_t)
 *
 * Uses CCITT'a generator polynomial: x^16 + x^12 + x^5 + 1
 * Wikipedia shows the different polynomial representations:
 *  16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0   coefficient
 *   1 [0  0  0  1 |0  0  0  0 |0  0  1  0 |0  0  0  1]  Normal
 *     [     1     |     0     |     2     |     1    ]  Normal=0x1021
 *  [1  0  0  0 |0  1  0  0 |0  0  0  0 |1  0  0  0] 1   Reverse
 *  [     8     |     4     |     0     |     8    ]     Hex=0x8408
 * Initial value of CRC=0xffff
 *
 * TODO:
 *  Compare results with another implementation
 */

#include "crc.h"

static uint16_t   fcstab[256];      /* lookup table */

/*
 * Create lookup table (see RFC-1662) in fcstab.
 */
void table_create(uint16_t *fcstab) {
  register unsigned int LookupIndex, v;
  register int i;

  for (LookupIndex=0; LookupIndex<256; LookupIndex++) {
    v = LookupIndex;
    for (i=8; i>0; i--)
      v = v & 1 ? (v >> 1) ^ P : v >> 1;
    fcstab[LookupIndex]=v;
  }
//  fprintf(stderr, "CRC: creating table fcstab[256]: ");
//  fprintf(stderr, "0=%04x 1=%04x 2=%04x ... 255=%04x\n", fcstab[0], fcstab[1], fcstab[2], fcstab[255]);
}

/*
 * Calculate crc given its current value (fcs) and the new data (cp).
 */
uint16_t pppfcs16(uint16_t *fcstab, register uint16_t fcs, register unsigned char *cp, register int len) {
  
  register unsigned int LookupIndex;
  
  assert(sizeof (uint16_t) == 2);
  assert(((uint16_t) -1) > 0);
  // Step 2) For each byte use table lookup to do division
  while (len--) {
    LookupIndex = (fcs ^ *cp++) & 0xff;
    fcs = (fcs >> 8) ^ fcstab[LookupIndex];
  }
  // Step 3) finalize by inverting all the bits */
  fcs ^= 0xffff;
  return (fcs);
}

/*
 * Check CRC is working (see RFC-1662)
 */
void tryfcs16(uint16_t *fcstab) {
  int len=3;
  unsigned char  cp[5]={1,10,100, 0, 0};
  uint16_t trialfcs;

  /* add on output */
  trialfcs = pppfcs16(fcstab, PPPINITFCS16, cp, len );
  cp[len] = (trialfcs & 0x00ff);      /* least significant byte first */
  cp[len+1] = ((trialfcs >> 8) & 0x00ff);

  /* check on input */
  trialfcs = pppfcs16(fcstab, PPPINITFCS16, cp, len + 2 );
//  if ( trialfcs == 0x0f47 ) printf("Good FCS\n");
//  else printf("Bad FCS %04x\n", trialfcs);
}

/* Caluculated CRC for a given bufer (starting from scratch) */
uint16_t crc16(uint8_t *buf, size_t len) {
  static int  do_once=1;
  uint16_t    crc=0;
  
  if (do_once==1) {
    table_create(fcstab);
    tryfcs16(fcstab);
    do_once=0;
  }
  // Step 1) Initialize crc to starting value to all 1's
  crc = pppfcs16(fcstab, PPPINITFCS16, buf, len);
  return (crc);
}

/* Local testing */
/*
int main(void) {
  uint8_t  buf[3]={1,10,128}; // 0x010a80 = 68224
  size_t   len = 3;
  uint16_t crc=0;

  crc = crc16(buf, len);
  printf("crc=0x%04x\n", crc);
  return (0);
}
*/
