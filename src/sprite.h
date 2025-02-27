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
    uint8_t const* pointers;
    uint8_t const* flags;
    uint8_t unused[3];
};
_Static_assert(sizeof(struct sprite) == 8,
               "Sprite struct size must be power of 2");

struct direction_sprite {
    uint8_t num_frames[DIRECTION_COUNT];
    uint8_t const* pointers[DIRECTION_COUNT];
    uint8_t const* flags[DIRECTION_COUNT];
};

struct animation {
    uint8_t rate;
    uint8_t current_frame;
    uint8_t sprite_frame;
    uint8_t __unused;
};

_Static_assert(sizeof(struct animation) == 4,
               "Animation struct size must be power of 2");

#endif  // SPRITE_H
