/*
 * SPDX-License-Identifier: MIT
 */
#include "mobs.h"

#include <cbm.h>
#include <stdlib.h>
#include <string.h>

#include "isr.h"
#include "map.h"
#include "move.h"
#include "player.h"
#include "reg.h"
#include "sprite.h"
#include "util.h"

#define FRAMES(f) ARRAY_SIZE(f), f

#define MOB_SPRITE_OFFSET (2)
#define DAMAGE_PUSH (3)

static uint8_t mobs_in_use = 0;
static uint8_t mob_update_idx = 0;

static struct sprite const* mobs_sprite[MAX_MOBS];
static struct bb mobs_bb[MAX_MOBS];
static uint16_t mobs_map_x[MAX_MOBS];
static uint8_t mobs_map_y[MAX_MOBS];
static int8_t mobs_hp[MAX_MOBS];
static uint8_t mobs_color[MAX_MOBS];
static uint8_t mobs_damage_color[MAX_MOBS];
static uint8_t mobs_damage_counter[MAX_MOBS];
static uint8_t mobs_speed_pixels[MAX_MOBS];
static uint8_t mobs_speed_frames[MAX_MOBS];
static uint16_t mobs_target_map_x[MAX_MOBS];
static uint8_t mobs_target_map_y[MAX_MOBS];
static int8_t mobs_damage_push_x[MAX_MOBS];
static int8_t mobs_damage_push_y[MAX_MOBS];
static struct animation mobs_animation[MAX_MOBS];
static mob_sword_collision_handler mobs_on_sword_collision[MAX_MOBS];
static mob_action_handler mobs_on_player_collision[MAX_MOBS];
static mob_action_handler mobs_on_death[MAX_MOBS];
static mob_action_handler mobs_on_reached_target[MAX_MOBS];
static mob_update_handler mobs_on_update[MAX_MOBS];
static uint8_t mobs_speed_counter[MAX_MOBS];
static bool mobs_needs_redraw[MAX_MOBS];
static uint8_t mobs_collisions[MAX_MOBS];
static bool mobs_reached_target[MAX_MOBS];
static uint8_t mobs_last_update_frame[MAX_MOBS];

void mob_set_sprite(uint8_t idx, struct sprite const* sprite) {
    mobs_sprite[idx] = sprite;
}

struct sprite const* mob_get_sprite(uint8_t idx) { return mobs_sprite[idx]; }

void mob_set_bb(uint8_t idx, struct bb bb) { mobs_bb[idx] = bb; }

void mob_set_position(uint8_t idx, uint16_t map_x, uint8_t map_y) {
    mobs_map_x[idx] = map_x;
    mobs_map_y[idx] = map_y;
}

uint16_t mob_get_x(uint8_t idx) {
    return mobs_map_x[idx] + MAP_OFFSET_X_PX - SPRITE_WIDTH_PX / 2;
}

uint16_t mob_get_y(uint8_t idx) {
    return mobs_map_y[idx] + MAP_OFFSET_Y_PX - SPRITE_HEIGHT_PX / 2;
}

uint16_t mob_get_map_x(uint8_t idx) { return mobs_map_x[idx]; }

uint8_t mob_get_map_y(uint8_t idx) { return mobs_map_y[idx]; }

uint8_t mob_get_quad_x(uint8_t idx) { return mobs_map_x[idx] / QUAD_WIDTH_PX; }

uint8_t mob_get_quad_y(uint8_t idx) { return mobs_map_y[idx] / QUAD_HEIGHT_PX; }

void mob_set_hp(uint8_t idx, int8_t hp) { mobs_hp[idx] = hp; }

void mob_set_color(uint8_t idx, uint8_t color) { mobs_color[idx] = color; }

void mob_set_damage_color(uint8_t idx, uint8_t color) {
    mobs_damage_color[idx] = color;
}

void mob_set_speed(uint8_t idx, uint8_t speed_pixels, uint8_t speed_frames) {
    mobs_speed_pixels[idx] = speed_pixels;
    mobs_speed_frames[idx] = speed_frames;
}

void mob_set_target(uint8_t idx, uint16_t map_x, uint8_t map_y) {
    mobs_target_map_x[idx] = map_x;
    mobs_target_map_y[idx] = map_y;
}

void mob_set_sword_collision_handler(uint8_t idx,
                                     mob_sword_collision_handler handler) {
    mobs_on_sword_collision[idx] = handler;
}

void mob_trigger_sword_collision(uint8_t idx, uint8_t damage) {
    if (mobs_on_sword_collision[idx]) {
        mobs_on_sword_collision[idx](idx, damage);
    }
}

void mob_set_player_collision_handler(uint8_t idx, mob_action_handler handler) {
    mobs_on_player_collision[idx] = handler;
}

void mob_trigger_player_collision(uint8_t idx) {
    if (mobs_on_player_collision[idx]) {
        mobs_on_player_collision[idx](idx);
    }
}

void mob_set_death_handler(uint8_t idx, mob_action_handler handler) {
    mobs_on_death[idx] = handler;
}

void mob_set_reached_target_handler(uint8_t idx, mob_action_handler handler) {
    mobs_on_reached_target[idx] = handler;
}

void mob_set_update_handler(uint8_t idx, mob_update_handler handler) {
    mobs_on_update[idx] = handler;
}

void mob_set_animation_rate(uint8_t idx, uint8_t frames) {
    mobs_animation[idx].rate = frames;
}

uint8_t alloc_mob(void) {
    for (uint8_t i = 0; i < MAX_MOBS; i++) {
        if ((mobs_in_use & setbit(i)) == 0) {
            mobs_in_use |= setbit(i);

            mobs_sprite[i] = NULL;
            memset(&mobs_bb[i], 0, sizeof(mobs_bb[i]));
            mobs_map_x[i] = 0;
            mobs_map_y[i] = 0;
            mobs_hp[i] = 0;
            mobs_color[i] = 0;
            mobs_damage_color[i] = 0;
            mobs_damage_counter[i] = 0;

            mobs_speed_pixels[i] = 0;
            mobs_speed_frames[i] = 0;

            mobs_target_map_x[i] = 0;
            mobs_target_map_y[i] = 0;

            mobs_damage_push_x[i] = 0;
            mobs_damage_push_y[i] = 0;

            memset(&mobs_animation[i], 0, sizeof(mobs_animation[i]));

            mobs_on_sword_collision[i] = NULL;
            mobs_on_player_collision[i] = NULL;
            mobs_on_death[i] = NULL;
            mobs_on_reached_target[i] = NULL;
            mobs_on_update[i] = NULL;

            mobs_speed_counter[i] = 1;
            mobs_needs_redraw[i] = true;
            mobs_collisions[i] = 0;
            mobs_reached_target[i] = false;
            mobs_last_update_frame[i] = frame_count;
            return i;
        }
    }
    return MAX_MOBS;
}

void destroy_mob(uint8_t idx) {
    mobs_in_use &= clrbit(idx);
    hide_sprite(MOB_SPRITE_OFFSET + idx);
}

void destroy_all_mobs(void) {
    for (uint8_t i = 0; i < MAX_MOBS; i++) {
        if (mobs_in_use & setbit(i)) {
            destroy_mob(i);
        }
    }
}

void draw_mobs(void) {
    for (uint8_t i = 0; i < MAX_MOBS; i++) {
        uint8_t sprite_idx = i + MOB_SPRITE_OFFSET;

        if ((mobs_in_use & setbit(i)) == 0 || !mobs_sprite[i]) {
            hide_sprite(sprite_idx);
            mobs_needs_redraw[i] = true;
            continue;
        }

        move_sprite(sprite_idx, mob_get_x(i), mob_get_y(i));

        if (sprite_is_visible(sprite_idx) && !mobs_needs_redraw[i]) {
            animate_step(sprite_idx, &mobs_animation[i], mobs_sprite[i]);
        } else {
            draw_sprite(sprite_idx, mobs_sprite[i], mobs_color[i]);
            mobs_needs_redraw[i] = false;
        }

        if (mobs_damage_counter[i] & 1) {
            VICII_SPRITE_COLOR[sprite_idx] = mobs_damage_color[i];
        } else {
            VICII_SPRITE_COLOR[sprite_idx] = mobs_color[i];
        }
    }
}

static bool check_mob_move(uint8_t idx, int8_t move_x, int8_t move_y) {
    if (move_x == 0 && move_y == 0) {
        return true;
    }

    if (move_x > 0) {
        if (mobs_map_x[idx] > MAP_WIDTH_PX - move_x) {
            return false;
        }
    } else if (move_x < 0) {
        if (-move_x > mobs_map_x[idx]) {
            return false;
        }
    }

    if (move_y > 0) {
        if (mobs_map_y[idx] > MAP_HEIGHT_PX - move_y) {
            return false;
        }
    } else if (move_y < 0) {
        if (-move_y > mobs_map_y[idx]) {
            return false;
        }
    }

    uint8_t new_quad_x = (mobs_map_x[idx] + move_x) / QUAD_WIDTH_PX;
    uint8_t new_quad_y = (mobs_map_y[idx] + move_y) / QUAD_HEIGHT_PX;

    return map_tile_is_passable(new_quad_x, new_quad_y);
}

void update_mobs(void) {
    bool called_reached_target = false;

    for (uint8_t i = 0; i < MAX_MOBS; i++) {
        if ((mobs_in_use & setbit(i)) == 0) {
            continue;
        }
        mobs_collisions[i] = 0;

        if (mobs_damage_counter[i]) {
            mobs_damage_counter[i]--;
            if (mobs_damage_counter[i] == 0 && mobs_hp[i] <= 0) {
                kill_mob(i);
                continue;
            }
        }

        if (mobs_damage_counter[i] &&
            (mobs_damage_push_x[i] || mobs_damage_push_y[i])) {
            int8_t move_x = mobs_damage_push_x[i];
            int8_t move_y = mobs_damage_push_y[i];

            if (check_mob_move(i, move_x, move_y)) {
                mobs_map_x[i] += move_x;
                mobs_map_y[i] += move_y;
            }
        } else {
            if (mobs_speed_pixels[i] &&
                (mobs_map_x[i] != mobs_target_map_x[i] ||
                 mobs_map_y[i] != mobs_target_map_y[i])) {
                mobs_speed_counter[i]--;
                if (mobs_speed_counter[i] == 0) {
                    mobs_speed_counter[i] = mobs_speed_frames[i];
                    if (mobs_map_x[i] < mobs_target_map_x[i]) {
                        mobs_map_x[i]++;
                    } else if (mobs_map_x[i] > mobs_target_map_x[i]) {
                        mobs_map_x[i]--;
                    }

                    if (mobs_map_y[i] < mobs_target_map_y[i]) {
                        mobs_map_y[i]++;
                    } else if (mobs_map_y[i] > mobs_target_map_y[i]) {
                        mobs_map_y[i]--;
                    }

                    if (mobs_map_x[i] == mobs_target_map_x[i] &&
                        mobs_map_y[i] == mobs_target_map_y[i] &&
                        mobs_on_reached_target[i]) {
                        mobs_reached_target[i] = true;
                    }
                }
            }
        }

        if (mobs_reached_target[i] && !called_reached_target) {
            mobs_on_reached_target[i](i);
            mobs_reached_target[i] = false;
            called_reached_target = true;
        }
    }

    if (!called_reached_target) {
        if ((mobs_in_use & setbit(mob_update_idx)) &&
            mobs_on_update[mob_update_idx]) {
            mobs_on_update[mob_update_idx](
                mob_update_idx,
                frame_count - mobs_last_update_frame[mob_update_idx]);
            mobs_last_update_frame[mob_update_idx] = frame_count;
        }

        mob_update_idx++;
        if (mob_update_idx > MAX_MOBS) {
            mob_update_idx = 0;
        }
    }
}

void capture_mob_collisions(void) {
    uint8_t sword_collisions = get_sword_collisions();
    uint8_t player_collisions = get_player_collisions();

    if (!sword_collisions && !player_collisions) {
        return;
    }

    struct bb16 sword_bb16;
    struct bb16 player_bb16;

    if (sword_collisions) {
        sword_bb16 = bb_add_offset(get_sword_bb(), sword_x, sword_y);
    }

    if (player_collisions) {
        player_bb16 =
            bb_add_offset(get_player_bb(), player_get_x(), player_get_y());
    }

    for (uint8_t i = 0; i < MAX_MOBS; i++) {
        if ((mobs_in_use & setbit(i)) == 0) {
            continue;
        }

        uint8_t mob_mask = setbit(i + MOB_SPRITE_OFFSET);
        if (((sword_collisions | player_collisions) & mob_mask) == 0) {
            continue;
        }

        const struct bb16 mob_bb =
            bb_add_offset(&mobs_bb[i], mob_get_x(i), mob_get_y(i));

        if ((sword_collisions & mob_mask) &&
            bb16_intersect(&mob_bb, &sword_bb16)) {
            mobs_collisions[i] |= _BV(SWORD_SPRITE_IDX);
        }

        if ((player_collisions & mob_mask) &&
            bb16_intersect(&mob_bb, &player_bb16)) {
            mobs_collisions[i] |= _BV(PLAYER_SPRITE_IDX);
        }
    }
}

bool check_mob_collision(uint8_t idx, uint8_t sprite_idx) {
    if ((mobs_in_use & setbit(idx)) &&
        (mobs_collisions[idx] & setbit(sprite_idx))) {
        return true;
    }
    return false;
}

void damage_mob(uint8_t idx, uint8_t damage) {
    mobs_hp[idx] -= damage;
    mobs_damage_counter[idx] = 5;
    mobs_damage_push_x[idx] = 0;
    mobs_damage_push_y[idx] = 0;
    if (mobs_hp[idx] <= 0) {
        kill_mob(idx);
    }
}

void damage_mob_pushback(uint8_t idx, uint8_t damage) {
    mobs_hp[idx] -= damage;
    mobs_damage_counter[idx] = 5;
    // Note if HP <= 0, mob will be killed after pushback
    switch (player_dir) {
        case NORTH:
            mobs_damage_push_x[idx] = 0;
            mobs_damage_push_y[idx] = -DAMAGE_PUSH;
            break;

        case SOUTH:
            mobs_damage_push_x[idx] = 0;
            mobs_damage_push_y[idx] = DAMAGE_PUSH;
            break;

        case EAST:
            mobs_damage_push_x[idx] = DAMAGE_PUSH;
            mobs_damage_push_y[idx] = 0;
            break;

        case WEST:
            mobs_damage_push_x[idx] = -DAMAGE_PUSH;
            mobs_damage_push_y[idx] = 0;
            break;

        default:
            mobs_damage_push_x[idx] = 0;
            mobs_damage_push_y[idx] = 0;
            break;
    }
}

void kill_mob(uint8_t idx) {
    if (mobs_on_death[idx]) {
        mobs_on_death[idx](idx);
    } else {
        destroy_mob(idx);
    }
}
