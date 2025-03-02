/*
 * SPDX-License-Identifier: MIT
 */
#include "bcd.h"

#include <stdint.h>

bcd_u16 bcd_add_u16(bcd_u16 a, bcd_u16 b) {
    bcd_u16 result;
    __attribute__((leaf)) asm(
        "sed\n"
        "clc\n"
        "lda %1\n"
        "adc %2\n"
        "sta %0\n"
        "lda %1+1\n"
        "adc %2+1\n"
        "sta %0+1\n"
        "cld\n" :
        // Output
        "=r"(result) :
        // Input
        "r"(a),
        "r"(b) :
        // Clobbers
        "a", "c", "v");
    return result;
}

bcd_u16 bcd_sub_u16(bcd_u16 minuend, bcd_u16 subtrahend) {
    bcd_u16 result;
    __attribute__((leaf)) asm(
        "sed\n"
        "sec\n"
        "lda %1\n"
        "sbc %2\n"
        "sta %0\n"
        "lda %1+1\n"
        "sbc %2+1\n"
        "sta %0+1\n"
        "cld\n" :
        // Output
        "=r"(result) :
        // Input
        "r"(minuend),
        "r"(subtrahend) :
        // Clobbers
        "a", "c", "v");
    return result;
}
