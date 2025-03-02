#include "arrow_east.spm.h"
#include "arrow_north.spm.h"
#include "arrow_south.spm.h"
#include "arrow_west.spm.h"
#include "mobs.h"
#include "player.h"

static const struct sprite arrow_north = ARROW_NORTH_SPRITE;
static const struct sprite arrow_south = ARROW_SOUTH_SPRITE;
static const struct sprite arrow_east = ARROW_EAST_SPRITE;
static const struct sprite arrow_west = ARROW_WEST_SPRITE;

static bool blocked_arrow_needs_created = true;

static uint8_t blocked_arrow_pointers[4];
static uint8_t blocked_arrow_flags[4];
static const struct sprite blocked_arrow = {
    4,
    blocked_arrow_pointers,
    blocked_arrow_flags,
};

static void on_reached_target(uint8_t idx) { kill_mob(idx); }

static uint8_t arrow_direction[MAX_MOBS];

enum direction arrow_get_direction(uint8_t idx) {
    return arrow_direction[idx];
}

uint8_t create_arrow(uint16_t map_x, uint8_t map_y, enum direction direction) {
    uint8_t idx = alloc_mob();
    if (idx == MAX_MOBS) {
        return MAX_MOBS;
    }

    switch (direction) {
        case NORTH:
            mob_set_sprite(idx, &arrow_north);
            mob_set_bb(idx, arrow_north_bb);
            break;

        case SOUTH:
            mob_set_sprite(idx, &arrow_south);
            mob_set_bb(idx, arrow_south_bb);
            break;

        case EAST:
            mob_set_sprite(idx, &arrow_east);
            mob_set_bb(idx, arrow_east_bb);
            break;

        case WEST:
            mob_set_sprite(idx, &arrow_west);
            mob_set_bb(idx, arrow_west_bb);
            break;
    };

    mob_set_position(idx, map_x, map_y);
    mob_set_hp(idx, 1);
    mob_set_color(idx, COLOR_ORANGE);
    mob_set_reached_target_handler(idx, on_reached_target);

    mob_set_speed(idx, 3, 1);
    arrow_direction[idx] = direction;

    return idx;
}

static uint8_t blocked_arrow_ttl[MAX_MOBS];

static void on_blocked_arrow_update(uint8_t idx, uint8_t num_frames) {
    if (num_frames >= blocked_arrow_ttl[idx]) {
        kill_mob(idx);
    } else {
        blocked_arrow_ttl[idx] -= num_frames;
    }
}

uint8_t create_blocked_arrow(uint16_t map_x, uint8_t map_y) {
    uint8_t idx = alloc_mob();
    if (idx == MAX_MOBS) {
        return MAX_MOBS;
    }

    if (blocked_arrow_needs_created) {
        blocked_arrow_pointers[0] = arrow_north_pointers[0];
        blocked_arrow_pointers[1] = arrow_east_pointers[0];
        blocked_arrow_pointers[2] = arrow_south_pointers[0];
        blocked_arrow_pointers[3] = arrow_west_pointers[0];

        blocked_arrow_flags[0] = arrow_north_flags[0];
        blocked_arrow_flags[1] = arrow_east_flags[0];
        blocked_arrow_flags[2] = arrow_south_flags[0];
        blocked_arrow_flags[3] = arrow_west_flags[0];
        blocked_arrow_needs_created = false;
    }

    mob_set_position(idx, map_x, map_y);
    mob_set_hp(idx, 1);
    mob_set_color(idx, COLOR_ORANGE);
    mob_set_sprite(idx, &blocked_arrow);
    mob_set_animation_rate(idx, 2);
    mob_set_update_handler(idx, on_blocked_arrow_update);
    blocked_arrow_ttl[idx] = 20;

    return idx;
}

