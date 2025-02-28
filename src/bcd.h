/*
 * SPDX-License-Identifier: MIT
 */
#ifndef _BCD_H
#define _BCD_H

#include <stdint.h>

#define BCD8(v) (((v) % 10) | ((((v) / 10) % 10) << 4))
#define BCD16(v)                                                         \
    (((v) % 10) | ((((v) / 10) % 10) << 4) | ((((v) / 100) % 10) << 8) | \
     ((((v) / 1000) % 10) << 12))

typedef uint8_t bcd_u8;
typedef uint16_t bcd_u16;

bcd_u16 bcd_add_u16(bcd_u16 a, bcd_u16 b);
bcd_u16 bcd_sub_u16(bcd_u16 minuend, bcd_u16 subtrahend);

#endif
