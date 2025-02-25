/*
 * SPDX-License-Identifier: MIT
 */
#include "sprite.h"

#include "reg.h"

void animate_step(uint8_t idx, struct animation *a, struct sprite const *s) {
    a->current_frame++;
    if (a->current_frame >= a->rate) {
        a->current_frame = 0;
        a->sprite_frame = animate_sprite(idx, s, a->sprite_frame + 1);
    }
}

uint8_t animate_sprite(uint8_t idx, struct sprite const *sprite,
                       uint8_t frame) {
    if (frame >= sprite->num_frames) {
        frame = 0;
    }
    uint8_t s =
        ((uint16_t)(sprite->frames[frame]) - (uint16_t)&video_base) / 64;

    uint8_t flags;
    ALL_RAM() {
        sprite_pointers[idx] = s;
        flags = sprite->frames[frame]->flags;
    };

    if (flags & SPRITE_IMAGE_EXPAND_Y) {
        VICII_SPRITE_Y_EXPAND |= setbit(idx);
    } else {
        VICII_SPRITE_Y_EXPAND &= clrbit(idx);
    }

    if (flags & SPRITE_IMAGE_EXPAND_X) {
        VICII_SPRITE_X_EXPAND |= setbit(idx);
    } else {
        VICII_SPRITE_X_EXPAND &= clrbit(idx);
    }

    if (flags & SPRITE_IMAGE_MULTICOLOR) {
        VICII_SPRITE_MULTICOLOR |= setbit(idx);
    } else {
        VICII_SPRITE_MULTICOLOR &= clrbit(idx);
    }

    return frame;
}

void draw_sprite(uint8_t idx, struct sprite const *sprite, uint8_t color) {
    VICII_SPRITE_ENABLE |= setbit(idx);

    VICII_SPRITE_COLOR[idx] = color;

    animate_sprite(idx, sprite, 0);
}

void move_sprite(uint8_t idx, uint16_t x, uint16_t y) {
    VICII_SPRITE_POSITION[idx].x = x & 0xFF;
    VICII_SPRITE_POSITION[idx].y = y;

    if (x & _BV(8)) {
        VICII_SPRITE_MSB |= setbit(idx);
    } else {
        VICII_SPRITE_MSB &= clrbit(idx);
    }
}

void hide_sprite(uint8_t idx) { VICII_SPRITE_ENABLE &= clrbit(idx); }

bool sprite_is_visible(uint8_t idx) {
    return (VICII_SPRITE_ENABLE & setbit(idx)) != 0;
}

void draw_direction_sprite(uint8_t idx, struct direction_sprite const *sprite,
                           uint8_t color, enum direction direction) {
    draw_sprite(idx, sprite->sprites[direction], color);
}

uint8_t animate_direction_sprite(uint8_t idx,
                                 struct direction_sprite const *sprite,
                                 enum direction direction, uint8_t frame) {
    return animate_sprite(idx, sprite->sprites[direction], frame);
}

