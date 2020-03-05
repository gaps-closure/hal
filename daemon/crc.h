/* CRC-16-CCITT with generator polynomial: x^16 + x^12 + x^5 + 1 */

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>

/*
 * CRC-16-CCITT'a generator polynomial: x^16 + x^12 + x^5 + 1
 * Wikipedia shows the different CRC representations:
 *  16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0   coefficient
 *   1 [0  0  0  1 |0  0  0  0 |0  0  1  0 |0  0  0  1]  Normal
 *     [     1     |     0     |     2     |     1    ]  Normal=0x1021
 *  [1  0  0  0 |0  1  0  0 |0  0  0  0 |1  0  0  0] 1   Reverse
 *  [     8     |     4     |     0     |     8    ]     Hex=0x8408
 * Initial value of CRC=0xffff
 */

#define P             0x8408   /* CRC-16-CCITT polynomial reversed */
#define PPPINITFCS16  0xffff   /* Initial CRC value */

uint16_t crc16(uint8_t *, size_t);
