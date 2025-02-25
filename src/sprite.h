/*
 * SPDX-License-Identifier: MIT
 */
#ifndef SPRITE_H
#define SPRITE_H

#include <stdbool.h>
#include <stdint.h>

#include "util.h"

#define SPRITE_WIDTH_PX (24)
#define SPRITE_HEIGHT_PX (21)

#define SPRITE_DOUBLE_WIDTH_PX (SPRITE_WIDTH_PX * 2)
#define SPRITE_DOUBLE_HEIGHT_PX (SPRITE_HEIGHT_PX * 2)

#define SPRITE_IMAGE_EXPAND_X (1 << 0)
#define SPRITE_IMAGE_EXPAND_Y (1 << 1)
#define SPRITE_IMAGE_MULTICOLOR (1 << 2)

struct sprite_frame {
    uint8_t data[63];
    uint8_t flags;
};

struct sprite {
    uint8_t num_frames;
    struct sprite_frame const* const* frames;
};

struct direction_sprite {
    struct sprite const* sprites[DIRECTION_COUNT];
};

struct animation {
    uint8_t rate;
    uint8_t current_frame;
    uint8_t sprite_frame;
    uint8_t __unused;
};

_Static_assert(sizeof(struct animation) == 4,
               "Animation struct size must be power of 2");

void animate_step(uint8_t idx, struct animation* a, struct sprite const* s);
void animate_pause(uint8_t idx, struct animation* a, struct sprite const* s);
uint8_t animate_sprite(uint8_t idx, struct sprite const* sprite, uint8_t frame);
void draw_sprite(uint8_t idx, struct sprite const* sprite, uint8_t color);
void draw_direction_sprite(uint8_t idx, struct direction_sprite const* sprite,
                           uint8_t color, enum direction direction);
uint8_t animate_direction_sprite(uint8_t idx,
                                 struct direction_sprite const* sprite,
                                 enum direction direction, uint8_t frame);
void move_sprite(uint8_t idx, uint16_t x, uint16_t y);
void hide_sprite(uint8_t idx);
bool sprite_is_visible(uint8_t idx);

#endif  // SPRITE_H
