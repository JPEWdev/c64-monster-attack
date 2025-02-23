/*
 * SPDX-License-Identifier: MIT
 */
#ifndef MOBS_H
#define MOBS_H

#include "sprite.h"

#define MAX_MOBS (6)

struct mob;

typedef void (*mob_action_handler)(struct mob* mob);

struct mob {
    uint8_t idx;
    struct sprite const* sprite;
    struct bb bb;
    uint16_t x;
    uint16_t y;
    int8_t hp;
    uint8_t color;
    uint8_t damage_color;
    uint8_t damage_counter;
    uint8_t speed_pixels;
    uint8_t speed_frames;
    uint16_t target_x;
    uint16_t target_y;
    int8_t damage_push_x;
    int8_t damage_push_y;
    struct animation animation;
    void (*on_sword_collision)(struct mob* mob, uint8_t damage);
    mob_action_handler on_player_collision;
    mob_action_handler on_death;
    mob_action_handler on_reached_target;
    mob_action_handler on_update;
};

struct mob* alloc_mob(void);
void destroy_mob(struct mob* mob);
void destroy_all_mobs(void);
void draw_mobs(void);
void update_mobs(void);
void capture_mob_collisions(void);
void damage_mob(struct mob* mob, uint8_t damage);
void kill_mob(struct mob* mob);

struct mob* find_mob_by_sprite_idx(uint8_t sprite_idx);
struct mob* check_mob_collision(uint8_t* idx, uint8_t sprite_idx);

struct mob* create_skeleton(uint16_t x, uint16_t y);
struct mob* create_coin(uint16_t x, uint16_t y, uint8_t value);
struct mob* create_heart(uint16_t x, uint16_t y);

#endif
