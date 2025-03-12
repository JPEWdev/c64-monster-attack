/*
 * SPDX-License-Identifier: MIT
 */
#ifndef _UTIL_H
#define _UTIL_H

#include <stdbool.h>
#include <stdint.h>

#include "bcd.h"

#define ARRAY_SIZE(arr) ((sizeof(arr)) / (sizeof(arr[0])))

#define _BV(bit) (1 << (bit))
#define _BMASK(bits) ((1 << bits) - 1)

#define disable_interrupts() asm volatile("sei")
#define enable_interrupts() asm volatile("cli")

#define swap(a, b)          \
    do {                    \
        typeof(a) temp = a; \
        (a) = (b);          \
        (b) = temp;         \
    } while (0)

enum direction {
    NORTH,
    SOUTH,
    EAST,
    WEST,
};

#define DIRECTION_COUNT (4)

uint8_t __attribute__((pure)) setbit(uint8_t bit);
uint8_t __attribute__((pure)) clrbit(uint8_t bit);

enum video_type {
    VIDEO_PAL_6596,
    VIDEO_NTSC_6567R8,
    VIDEO_NTSC_6567R65A,
    VIDEO_UNKNOWN,
};

enum video_type detect_video(void);

struct bb {
    uint8_t north;
    uint8_t south;
    uint8_t east;
    uint8_t west;
};

struct bb16 {
    uint8_t north;
    uint8_t south;
    uint16_t east;
    uint16_t west;
};

struct bb16 bb_add_offset(struct bb const* bb, uint16_t x, uint16_t y);
bool bb16_intersect(struct bb16 const* a, struct bb16 const* b);

void int_to_string(int16_t i, char str[7]);
uint8_t u16_to_string(uint16_t i, char str[5]);

uint8_t fast_strlen(char* s);
void pad_string(char* str, uint8_t size, uint8_t offset);

enum direction dir_from(uint16_t from_x, uint16_t from_y, uint16_t to_x,
                        uint16_t to_y);

uint8_t set_all_ram();
void restore_all_ram(uint8_t* port);

uint8_t _disable_int(void);
void _restore_int(uint8_t* p);

#define DISABLE_INTERRUPTS()                                      \
    for (uint8_t _di_todo                                         \
         __attribute__((cleanup(_restore_int))) = _disable_int(); \
         _di_todo; _di_todo = 0)

#define ALL_RAM()                                                           \
    for (uint8_t _ram_todo = 1, _port_save                                  \
                                __attribute__((cleanup(restore_all_ram))) = \
                                    set_all_ram();                          \
         _ram_todo; _ram_todo = 0)

#endif
