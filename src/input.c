/*
 * SPDX-License-Identifier: MIT
 */
#include "input.h"

#include <stdint.h>

#include "reg.h"

uint16_t read_keyboard(void) {
    CIA_1_DDR_B = 0;
    CIA_1_DDR_A = 0xFF;

    // Check if any key is pressed
    CIA_1_PORT_A = 0;
    if (CIA_1_PORT_B == 0xFF) {
        return NO_KEY;
    }

#pragma clang loop unroll(full)
    for (uint8_t col = 0, mask = 0x80; col < 64; col += 8, mask >>= 1) {
        CIA_1_PORT_A = ~mask;
        uint8_t p = ~CIA_1_PORT_B;
        if (p) {
            uint8_t key = col;
            while (!(p & 0x1)) {
                key++;
                p = p >> 1;
            }
            return key;
        }
    }

    return NO_KEY;
}

uint8_t read_joystick_1(void) {
    CIA_1_DDR_B = 0;
    return ~CIA_1_PORT_B;
}

uint8_t read_joystick_2(void) {
    CIA_1_DDR_A = 0;
    return ~CIA_1_PORT_A;
}
