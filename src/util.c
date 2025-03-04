/*
 * SPDX-License-Identifier: MIT
 */

#include "util.h"

#include <string.h>

#include "reg.h"

#define setbit_helper(b) \
    case b:              \
        return 1 << b

uint8_t setbit(uint8_t bit) {
    switch (bit) {
        setbit_helper(0);
        setbit_helper(1);
        setbit_helper(2);
        setbit_helper(3);
        setbit_helper(4);
        setbit_helper(5);
        setbit_helper(6);
        setbit_helper(7);
    }
    return 0;
}

#define clrbit_helper(b) \
    case b:              \
        return (uint8_t)(~(1 << b))

uint8_t clrbit(uint8_t bit) {
    switch (bit) {
        clrbit_helper(0);
        clrbit_helper(1);
        clrbit_helper(2);
        clrbit_helper(3);
        clrbit_helper(4);
        clrbit_helper(5);
        clrbit_helper(6);
        clrbit_helper(7);
    }
    return 0xFF;
}

enum video_type detect_video(void) {
    uint8_t r;
    __attribute__((leaf)) asm volatile(
        "1:\n"
        "   lda $d012\n"
        "2:\n"
        "   cmp $d012\n"
        "   beq 2b\n"
        "   bmi 1b\n" :
        // Outputs
        "+a"(r) :
        // Inputs
        :
        // Clobber
        "c",
        "v");

    switch (r) {
        case 0x37:
            return VIDEO_PAL_6596;
        case 0x06:
            return VIDEO_NTSC_6567R8;
        case 0x05:
            return VIDEO_NTSC_6567R65A;
    }

    return VIDEO_UNKNOWN;
}

struct bb16 bb_add_offset(struct bb const* bb, uint16_t x, uint16_t y) {
    struct bb16 result;
    result.north = y + bb->north;
    result.south = y + bb->south;
    result.east = x + bb->east;
    result.west = x + bb->west;
    return result;
}

bool bb16_intersect(struct bb16 const* a, struct bb16 const* b) {
    return (a->north <= b->south) && (b->north <= a->south) &&
           (a->west <= b->east) && (b->west <= a->east);
}

void int_to_string(int16_t i, char str[7]) {
    if (i == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    bool negative = false;
    if (i < 0) {
        i = -i;
        negative = true;
    }

    char* c = str;
    while (i) {
        *c = '0' + (i % 10);
        i = i / 10;
        c++;
    }

    if (negative) {
        *c = '-';
        c++;
    }

    *c = '\0';

    // Reverse string
    char* s = str;
    c--;
    for (; c > s; c--, s++) {
        *c ^= *s;
        *s ^= *c;
        *c ^= *s;
    }
}

static char get_hex_char(uint8_t n) {
    char result;
    // A neat trick is that adding '0' to the nibble in BCD mode will correctly
    // choose the correct hex character, since the overflow when >= 10 will
    // place the index into the upper case numbers
    __attribute__((leaf)) asm(
        "sed\n"
        "cmp #10\n"
        "adc #%2\n"
        "cld\n" :
        // Output
        "+a"(result) :
        // Input
        "a"(n),
        "i"('0') :
        // Clobbers
        "c", "v");
    return result;
}

// Converts a uint16_t to a hexadecimal string. The uint16_t can be either
// binary, or BCD
uint8_t u16_to_string(uint16_t v, char str[5]) {
    uint8_t num_chars = 1;
    if (v & 0xF000) {
        num_chars = 4;
    } else if (v & 0x0F00) {
        num_chars = 3;
    } else if (v & 0x00F0) {
        num_chars = 2;
    }
    str[num_chars] = '\0';
    for (uint8_t i = 1; i <= num_chars; i++) {
        str[num_chars - i] = get_hex_char(v & 0xF);
        v = v >> 4;
    }
    return num_chars;
}

enum direction dir_from(uint16_t from_x, uint16_t from_y, uint16_t to_x,
                        uint16_t to_y) {
    int8_t delta_x = from_x - to_x;
    int8_t delta_y = from_y - to_y;

    if (delta_x >= 0) {
        if (delta_y >= 0) {
            if (delta_x > delta_y) {
                return WEST;
            } else {
                return NORTH;
            }
        } else {
            if (delta_x > -delta_y) {
                return WEST;
            } else {
                return SOUTH;
            }
        }
    } else {
        if (delta_y >= 0) {
            if (-delta_x > delta_y) {
                return EAST;
            } else {
                return NORTH;
            }
        } else {
            if (-delta_x > -delta_y) {
                return EAST;
            } else {
                return SOUTH;
            }
        }
    }
}

uint8_t _disable_int(void) {
    disable_interrupts();
    return 1;
}

void _restore_int(uint8_t* p) { enable_interrupts(); }

uint8_t set_all_ram(void) {
    uint8_t p = PROCESSOR_PORT;
    PROCESSOR_PORT = (PROCESSOR_PORT & ~MEMORY_MASK) | MEMORY_RAM_ONLY;
    return p;
}

void restore_all_ram(uint8_t* port) { PROCESSOR_PORT = *port; }

