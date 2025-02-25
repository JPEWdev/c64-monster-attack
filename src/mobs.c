/*
 * SPDX-License-Identifier: MIT
 */
#include "mobs.h"

#include <cbm.h>
#include <stdlib.h>
#include <string.h>

#include "isr.h"
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

static struct {
    struct sprite const* sprite[MAX_MOBS];
    struct bb bb[MAX_MOBS];
    uint16_t x[MAX_MOBS];
    uint16_t y[MAX_MOBS];
    int8_t hp[MAX_MOBS];
    uint8_t color[MAX_MOBS];
    uint8_t damage_color[MAX_MOBS];
    uint8_t damage_counter[MAX_MOBS];
    uint8_t speed_pixels[MAX_MOBS];
    uint8_t speed_frames[MAX_MOBS];
    uint16_t target_x[MAX_MOBS];
    uint16_t target_y[MAX_MOBS];
    int8_t damage_push_x[MAX_MOBS];
    int8_t damage_push_y[MAX_MOBS];
    struct animation animation[MAX_MOBS];
    mob_sword_collision_handler on_sword_collision[MAX_MOBS];
    mob_action_handler on_player_collision[MAX_MOBS];
    mob_action_handler on_death[MAX_MOBS];
    mob_action_handler on_reached_target[MAX_MOBS];
    mob_update_handler on_update[MAX_MOBS];

    uint8_t speed_counter[MAX_MOBS];
    bool needs_redraw[MAX_MOBS];
    uint8_t collisions[MAX_MOBS];
    bool reached_target[MAX_MOBS];
    uint8_t last_update_frame[MAX_MOBS];
} mobs;

void mob_set_sprite(uint8_t idx, struct sprite const* sprite) {
    mobs.sprite[idx] = sprite;
}

struct sprite const* mob_get_sprite(uint8_t idx) { return mobs.sprite[idx]; }

void mob_set_bb(uint8_t idx, struct bb bb) { mobs.bb[idx] = bb; }

void mob_set_position(uint8_t idx, uint16_t x, uint16_t y) {
    mobs.x[idx] = x;
    mobs.y[idx] = y;
}

uint16_t mob_get_x(uint8_t idx) { return mobs.x[idx]; }

uint16_t mob_get_y(uint8_t idx) { return mobs.y[idx]; }

void mob_set_hp(uint8_t idx, int8_t hp) { mobs.hp[idx] = hp; }

void mob_set_color(uint8_t idx, uint8_t color) { mobs.color[idx] = color; }

void mob_set_damage_color(uint8_t idx, uint8_t color) {
    mobs.damage_color[idx] = color;
}

void mob_set_speed(uint8_t idx, uint8_t speed_pixels, uint8_t speed_frames) {
    mobs.speed_pixels[idx] = speed_pixels;
    mobs.speed_frames[idx] = speed_frames;
}

void mob_set_target(uint8_t idx, uint16_t x, uint16_t y) {
    mobs.target_x[idx] = x;
    mobs.target_y[idx] = y;
}

void mob_set_sword_collision_handler(uint8_t idx,
                                     mob_sword_collision_handler handler) {
    mobs.on_sword_collision[idx] = handler;
}

void mob_trigger_sword_collision(uint8_t idx, uint8_t damage) {
    if (mobs.on_sword_collision[idx]) {
        mobs.on_sword_collision[idx](idx, damage);
    }
}

void mob_set_player_collision_handler(uint8_t idx, mob_action_handler handler) {
    mobs.on_player_collision[idx] = handler;
}

void mob_trigger_player_collision(uint8_t idx) {
    if (mobs.on_player_collision[idx]) {
        mobs.on_player_collision[idx](idx);
    }
}

void mob_set_death_handler(uint8_t idx, mob_action_handler handler) {
    mobs.on_death[idx] = handler;
}

void mob_set_reached_target_handler(uint8_t idx, mob_action_handler handler) {
    mobs.on_reached_target[idx] = handler;
}

void mob_set_update_handler(uint8_t idx, mob_update_handler handler) {
    mobs.on_update[idx] = handler;
}

void mob_set_animation_rate(uint8_t idx, uint8_t frames) {
    mobs.animation[idx].rate = frames;
}

uint8_t alloc_mob(void) {
    for (uint8_t i = 0; i < MAX_MOBS; i++) {
        if ((mobs_in_use & setbit(i)) == 0) {
            mobs_in_use |= setbit(i);

            mobs.sprite[i] = NULL;
            memset(&mobs.bb[i], 0, sizeof(mobs.bb[i]));
            mobs.x[i] = 0;
            mobs.y[i] = 0;
            mobs.hp[i] = 0;
            mobs.color[i] = 0;
            mobs.damage_color[i] = 0;
            mobs.damage_counter[i] = 0;

            mobs.speed_pixels[i] = 0;
            mobs.speed_frames[i] = 0;

            mobs.target_x[i] = 0;
            mobs.target_y[i] = 0;

            mobs.damage_push_x[i] = 0;
            mobs.damage_push_y[i] = 0;

            memset(&mobs.animation[i], 0, sizeof(mobs.animation[i]));

            mobs.on_sword_collision[i] = NULL;
            mobs.on_player_collision[i] = NULL;
            mobs.on_death[i] = NULL;
            mobs.on_reached_target[i] = NULL;
            mobs.on_update[i] = NULL;

            mobs.speed_counter[i] = 1;
            mobs.needs_redraw[i] = true;
            mobs.collisions[i] = 0;
            mobs.reached_target[i] = false;
            mobs.last_update_frame[i] = frame_count;
            return i;
        }
    }
    return MAX_MOBS;
}

void destroy_mob(uint8_t idx) {
    mobs_in_use &= clrbit(idx);
    sprite_enable_collisions(MOB_SPRITE_OFFSET + idx, false);
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

        if ((mobs_in_use & setbit(i)) == 0 || !mobs.sprite[i]) {
            hide_sprite(sprite_idx);
            mobs.needs_redraw[i] = true;
            continue;
        }

        move_sprite(sprite_idx, mobs.x[i], mobs.y[i]);

        if (sprite_is_visible(sprite_idx) && !mobs.needs_redraw[i]) {
            animate_step(sprite_idx, &mobs.animation[i], mobs.sprite[i]);
        } else {
            draw_sprite(sprite_idx, mobs.sprite[i], mobs.color[i]);
            sprite_enable_collisions(MOB_SPRITE_OFFSET + i, true);
            mobs.needs_redraw[i] = false;
        }

        if (mobs.damage_counter[i] & 1) {
            VICII_SPRITE_COLOR[sprite_idx] = mobs.damage_color[i];
        } else {
            VICII_SPRITE_COLOR[sprite_idx] = mobs.color[i];
        }
    }
}

void update_mobs(void) {
    bool called_reached_target = false;

    for (uint8_t i = 0; i < MAX_MOBS; i++) {
        if ((mobs_in_use & setbit(i)) == 0) {
            continue;
        }
        mobs.collisions[i] = 0;

        if (mobs.damage_counter[i]) {
            mobs.damage_counter[i]--;
            if (mobs.damage_counter[i] == 0 && mobs.hp[i] <= 0) {
                kill_mob(i);
                continue;
            }
        }

        if (mobs.damage_counter[i] &&
            (mobs.damage_push_x[i] || mobs.damage_push_y[i])) {
            int8_t move_x = mobs.damage_push_x[i];
            int8_t move_y = mobs.damage_push_y[i];

            if (check_move_fast(mobs.x[i] + SPRITE_WIDTH_PX / 2,
                                mobs.y[i] + SPRITE_HEIGHT_PX / 2, move_x,
                                move_y)) {
                mobs.x[i] += move_x;
                mobs.y[i] += move_y;
            }
        } else {
            if (mobs.speed_pixels[i] && (mobs.x[i] != mobs.target_x[i] ||
                                         mobs.y[i] != mobs.target_y[i])) {
                mobs.speed_counter[i]--;
                if (mobs.speed_counter[i] == 0) {
                    mobs.speed_counter[i] = mobs.speed_frames[i];
                    if (mobs.x[i] < mobs.target_x[i]) {
                        mobs.x[i]++;
                    } else if (mobs.x[i] > mobs.target_x[i]) {
                        mobs.x[i]--;
                    }

                    if (mobs.y[i] < mobs.target_y[i]) {
                        mobs.y[i]++;
                    } else if (mobs.y[i] > mobs.target_y[i]) {
                        mobs.y[i]--;
                    }

                    if (mobs.x[i] == mobs.target_x[i] &&
                        mobs.y[i] == mobs.target_y[i] &&
                        mobs.on_reached_target[i]) {
                        mobs.reached_target[i] = true;
                    }
                }
            }
        }

        if (mobs.reached_target[i] && !called_reached_target) {
            mobs.on_reached_target[i](i);
            mobs.reached_target[i] = false;
            called_reached_target = true;
        }
    }

    if (!called_reached_target) {
        if ((mobs_in_use & setbit(mob_update_idx)) &&
            mobs.on_update[mob_update_idx]) {
            mobs.on_update[mob_update_idx](
                mob_update_idx,
                frame_count - mobs.last_update_frame[mob_update_idx]);
            mobs.last_update_frame[mob_update_idx] = frame_count;
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
        player_bb16 = bb_add_offset(get_player_bb(), player_x, player_y);
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
            bb_add_offset(&mobs.bb[i], mobs.x[i], mobs.y[i]);

        if ((sword_collisions & mob_mask) &&
            bb16_intersect(&mob_bb, &sword_bb16)) {
            mobs.collisions[i] |= _BV(SWORD_SPRITE_IDX);
        }

        if ((player_collisions & mob_mask) &&
            bb16_intersect(&mob_bb, &player_bb16)) {
            mobs.collisions[i] |= _BV(PLAYER_SPRITE_IDX);
        }
    }
}

bool check_mob_collision(uint8_t idx, uint8_t sprite_idx) {
    if ((mobs_in_use & setbit(idx)) &&
        (mobs.collisions[idx] & setbit(sprite_idx))) {
        return true;
    }
    return false;
}

void damage_mob(uint8_t idx, uint8_t damage) {
    mobs.hp[idx] -= damage;
    mobs.damage_counter[idx] = 5;
    mobs.damage_push_x[idx] = 0;
    mobs.damage_push_y[idx] = 0;
    if (mobs.hp[idx] <= 0) {
        kill_mob(idx);
    }
}

void damage_mob_pushback(uint8_t idx, uint8_t damage) {
    mobs.hp[idx] -= damage;
    mobs.damage_counter[idx] = 5;
    // Note if HP <= 0, mob will be killed after pushback
    switch (player_dir) {
        case NORTH:
            mobs.damage_push_x[idx] = 0;
            mobs.damage_push_y[idx] = -DAMAGE_PUSH;
            break;

        case SOUTH:
            mobs.damage_push_x[idx] = 0;
            mobs.damage_push_y[idx] = DAMAGE_PUSH;
            break;

        case EAST:
            mobs.damage_push_x[idx] = DAMAGE_PUSH;
            mobs.damage_push_y[idx] = 0;
            break;

        case WEST:
            mobs.damage_push_x[idx] = -DAMAGE_PUSH;
            mobs.damage_push_y[idx] = 0;
            break;

        default:
            mobs.damage_push_x[idx] = 0;
            mobs.damage_push_y[idx] = 0;
            break;
    }
}

void kill_mob(uint8_t idx) {
    if (mobs.on_death[idx]) {
        mobs.on_death[idx](idx);
    } else {
        destroy_mob(idx);
    }
}
