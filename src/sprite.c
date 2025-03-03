/*
 * SPDX-License-Identifier: MIT
 */
#include "sprite.h"

#include "reg.h"

uint8_t sprite_pointers_shadow[8];

void update_sprite_pointers() {
    DISABLE_INTERRUPTS() {
        ALL_RAM() {
            sprite_pointers[0] = sprite_pointers_shadow[0];
            sprite_pointers[1] = sprite_pointers_shadow[1];
            sprite_pointers[2] = sprite_pointers_shadow[2];
            sprite_pointers[3] = sprite_pointers_shadow[3];
            sprite_pointers[4] = sprite_pointers_shadow[4];
            sprite_pointers[5] = sprite_pointers_shadow[5];
            sprite_pointers[6] = sprite_pointers_shadow[6];
            sprite_pointers[7] = sprite_pointers_shadow[7];
        }
    }
}

