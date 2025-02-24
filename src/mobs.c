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

extern const struct sprite_frame skeleton_0;
extern const struct sprite_frame skeleton_1;
static const struct sprite_frame* const skeleton_frames[] = {
    &skeleton_0,
    &skeleton_1,
};
const struct sprite skeleton = {FRAMES(skeleton_frames)};
extern const struct bb skeleton_bb;

extern const struct sprite_frame coin_0;
extern const struct sprite_frame coin_1;
extern const struct sprite_frame coin_2;
extern const struct sprite_frame coin_3;

static const struct sprite_frame* const coin_frames[] = {
    &coin_0,
    &coin_1,
    &coin_2,
    &coin_3,
};

const struct sprite coin = {FRAMES(coin_frames)};
extern const struct bb coin_bb;

extern const struct sprite_frame heart_0;
extern const struct sprite_frame heart_1;

static const struct sprite_frame* const heart_frames[] = {
    &heart_0,
    &heart_1,
};

const struct sprite heart = {FRAMES(heart_frames)};
extern const struct bb heart_bb;

#define MOB_SPRITE_OFFSET (2)
#define DAMAGE_PUSH (3)

static struct mob mobs[MAX_MOBS];
static uint8_t mobs_in_use = 0;
static uint8_t mob_update_idx = 0;
static struct {
    uint8_t speed_counter;
    bool needs_redraw;
    uint8_t collisions;
    bool reached_target;
    uint8_t last_update_frame;
} mob_data[MAX_MOBS];

struct mob* alloc_mob(void) {
    for (uint8_t i = 0; i < MAX_MOBS; i++) {
        if ((mobs_in_use & setbit(i)) == 0) {
            mobs_in_use |= setbit(i);
            mob_data[i].speed_counter = 1;
            mob_data[i].needs_redraw = true;
            mob_data[i].collisions = 0;
            mob_data[i].reached_target = false;
            mob_data[i].last_update_frame = frame_count;
            mobs[i].idx = i;
            return &mobs[i];
        }
    }
    return NULL;
}

void destroy_mob(struct mob* mob) {
    uint8_t idx = mob - &mobs[0];
    memset(mob, 0, sizeof(*mob));
    mobs_in_use &= clrbit(idx);
    sprite_enable_collisions(MOB_SPRITE_OFFSET + idx, false);
    hide_sprite(MOB_SPRITE_OFFSET + idx);
}

void destroy_all_mobs(void) {
    for (uint8_t i = 0; i < MAX_MOBS; i++) {
        if (mobs_in_use & setbit(i)) {
            destroy_mob(&mobs[i]);
        }
    }
}

void draw_mobs(void) {
    for (uint8_t i = 0; i < MAX_MOBS; i++) {
        uint8_t sprite_idx = i + MOB_SPRITE_OFFSET;

        if ((mobs_in_use & setbit(i)) == 0 || !mobs[i].sprite) {
            hide_sprite(sprite_idx);
            mob_data[i].needs_redraw = true;
            continue;
        }

        move_sprite(sprite_idx, mobs[i].x, mobs[i].y);

        if (sprite_is_visible(sprite_idx) && !mob_data[i].needs_redraw) {
            animate_step(sprite_idx, &mobs[i].animation, mobs[i].sprite);
        } else {
            draw_sprite(sprite_idx, mobs[i].sprite, mobs[i].color);
            sprite_enable_collisions(MOB_SPRITE_OFFSET + i, true);
            mob_data[i].needs_redraw = false;
        }

        if (mobs[i].damage_counter & 1) {
            VICII_SPRITE_COLOR[sprite_idx] = mobs[i].damage_color;
        } else {
            VICII_SPRITE_COLOR[sprite_idx] = mobs[i].color;
        }
    }
}

void update_mobs(void) {
    bool called_reached_target = false;

    for (uint8_t i = 0; i < MAX_MOBS; i++) {
        if ((mobs_in_use & setbit(i)) == 0) {
            continue;
        }
        mob_data[i].collisions = 0;

        if (mobs[i].damage_counter) {
            mobs[i].damage_counter--;
            if (mobs[i].damage_counter == 0 && mobs[i].hp <= 0) {
                kill_mob(&mobs[i]);
                continue;
            }
        }

        if (mobs[i].damage_counter &&
            (mobs[i].damage_push_x || mobs[i].damage_push_y)) {
            int8_t move_x = mobs[i].damage_push_x;
            int8_t move_y = mobs[i].damage_push_y;

            if (check_move_fast(mobs[i].x + SPRITE_WIDTH_PX / 2,
                                mobs[i].y + SPRITE_HEIGHT_PX / 2, move_x,
                                move_y)) {
                mobs[i].x += move_x;
                mobs[i].y += move_y;
            }
        } else {
            if (mobs[i].speed_pixels && (mobs[i].x != mobs[i].target_x ||
                                         mobs[i].y != mobs[i].target_y)) {
                mob_data[i].speed_counter--;
                if (mob_data[i].speed_counter == 0) {
                    mob_data[i].speed_counter = mobs[i].speed_frames;
                    if (mobs[i].x < mobs[i].target_x) {
                        mobs[i].x++;
                    } else if (mobs[i].x > mobs[i].target_x) {
                        mobs[i].x--;
                    }

                    if (mobs[i].y < mobs[i].target_y) {
                        mobs[i].y++;
                    } else if (mobs[i].y > mobs[i].target_y) {
                        mobs[i].y--;
                    }

                    if (mobs[i].x == mobs[i].target_x &&
                        mobs[i].y == mobs[i].target_y &&
                        mobs[i].on_reached_target) {
                        mob_data[i].reached_target = true;
                    }
                }
            }
        }

        if (mob_data[i].reached_target && !called_reached_target) {
            mobs[i].on_reached_target(&mobs[i]);
            mob_data[i].reached_target = false;
            called_reached_target = true;
        }
    }

    if (!called_reached_target) {
        if ((mobs_in_use & setbit(mob_update_idx)) &&
            mobs[mob_update_idx].on_update) {
            mobs[mob_update_idx].on_update(
                &mobs[mob_update_idx],
                frame_count - mob_data[mob_update_idx].last_update_frame);
            mob_data[mob_update_idx].last_update_frame = frame_count;
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
            bb_add_offset(&mobs[i].bb, mobs[i].x, mobs[i].y);

        if ((sword_collisions & mob_mask) &&
            bb16_intersect(&mob_bb, &sword_bb16)) {
            mob_data[i].collisions |= _BV(SWORD_SPRITE_IDX);
        }

        if ((player_collisions & mob_mask) &&
            bb16_intersect(&mob_bb, &player_bb16)) {
            mob_data[i].collisions |= _BV(PLAYER_SPRITE_IDX);
        }
    }
}

struct mob* check_mob_collision(uint8_t* idx, uint8_t sprite_idx) {
    while (*idx < MAX_MOBS) {
        uint8_t t = *idx;
        (*idx)++;
        if ((mobs_in_use & setbit(t)) &&
            (mob_data[t].collisions & setbit(sprite_idx))) {
            return &mobs[t];
        }
    }
    return NULL;
}

void damage_mob(struct mob* mob, uint8_t damage) {
    mob->hp -= damage;
    mob->damage_counter = 5;
    mob->damage_push_x = 0;
    mob->damage_push_y = 0;
    if (mob->hp <= 0) {
        kill_mob(mob);
    }
}

void damage_mob_pushback(struct mob* mob, uint8_t damage) {
    mob->hp -= damage;
    mob->damage_counter = 5;
    // Note if HP <= 0, mob will be killed after pushback
    switch (player_dir) {
        case NORTH:
            mob->damage_push_x = 0;
            mob->damage_push_y = -DAMAGE_PUSH;
            break;

        case SOUTH:
            mob->damage_push_x = 0;
            mob->damage_push_y = DAMAGE_PUSH;
            break;

        case EAST:
            mob->damage_push_x = DAMAGE_PUSH;
            mob->damage_push_y = 0;
            break;

        case WEST:
            mob->damage_push_x = -DAMAGE_PUSH;
            mob->damage_push_y = 0;
            break;

        default:
            mob->damage_push_x = 0;
            mob->damage_push_y = 0;
            break;
    }
}

void kill_mob(struct mob* mob) {
    if (mob->on_death) {
        mob->on_death(mob);
    } else {
        destroy_mob(mob);
    }
}

struct mob* find_mob_by_sprite_idx(uint8_t sprite_idx) {
    if (sprite_idx < MOB_SPRITE_OFFSET) {
        return NULL;
    }

    uint8_t idx = sprite_idx - MOB_SPRITE_OFFSET;
    if (mobs_in_use & setbit(idx) &&
        sprite_is_visible(idx + MOB_SPRITE_OFFSET)) {
        return &mobs[idx];
    }
    return NULL;
}

struct mob* create_skeleton(uint16_t x, uint16_t y) {
    struct mob* m = alloc_mob();
    if (!m) {
        return NULL;
    }

    m->sprite = &skeleton;
    m->bb = skeleton_bb;
    m->x = x;
    m->y = y;
    m->hp = 10;
    m->color = COLOR_WHITE;
    m->damage_color = COLOR_ORANGE;
    m->animation.rate = 60;
    m->on_sword_collision = damage_mob_pushback;
    m->speed_pixels = 1;
    m->speed_frames = 5;

    return m;
}

struct {
    uint8_t value;
    int16_t ttl;
} coin_data[MAX_MOBS];

static void coin_sword_collision(struct mob* mob, uint8_t damage) {
    player_coins += coin_data[mob->idx].value;
    kill_mob(mob);
}

static void coin_player_collision(struct mob* mob) {
    player_coins += coin_data[mob->idx].value;
    kill_mob(mob);
}

static void coin_update(struct mob* mob, uint8_t num_frames) {
    coin_data[mob->idx].ttl -= num_frames;
    if (coin_data[mob->idx].ttl < 0) {
        kill_mob(mob);
        return;
    }

    if (coin_data[mob->idx].ttl <= 120) {
        mob->sprite = mob->sprite ? NULL : &coin;
    }
}

struct mob* create_coin(uint16_t x, uint16_t y, uint8_t value) {
    struct mob* m = alloc_mob();
    if (!m) {
        return NULL;
    }
    m->sprite = &coin;
    m->bb = coin_bb;
    m->x = x;
    m->y = y;
    m->hp = 0;
    m->color = COLOR_YELLOW;
    m->animation.rate = 15;
    m->on_sword_collision = coin_sword_collision;
    m->on_player_collision = coin_player_collision;
    m->on_update = coin_update;

    coin_data[m->idx].value = value;
    coin_data[m->idx].ttl = 300;
    return m;
}

struct {
    int16_t ttl;
} heart_data[MAX_MOBS];

static void heart_sword_collision(struct mob* mob, uint8_t damage) {
    heal_player(1);
    kill_mob(mob);
}

static void heart_player_collision(struct mob* mob) {
    heal_player(1);
    kill_mob(mob);
}

static void heart_update(struct mob* mob, uint8_t num_frames) {
    heart_data[mob->idx].ttl -= num_frames;
    if (heart_data[mob->idx].ttl <= 0) {
        kill_mob(mob);
        return;
    }

    if (heart_data[mob->idx].ttl <= 120) {
        mob->sprite = mob->sprite ? NULL : &heart;
    }
}

struct mob* create_heart(uint16_t x, uint16_t y) {
    struct mob* m = alloc_mob();
    if (!m) {
        return NULL;
    }

    m->sprite = &heart;
    m->bb = heart_bb;
    m->x = x;
    m->y = y;
    m->hp = 0;
    m->color = COLOR_RED;
    m->animation.rate = 15;
    m->on_sword_collision = heart_sword_collision;
    m->on_player_collision = heart_player_collision;
    m->on_update = heart_update;

    heart_data[m->idx].ttl = 300;

    return m;
}
