/*
 * SPDX-License-Identifier: MIT
 */
#include "isr.h"

#include <cbm.h>
#include <stdint.h>

#include "player.h"
#include "reg.h"
#include "util.h"

uint8_t volatile frame_count = 0;
uint8_t __zeropage player_collisions = 0;
uint8_t __zeropage sword_collisions = 0;

uint8_t __zeropage vicii_bg_0_next;
uint8_t __zeropage vicii_ctrl_1_next;
uint8_t __zeropage vicii_ctrl_2_next;
uint8_t __zeropage vicii_interrupt_enable_next;
uint8_t __zeropage vicii_raster_next;

uint8_t get_player_collisions(void) {
    disable_interrupts();
    uint8_t c = player_collisions;
    player_collisions = 0;
    enable_interrupts();
    return c;
}

uint8_t get_sword_collisions(void) {
    disable_interrupts();
    uint8_t c = sword_collisions;
    sword_collisions = 0;
    enable_interrupts();
    return c;
}

