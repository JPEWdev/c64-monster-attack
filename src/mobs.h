/*
 * SPDX-License-Identifier: MIT
 */
#ifndef MOBS_H
#define MOBS_H

#include "bcd.h"
#include "sprite.h"
#include "util.h"

#define MAX_MOBS (9)
#define RESERVED_MOBS (1)
#define PLAYER_PROJECTILE_MOB_IDX (8)

#define FRAMES(f) ARRAY_SIZE(f), f

typedef void (*mob_weapon_collision_handler)(uint8_t idx, uint8_t damage,
                                             enum direction dir);
typedef void (*mob_action_handler)(uint8_t idx);
typedef void (*mob_update_handler)(uint8_t idx, uint8_t num_frames);
typedef void (*mob_mob_collision_handler)(uint8_t idx, uint8_t collision_idx);

void init_mobs(void);
void mob_set_sprite(uint8_t idx, struct sprite const* sprite);
uint8_t mob_has_sprite(uint8_t idx);
void mob_set_bb(uint8_t idx, struct bb bb);
void mob_set_position(uint8_t idx, uint16_t map_x, uint8_t map_y);
uint16_t mob_get_x(uint8_t idx);
uint8_t mob_get_y(uint8_t idx);
uint16_t mob_get_map_x(uint8_t idx);
uint8_t mob_get_map_y(uint8_t idx);
uint8_t mob_get_quad_x(uint8_t idx);
uint8_t mob_get_quad_y(uint8_t idx);
void mob_set_hp(uint8_t idx, int8_t hp);
void mob_set_color(uint8_t idx, uint8_t color);
void mob_set_damage_color(uint8_t idx, uint8_t color);
void mob_set_speed(uint8_t idx, uint8_t speed_pixels, uint8_t speed_frames);
void mob_set_target(uint8_t idx, uint16_t map_x, uint8_t map_y);
uint8_t mob_has_weapon_collision(uint8_t idx);
void mob_set_weapon_collision_handler(uint8_t idx,
                                      mob_weapon_collision_handler handler);
void mob_trigger_weapon_collision(uint8_t idx, uint8_t damage,
                                  enum direction dir);
uint8_t mob_is_hostile(uint8_t idx);
void mob_set_hostile(uint8_t idx, bool hostile);
uint8_t mob_has_player_collision(uint8_t idx);
void mob_set_player_collision_handler(uint8_t idx, mob_action_handler handler);
void mob_trigger_player_collision(uint8_t idx);
void mob_set_death_handler(uint8_t idx, mob_action_handler handler);
void mob_set_reached_target_handler(uint8_t idx, mob_action_handler handler);
void mob_set_update_handler(uint8_t idx, mob_update_handler handler);
void mob_set_animation_rate(uint8_t idx, uint8_t frames);
void mob_set_mob_collision_handler(uint8_t idx,
                                   mob_mob_collision_handler handler);

uint8_t mob_in_use(uint8_t idx);
uint8_t alloc_mob(void);
bool alloc_mob_idx(uint8_t idx);
void destroy_mob(uint8_t idx);
void destroy_all_mobs(void);
void draw_mobs(void);
void tick_mobs(void);
void update_mobs(void);
void capture_mob_collisions(void);
void damage_mob(uint8_t idx, uint8_t damage);
void damage_mob_pushback(uint8_t idx, uint8_t damage, enum direction dir);
void kill_mob(uint8_t idx);

bool check_mob_collision(uint8_t idx, uint8_t north, uint8_t south,
                         uint16_t east, uint16_t west);

uint8_t create_skeleton(uint16_t map_x, uint8_t map_y);
uint8_t create_skeleton_archer(uint16_t map_x, uint8_t map_y);
uint8_t create_coin(uint16_t map_x, uint8_t map_y, bcd_u8 value);
uint8_t create_heart(uint16_t map_x, uint8_t map_y);
void create_arrow(uint8_t idx, uint16_t map_x, uint8_t map_y,
                  enum direction direction);
enum direction arrow_get_direction(uint8_t idx);
uint8_t create_blocked_arrow(uint16_t map_x, uint8_t map_y);

#endif
