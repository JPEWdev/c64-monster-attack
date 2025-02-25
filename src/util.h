/*
 * SPDX-License-Identifier: MIT
 */
#ifndef _UTIL_H
#define _UTIL_H

#include <stdbool.h>
#include <stdint.h>

#define ARRAY_SIZE(arr) ((sizeof(arr)) / (sizeof(arr[0])))

#define _BV(bit) (1 << (bit))

#define disable_interrupts() asm volatile("sei")
#define enable_interrupts() asm volatile("cli")

enum direction {
    NORTH,
    SOUTH,
    EAST,
    WEST,
};

#define DIRECTION_COUNT (4)

uint8_t __attribute__((pure)) setbit(uint8_t bit);
uint8_t __attribute__((pure)) clrbit(uint8_t bit);

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

enum direction dir_from(uint16_t from_x, uint16_t from_y, uint16_t to_x,
                        uint16_t to_y);

uint8_t set_all_ram(void);
void restore_all_ram(uint8_t* port);

#define ALL_RAM()                                                       \
    for (uint8_t _todo = 1, _port_save                                  \
                            __attribute__((cleanup(restore_all_ram))) = \
                                set_all_ram();                          \
         _todo; _todo = 0)

#endif
