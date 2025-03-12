/*
 * SPDX-License-Identifier: MIT
 */
#ifndef _PLAYER_H
#define _PLAYER_H

#include <cbm.h>
#include <stdint.h>

#include "bcd.h"
#include "sprite.h"
#include "util.h"

enum weapon_state {
    WEAPON_AWAY,
    WEAPON_VISIBLE,
    WEAPON_ATTACKED,
};

enum weapon {
    WEAPON_SWORD,
    WEAPON_FLAIL,
    WEAPON_BOW,
};

#define WEAPON_SPRITE_IDX (0)
#define PLAYER_SPRITE_IDX (1)

#define PLAYER_MAX_HEALTH (20)
#define PLAYER_HEALTH_STR_LEN ((PLAYER_MAX_HEALTH / 2) + 1)

#define PLAYER_COLOR (COLOR_GREEN)
#define PLAYER_HIT_COLOR_1 (COLOR_WHITE)
#define PLAYER_HIT_COLOR_2 (COLOR_CYAN)
#define PLAYER_HIT_COLOR_3 (COLOR_ORANGE)

extern uint16_t player_map_x;
extern uint8_t player_map_y;
extern enum direction player_dir;
extern uint8_t player_health;
extern bool player_health_changed;
extern uint8_t player_full_health;
extern bool player_coins_changed;
extern uint8_t player_temp_invulnerable;

extern uint8_t weapon_state;

void init_player(void);
uint16_t player_get_x(void);
uint8_t player_get_y(void);
uint8_t player_get_quad_x(void);
uint8_t player_get_quad_y(void);
bcd_u16 player_get_coins(void);
void player_set_coins(bcd_u16 coins);
void player_add_coins(bcd_u16 coins);
void player_sub_coins(bcd_u16 coins);
void player_set_weapon(enum weapon weapon);
void player_get_health_str(char s[PLAYER_HEALTH_STR_LEN]);
enum weapon player_get_weapon(void);
uint16_t player_weapon_get_x(void);
uint8_t player_weapon_get_y(void);
bool damage_player(uint8_t damage);
void damage_player_push(uint8_t damage, int8_t push_x, int8_t push_y);
void heal_player(uint8_t health);
void draw_player(void);
void tick_player(void);

struct bb const* get_player_bb(void);
struct bb const* get_weapon_bb(void);

#endif
