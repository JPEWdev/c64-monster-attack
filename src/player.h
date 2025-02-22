/*
 * SPDX-License-Identifier: MIT
 */
#ifndef _PLAYER_H
#define _PLAYER_H

#include <cbm.h>
#include <stdint.h>

#include "sprite.h"
#include "util.h"

enum sword_state {
    SWORD_AWAY,
    SWORD_VISIBLE,
    SWORD_ATTACKED,
};

#define SWORD_SPRITE_IDX (0)
#define PLAYER_SPRITE_IDX (1)

#define PLAYER_MAX_HEALTH (20)

#define PLAYER_COLOR (COLOR_GREEN)
#define PLAYER_HIT_COLOR_1 (COLOR_WHITE)
#define PLAYER_HIT_COLOR_2 (COLOR_CYAN)
#define PLAYER_HIT_COLOR_3 (COLOR_ORANGE)

extern uint16_t player_x;
extern uint16_t player_y;
extern enum direction player_dir;
extern uint8_t player_sword_damage;
extern uint8_t player_health;
extern uint8_t player_full_health;
extern uint8_t player_coins;
extern uint8_t player_temp_invulnerable;

extern uint8_t sword_state;
extern uint16_t sword_x;
extern uint16_t sword_y;

void init_player(void);
bool damage_player(uint8_t damage);
void damage_player_push(uint8_t damage, int8_t push_x, int8_t push_y);
void heal_player(uint8_t health);
void draw_player(void);
void update_player(void);

struct bb const* get_player_bb(void);
struct bb const* get_sword_bb(void);

#endif
