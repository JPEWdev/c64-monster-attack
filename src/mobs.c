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

#define MOB_FLAG_IN_USE _BV(0)
#define MOB_FLAG_REACHED_TARGET _BV(1)
#define MOB_FLAG_HAS_SPRITE _BV(2)

#define mob_check_flag(idx, _flag) (mobs_flags[idx] & MOB_FLAG_##_flag)
#define mob_set_flag(idx, _flag) (mobs_flags[idx] |= MOB_FLAG_##_flag)
#define mob_clr_flag(idx, _flag) (mobs_flags[idx] &= ~(MOB_FLAG_##_flag))

#define MOB_HANDLER_FLAG_PLAYER_COLLISION _BV(0)
#define MOB_HANDLER_FLAG_SWORD_COLLISION _BV(1)
#define MOB_HANDLER_FLAG_DEATH _BV(2)
#define MOB_HANDLER_FLAG_REACHED_TARGET _BV(3)
#define MOB_HANDLER_FLAG_UPDATE _BV(4)

#define mob_check_handler_flag(idx, _handler) \
    (mobs_handler_flags[idx] & MOB_HANDLER_FLAG_##_handler)
#define mob_set_handler_flag(idx, _handler) \
    (mobs_handler_flags[idx] |= MOB_HANDLER_FLAG_##_handler)
#define mob_clr_handler_flag(idx, _handler) \
    (mobs_handler_flags[idx] &= ~(MOB_HANDLER_FLAG_##_handler))

static uint8_t mob_update_idx = 0;

static uint8_t mobs_flags[MAX_MOBS] = {0};
static uint8_t mobs_handler_flags[MAX_MOBS];
static struct sprite const* mobs_sprite[MAX_MOBS];
static uint8_t mobs_bb_north[MAX_MOBS];
static uint8_t mobs_bb_south[MAX_MOBS];
static uint8_t mobs_bb_east[MAX_MOBS];
static uint8_t mobs_bb_west[MAX_MOBS];

static uint8_t mobs_bb16_north[MAX_MOBS];
static uint8_t mobs_bb16_south[MAX_MOBS];
static uint16_t mobs_bb16_east[MAX_MOBS];
static uint16_t mobs_bb16_west[MAX_MOBS];

static uint16_t mobs_map_x[MAX_MOBS];
static uint8_t mobs_map_y[MAX_MOBS];
static uint8_t mobs_bot_y[MAX_MOBS];
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
static uint8_t mobs_sprite_frame[MAX_MOBS];
static uint8_t mobs_animation_rate[MAX_MOBS];
static uint8_t mobs_animation_count[MAX_MOBS];
static mob_sword_collision_handler mobs_on_sword_collision[MAX_MOBS];
static mob_action_handler mobs_on_player_collision[MAX_MOBS];
static mob_action_handler mobs_on_death[MAX_MOBS];
static mob_action_handler mobs_on_reached_target[MAX_MOBS];
static mob_update_handler mobs_on_update[MAX_MOBS];
static uint8_t mobs_speed_counter[MAX_MOBS];
static uint8_t mobs_last_update_frame[MAX_MOBS];
static uint8_t mobs_raster_idx[MAX_MOBS];

static uint8_t mob_idx_by_y[MAX_MOBS];

void init_mobs(void) {
    for (uint8_t i = 0; i < MAX_MOBS; i++) {
        mobs_bot_y[i] = 0xFF;
        mob_idx_by_y[i] = i;
    }
}

static void set_bot_y(uint8_t idx) {
    if (mob_check_flag(idx, HAS_SPRITE)) {
        mobs_bot_y[idx] = mob_get_y(idx) + mobs_bb_south[idx];
    } else {
        mobs_bot_y[idx] = 0xFF;
    }
}

static void mob_calc_bb16(uint8_t idx) {
    mobs_bb16_north[idx] = mob_get_y(idx) + mobs_bb_north[idx];
    mobs_bb16_south[idx] = mob_get_y(idx) + mobs_bb_south[idx];
    mobs_bb16_west[idx] = mob_get_x(idx) + mobs_bb_west[idx];
    mobs_bb16_east[idx] = mob_get_x(idx) + mobs_bb_east[idx];
}

// Ocean sort method
static void sort_mobs_y(void) {
    for (uint8_t i = 0; i < MAX_MOBS - 1; i++) {
        if (mobs_bot_y[mob_idx_by_y[i + 1]] < mobs_bot_y[mob_idx_by_y[i]]) {
            uint8_t j = i;
            while (true) {
                swap(mob_idx_by_y[j], mob_idx_by_y[j + 1]);
                if (j == 0) {
                    break;
                }
                j--;
                if (mobs_bot_y[mob_idx_by_y[j + 1]] >=
                    mobs_bot_y[mob_idx_by_y[j]]) {
                    break;
                }
            }
        }
    }
}

void mob_set_sprite(uint8_t idx, struct sprite const* sprite) {
    mobs_sprite[idx] = sprite;
    if (sprite) {
        mob_set_flag(idx, HAS_SPRITE);
    } else {
        mob_clr_flag(idx, HAS_SPRITE);
    }
    mobs_sprite_frame[idx] = 0;
    set_bot_y(idx);
}

uint8_t mob_has_sprite(uint8_t idx) { return mob_check_flag(idx, HAS_SPRITE); }

void mob_set_bb(uint8_t idx, struct bb bb) {
    mobs_bb_north[idx] = bb.north;
    mobs_bb_south[idx] = bb.south;
    mobs_bb_east[idx] = bb.east;
    mobs_bb_west[idx] = bb.west;

    mob_calc_bb16(idx);
}

void mob_set_position(uint8_t idx, uint16_t map_x, uint8_t map_y) {
    mobs_map_x[idx] = map_x;
    mobs_map_y[idx] = map_y;
    set_bot_y(idx);
    mob_calc_bb16(idx);
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
    if (handler) {
        mob_set_handler_flag(idx, SWORD_COLLISION);
    } else {
        mob_clr_handler_flag(idx, SWORD_COLLISION);
    }
}

void mob_trigger_sword_collision(uint8_t idx, uint8_t damage) {
    if (mob_check_handler_flag(idx, SWORD_COLLISION)) {
        mobs_on_sword_collision[idx](idx, damage);
    }
}

void mob_set_player_collision_handler(uint8_t idx, mob_action_handler handler) {
    mobs_on_player_collision[idx] = handler;
    if (handler) {
        mob_set_handler_flag(idx, PLAYER_COLLISION);
    } else {
        mob_clr_handler_flag(idx, PLAYER_COLLISION);
    }
}

void mob_trigger_player_collision(uint8_t idx) {
    if (mob_check_handler_flag(idx, PLAYER_COLLISION)) {
        mobs_on_player_collision[idx](idx);
    }
}

void mob_set_death_handler(uint8_t idx, mob_action_handler handler) {
    mobs_on_death[idx] = handler;
    if (handler) {
        mob_set_handler_flag(idx, DEATH);
    } else {
        mob_clr_handler_flag(idx, DEATH);
    }
}

void mob_set_reached_target_handler(uint8_t idx, mob_action_handler handler) {
    mobs_on_reached_target[idx] = handler;
    if (handler) {
        mob_set_handler_flag(idx, REACHED_TARGET);
    } else {
        mob_clr_handler_flag(idx, REACHED_TARGET);
    }
}

void mob_set_update_handler(uint8_t idx, mob_update_handler handler) {
    mobs_on_update[idx] = handler;
    if (handler) {
        mob_set_handler_flag(idx, UPDATE);
    } else {
        mob_clr_handler_flag(idx, UPDATE);
    }
}

void mob_set_animation_rate(uint8_t idx, uint8_t frames) {
    mobs_animation_rate[idx] = frames;
}

static void mob_inc_x(uint8_t idx, int8_t delta) {
    mobs_map_x[idx] += delta;
    mobs_bb16_east[idx] += delta;
    mobs_bb16_west[idx] += delta;
}

static void mob_inc_y(uint8_t idx, int8_t delta) {
    mobs_map_y[idx] += delta;
    mobs_bb16_north[idx] += delta;
    mobs_bb16_south[idx] += delta;
}

uint8_t alloc_mob(void) {
    for (uint8_t i = 0; i < MAX_MOBS; i++) {
        if (!mob_check_flag(i, IN_USE)) {
            mobs_flags[i] = MOB_FLAG_IN_USE;
            mobs_handler_flags[i] = 0;

            mobs_sprite[i] = NULL;
            mobs_bb_north[i] = 0;
            mobs_bb_south[i] = SPRITE_HEIGHT_PX - 1;
            mobs_bb_east[i] = SPRITE_WIDTH_PX - 1;
            mobs_bb_west[i] = 0;
            mobs_map_x[i] = 0;
            mobs_map_y[i] = 0;
            mobs_bot_y[i] = 0xFF;
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

            mobs_sprite_frame[i] = 0;

            mobs_on_sword_collision[i] = NULL;
            mobs_on_player_collision[i] = NULL;
            mobs_on_death[i] = NULL;
            mobs_on_reached_target[i] = NULL;
            mobs_on_update[i] = NULL;

            mobs_speed_counter[i] = 1;
            mobs_last_update_frame[i] = frame_count;
            mob_calc_bb16(i);
            return i;
        }
    }
    return MAX_MOBS;
}

void destroy_mob(uint8_t idx) {
    mob_clr_flag(idx, IN_USE);
    // Set the Y position to max value so this sprite sorts to the end of the
    // list
    mobs_bot_y[idx] = 0xFF;
}

void destroy_all_mobs(void) {
    for (uint8_t i = 0; i < MAX_MOBS; i++) {
        if (mob_check_flag(i, IN_USE)) {
            destroy_mob(i);
        }
    }
}

static void animate_mob(uint8_t idx) {
    if (mobs_animation_count[idx] == 0) {
        mobs_sprite_frame[idx]++;
        mobs_animation_count[idx] = mobs_animation_rate[idx];
    } else {
        mobs_animation_count[idx]--;
    }

    if (mobs_sprite_frame[idx] >= mobs_sprite[idx]->num_frames) {
        mobs_sprite_frame[idx] = 0;
    }
}

void draw_mobs(void) {
    uint8_t sprite_enable = VICII_SPRITE_ENABLE & _BMASK(MOB_SPRITE_OFFSET);
    uint8_t sprite_msb = VICII_SPRITE_MSB & _BMASK(MOB_SPRITE_OFFSET);
    uint8_t sprite_y_expand = VICII_SPRITE_Y_EXPAND & _BMASK(MOB_SPRITE_OFFSET);
    uint8_t sprite_x_expand = VICII_SPRITE_X_EXPAND & _BMASK(MOB_SPRITE_OFFSET);
    uint8_t sprite_multicolor =
        VICII_SPRITE_MULTICOLOR & _BMASK(MOB_SPRITE_OFFSET);

    // Initial drawn sprites
    uint8_t sprite_idx = MOB_SPRITE_OFFSET;
    uint8_t y_idx = 0;
    for (; y_idx < MAX_MOBS && sprite_idx < 8; y_idx++, sprite_idx++) {
        uint8_t mob_idx = mob_idx_by_y[y_idx];
        if (mobs_bot_y[mob_idx] == 0xFF) {
            // As soon as we see an invalid Y coordinate, we are done.
            break;
        }

        sprite_enable |= setbit(sprite_idx);

        if (mob_get_x(mob_idx) & 0x0100) {
            sprite_msb |= setbit(sprite_idx);
        }

        VICII_SPRITE_POSITION[sprite_idx].x = mob_get_x(mob_idx) & 0xFF;
        VICII_SPRITE_POSITION[sprite_idx].y = mob_get_y(mob_idx);

        if (mobs_damage_counter[mob_idx] & 1) {
            VICII_SPRITE_COLOR[sprite_idx] = mobs_damage_color[mob_idx];
        } else {
            VICII_SPRITE_COLOR[sprite_idx] = mobs_color[mob_idx];
        }

        uint8_t frame = mobs_sprite_frame[mob_idx];

        sprite_pointers_shadow[sprite_idx] =
            mobs_sprite[mob_idx]->pointers[frame];

        uint8_t flags = mobs_sprite[mob_idx]->flags[frame];

        if (flags & SPRITE_IMAGE_EXPAND_Y) {
            sprite_y_expand |= setbit(sprite_idx);
        }

        if (flags & SPRITE_IMAGE_EXPAND_X) {
            sprite_x_expand |= setbit(sprite_idx);
        }

        if (flags & SPRITE_IMAGE_MULTICOLOR) {
            sprite_multicolor |= setbit(sprite_idx);
        }
        mobs_raster_idx[mob_idx] = 0xFF;
    }

    VICII_SPRITE_ENABLE = sprite_enable;
    VICII_SPRITE_MSB = sprite_msb;
    VICII_SPRITE_Y_EXPAND = sprite_y_expand;
    VICII_SPRITE_X_EXPAND = sprite_x_expand;
    VICII_SPRITE_MULTICOLOR = sprite_multicolor;

    // Multiplexed sprites
    for (; y_idx < MAX_MOBS; y_idx++, sprite_idx++) {
        uint8_t mob_idx = mob_idx_by_y[y_idx];
        if (mobs_bot_y[mob_idx] == 0xFF) {
            // As soon as we see an invalid Y coordinate, we are done.
            break;
        }

        if (sprite_idx >= 8) {
            sprite_idx = MOB_SPRITE_OFFSET;
        }

        uint8_t const mask = setbit(sprite_idx);

        uint8_t color = (mobs_damage_counter[mob_idx] & 1)
                            ? mobs_damage_color[mob_idx]
                            : mobs_color[mob_idx];

        uint8_t frame = mobs_sprite_frame[mob_idx];

        if (mob_get_x(mob_idx) & 0x0100) {
            sprite_msb |= mask;
        } else {
            sprite_msb &= ~mask;
        }

        uint8_t flags = mobs_sprite[mob_idx]->flags[frame];

        if (flags & SPRITE_IMAGE_EXPAND_Y) {
            sprite_y_expand |= mask;
        } else {
            sprite_y_expand &= ~mask;
        }

        if (flags & SPRITE_IMAGE_EXPAND_X) {
            sprite_x_expand |= mask;
        } else {
            sprite_x_expand &= ~mask;
        }

        if (flags & SPRITE_IMAGE_MULTICOLOR) {
            sprite_multicolor |= mask;
        } else {
            sprite_multicolor &= ~mask;
        }

        // Note: +2 is required because the bottom y is the bottom of the
        // bounding box (not the distance from the y coordinate), and the
        // raster interrupt is triggered at the start of the raster line
        mobs_raster_idx[mob_idx] = alloc_raster_cmd(
            mobs_bot_y[mob_idx_by_y[y_idx - (8 - MOB_SPRITE_OFFSET)]] + 2);

        raster_set_sprite(mobs_raster_idx[mob_idx], sprite_idx,
                          mobs_sprite[mob_idx]->pointers[frame],
                          mob_get_x(mob_idx) & 0xFF, mob_get_y(mob_idx), color,
                          sprite_msb, sprite_x_expand, sprite_y_expand,
                          sprite_multicolor);
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

        // Prevent mob bottom coordinate from going at or past 255
        if (mob_get_y(idx) + SPRITE_HEIGHT_PX >= 255 - move_y) {
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
        if (!mob_check_flag(i, IN_USE)) {
            continue;
        }

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
                mob_inc_x(i, move_x);
                mob_inc_y(i, move_y);
            }
        } else {
            if (mobs_speed_pixels[i] &&
                (mobs_map_x[i] != mobs_target_map_x[i] ||
                 mobs_map_y[i] != mobs_target_map_y[i])) {
                mobs_speed_counter[i]--;
                if (mobs_speed_counter[i] == 0) {
                    mobs_speed_counter[i] = mobs_speed_frames[i];
                    if (mobs_map_x[i] < mobs_target_map_x[i]) {
                        mob_inc_x(i, 1);
                    } else if (mobs_map_x[i] > mobs_target_map_x[i]) {
                        mob_inc_x(i, -1);
                    }

                    if (mobs_map_y[i] < mobs_target_map_y[i]) {
                        mob_inc_y(i, 1);
                    } else if (mobs_map_y[i] > mobs_target_map_y[i]) {
                        mob_inc_y(i, -1);
                    }

                    if (mobs_map_x[i] == mobs_target_map_x[i] &&
                        mobs_map_y[i] == mobs_target_map_y[i]) {
                        mob_set_flag(i, REACHED_TARGET);
                    }
                }
            }
        }

        set_bot_y(i);

        if (mob_check_flag(i, REACHED_TARGET) && !called_reached_target) {
            if (mob_check_handler_flag(i, REACHED_TARGET)) {
                mobs_on_reached_target[i](i);
                called_reached_target = true;
            }
            mob_clr_flag(i, REACHED_TARGET);
        }

        animate_mob(i);
    }

    if (!called_reached_target) {
        if (mob_check_flag(mob_update_idx, IN_USE) &&
            mob_check_handler_flag(mob_update_idx, UPDATE)) {
            mobs_on_update[mob_update_idx](
                mob_update_idx,
                frame_count - mobs_last_update_frame[mob_update_idx]);
            mobs_last_update_frame[mob_update_idx] = frame_count;
        }

        mob_update_idx++;
        if (mob_update_idx >= MAX_MOBS) {
            mob_update_idx = 0;
        }
    }

    // If not all MOBs were drawn, set some mobs with lower Y values (those
    // guaranteed to be drawn) to invalid. This will prevent them from drawing,
    // allowing the missing sprites to move up to a guaranteed slot. This
    // effectively "flicker plexs" the sprites as a backup if they are bunched
    // too close together (and by extension if there are more than 8 sprites on
    // a raster line).
    //
    // The sprites with the lowest Y values should still be drawn so that the
    // interrupts to draw later sprites (if any) are still done. Fortunately,
    // its easy to deduce that the number of missed sprites is the same number
    // that should be kept at the beginning.
    //
    // Note that a new sprite bottom will be recalculated on the next frame in
    // the loop above, overriding this
    //
    // This isn't a 100% ideal solution, but it is a fast calculation
    for (uint8_t i = 0; i < last_num_missed_sprites; i++) {
        mobs_bot_y[mob_idx_by_y[i + last_num_missed_sprites]] = 0xFF;
    }

    sort_mobs_y();
}

bool check_mob_collision(uint8_t idx, struct bb16 const bb) {
    if (mob_check_flag(idx, IN_USE)) {
        if (mobs_raster_idx[idx] != 0xFF &&
            last_missed_sprite[mobs_raster_idx[idx]]) {
            return false;
        }

        return (mobs_bb16_north[idx] <= bb.south &&
                bb.north <= mobs_bb16_south[idx] &&
                mobs_bb16_west[idx] <= bb.east &&
                bb.west <= mobs_bb16_east[idx]);
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
    if (mob_check_handler_flag(idx, DEATH)) {
        mobs_on_death[idx](idx);
    } else {
        destroy_mob(idx);
    }
}
