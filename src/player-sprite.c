/*
 * SPDX-License-Identifier: MIT
 */
#include "player-sprite.h"

#include <cbm.h>

#include "bow_east.spm.h"
#include "bow_north.spm.h"
#include "bow_south.spm.h"
#include "bow_west.spm.h"
#include "flail.spm.h"
#include "player_attack_east.spm.h"
#include "player_attack_north.spm.h"
#include "player_attack_south.spm.h"
#include "player_attack_west.spm.h"
#include "player_east.spm.h"
#include "player_north.spm.h"
#include "player_south.spm.h"
#include "player_west.spm.h"
#include "sprite.h"
#include "sword_east.spm.h"
#include "sword_north.spm.h"
#include "sword_south.spm.h"
#include "sword_west.spm.h"
#include "util.h"

const struct direction_sprite player_sprite = {
    {
        PLAYER_NORTH_NUM_FRAMES,
        PLAYER_SOUTH_NUM_FRAMES,
        PLAYER_EAST_NUM_FRAMES,
        PLAYER_WEST_NUM_FRAMES,
    },
    {
        player_north_pointers,
        player_south_pointers,
        player_east_pointers,
        player_west_pointers,
    },
    {
        player_north_flags,
        player_south_flags,
        player_east_flags,
        player_west_flags,
    },
};

const struct direction_sprite player_attack_sprite = {
    {
        PLAYER_ATTACK_NORTH_NUM_FRAMES,
        PLAYER_ATTACK_SOUTH_NUM_FRAMES,
        PLAYER_ATTACK_EAST_NUM_FRAMES,
        PLAYER_ATTACK_WEST_NUM_FRAMES,
    },
    {
        player_attack_north_pointers,
        player_attack_south_pointers,
        player_attack_east_pointers,
        player_attack_west_pointers,
    },
    {
        player_attack_north_flags,
        player_attack_south_flags,
        player_attack_east_flags,
        player_attack_west_flags,
    },
};

const struct direction_sprite sword_sprite = {
    {
        SWORD_NORTH_NUM_FRAMES,
        SWORD_SOUTH_NUM_FRAMES,
        SWORD_EAST_NUM_FRAMES,
        SWORD_WEST_NUM_FRAMES,
    },
    {
        sword_north_pointers,
        sword_south_pointers,
        sword_east_pointers,
        sword_west_pointers,
    },
    {
        sword_north_flags,
        sword_south_flags,
        sword_east_flags,
        sword_west_flags,
    },
};

const struct sprite flail_sprite = FLAIL_SPRITE;

const struct direction_sprite bow_sprite = {
    {
        BOW_NORTH_NUM_FRAMES,
        BOW_SOUTH_NUM_FRAMES,
        BOW_EAST_NUM_FRAMES,
        BOW_WEST_NUM_FRAMES,
    },
    {
        bow_north_pointers,
        bow_south_pointers,
        bow_east_pointers,
        bow_west_pointers,
    },
    {
        bow_north_flags,
        bow_south_flags,
        bow_east_flags,
        bow_west_flags,
    },
};

