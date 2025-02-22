/*
 * SPDX-License-Identifier: MIT
 */
#include "player-sprite.h"

#include <cbm.h>

#include "sprite.h"
#include "util.h"

#define PLAYER_DIR_SPRITE(dir)                                          \
    extern const struct sprite_frame player_##dir##_0;                  \
    extern const struct sprite_frame player_##dir##_1;                  \
    extern const struct sprite_frame player_##dir##_2;                  \
                                                                        \
    static const struct sprite_frame* const player_##dir##_frames[] = { \
        &player_##dir##_0,                                              \
        &player_##dir##_1,                                              \
        &player_##dir##_0,                                              \
        &player_##dir##_2,                                              \
    };                                                                  \
                                                                        \
    static const struct sprite player_##dir = {                         \
        4,                                                              \
        player_##dir##_frames,                                          \
    }

PLAYER_DIR_SPRITE(north);
PLAYER_DIR_SPRITE(south);
PLAYER_DIR_SPRITE(east);
PLAYER_DIR_SPRITE(west);

const struct direction_sprite player_sprite = {
    {
        &player_north,
        &player_south,
        &player_east,
        &player_west,
    },
};

#define SINGLE_SPRITE(name)                                     \
    extern const struct sprite_frame name##_0;                  \
                                                                \
    static const struct sprite_frame* const name##_frames[] = { \
        &name##_0,                                              \
    };                                                          \
                                                                \
    static const struct sprite name = {                         \
        1,                                                      \
        name##_frames,                                          \
    }

PLAYER_DIR_SPRITE(attack_north);
PLAYER_DIR_SPRITE(attack_south);
PLAYER_DIR_SPRITE(attack_east);
PLAYER_DIR_SPRITE(attack_west);

const struct direction_sprite player_attack_sprite = {
    {
        &player_attack_north,
        &player_attack_south,
        &player_attack_east,
        &player_attack_west,
    },
};

SINGLE_SPRITE(sword_north);
SINGLE_SPRITE(sword_south);
SINGLE_SPRITE(sword_east);
SINGLE_SPRITE(sword_west);

const struct direction_sprite sword_sprite = {
    {
        &sword_north,
        &sword_south,
        &sword_east,
        &sword_west,
    },
};

