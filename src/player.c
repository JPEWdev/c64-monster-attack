/*
 * SPDX-License-Identifier: MIT
 */
#include "player.h"

#include <stdlib.h>

#include "bcd.h"
#include "input.h"
#include "map.h"
#include "mobs.h"
#include "move.h"
#include "player-sprite.h"
#include "reg.h"
#include "tick.h"
#include "trigconst.h"

#define PLAYER_SPEED (2)

#define PLAYER_ANIMATION_RATE (10)
#define WEAPON_ANIMATION_RATE (10)

uint16_t player_map_x;
uint8_t player_map_y;
enum direction player_dir = SOUTH;
uint8_t player_weapon_damage;
uint8_t player_health;
bool player_health_changed;
uint8_t player_full_health;
uint8_t player_temp_invulnerable;
static int8_t player_push_x;
static int8_t player_push_y;
static uint8_t player_push_timer;
static uint8_t player_frame;
static uint8_t player_animation_counter;
static uint8_t weapon_frame;
static uint8_t weapon_animation_counter;

static bcd_u16 player_coins;
bool player_coins_changed;

uint8_t weapon_state;
static enum weapon current_weapon;
static uint16_t weapon_x = 0;
static uint8_t weapon_y = 0;
static uint8_t weapon_move_counter = 0;
static uint8_t flail_timer;
static uint8_t flail_damage;

static const struct {
    int8_t x;
    int8_t y;
} sword_offset[] = {
    /* NORTH */ {10, -14},
    /* SOUTH */ {-8, 15},
    /* EAST */ {18, 2},
    /* WEST */ {-18, 2},
};

#define FLAIL_R (35)

// A half sine wave describing the position of the flail relative to the
// player. The other half is derived by wrapping and subtracting values, and
// the cosine is derived by adding an offset of half the length
static const int8_t flail_table[] = {
    SIN_0 * FLAIL_R,  SIN_5* FLAIL_R,   SIN_10* FLAIL_R,  SIN_15* FLAIL_R,
    SIN_20* FLAIL_R,  SIN_25* FLAIL_R,  SIN_30* FLAIL_R,  SIN_35* FLAIL_R,
    SIN_40* FLAIL_R,  SIN_45* FLAIL_R,  SIN_50* FLAIL_R,  SIN_55* FLAIL_R,
    SIN_60* FLAIL_R,  SIN_65* FLAIL_R,  SIN_70* FLAIL_R,  SIN_75* FLAIL_R,
    SIN_80* FLAIL_R,  SIN_85* FLAIL_R,  SIN_90* FLAIL_R,  SIN_95* FLAIL_R,
    SIN_100* FLAIL_R, SIN_105* FLAIL_R, SIN_110* FLAIL_R, SIN_115* FLAIL_R,
    SIN_120* FLAIL_R, SIN_125* FLAIL_R, SIN_130* FLAIL_R, SIN_135* FLAIL_R,
    SIN_140* FLAIL_R, SIN_145* FLAIL_R, SIN_150* FLAIL_R, SIN_155* FLAIL_R,
    SIN_160* FLAIL_R, SIN_165* FLAIL_R, SIN_170* FLAIL_R, SIN_175* FLAIL_R,
};

void init_player(void) {
    player_push_x = 0;
    player_push_y = 0;
    player_push_timer = 0;
    weapon_state = WEAPON_AWAY;
    current_weapon = WEAPON_SWORD;
    player_temp_invulnerable = 0;
    player_frame = 0;
    weapon_frame = 0;
    weapon_move_counter = 0;
}

bool damage_player(uint8_t damage) {
    if (player_temp_invulnerable) {
        return false;
    }

    // Prevent entering the invulnerable state if health was previoulsy reduced
    // to zero
    if (player_health == 0) {
        return true;
    }

    if (damage > player_health) {
        player_health = 0;
    } else {
        player_health -= damage;
    }
    player_health_changed = true;

    player_temp_invulnerable = 30;
    return true;
}

void damage_player_push(uint8_t damage, int8_t push_x, int8_t push_y) {
    if (!damage_player(damage)) {
        return;
    }

    player_push_x = push_x;
    player_push_y = push_y;
    player_push_timer = 15;
}

void heal_player(uint8_t health) {
    player_health += health;
    if (player_health > player_full_health) {
        player_health = player_full_health;
    }
    player_health_changed = true;
}

uint16_t player_get_x(void) {
    return player_map_x + MAP_OFFSET_X_PX - SPRITE_WIDTH_PX / 2;
}

uint8_t player_get_y(void) {
    return player_map_y + MAP_OFFSET_Y_PX - SPRITE_HEIGHT_PX / 2;
}

uint8_t player_get_quad_x(void) { return player_map_x / QUAD_WIDTH_PX; }

uint8_t player_get_quad_y(void) { return player_map_y / QUAD_HEIGHT_PX; }

bcd_u16 player_get_coins(void) { return player_coins; }

void player_set_coins(bcd_u16 coins) {
    player_coins = coins;
    player_coins_changed = true;
}

void player_add_coins(bcd_u16 coins) {
    if (!coins) {
        return;
    }
    player_coins = bcd_add_u16(player_coins, coins);
    player_coins_changed = true;
}

void player_set_weapon(enum weapon weapon) {
    if (current_weapon != weapon) {
        current_weapon = weapon;
        weapon_state = WEAPON_AWAY;
        weapon_frame = 0;
        weapon_move_counter = 0;
    }
}

void draw_player(void) {
    uint8_t sprite_enable = VICII_SPRITE_ENABLE & 0xFC;
    uint8_t sprite_msb = VICII_SPRITE_MSB & 0xFC;
    uint8_t sprite_y_expand = VICII_SPRITE_Y_EXPAND & 0xFC;
    uint8_t sprite_x_expand = VICII_SPRITE_X_EXPAND & 0xFC;
    uint8_t sprite_multicolor = VICII_SPRITE_MULTICOLOR & 0xFC;

    if (weapon_state == WEAPON_VISIBLE) {
        sprite_enable |= _BV(WEAPON_SPRITE_IDX);

        // In debug mode, always setup the sword registers for a better idea of
        // the worst case frame time
#ifdef DEBUG
    }
    {
#endif
        uint8_t flags;
        uint8_t sprite_pointer;
        uint8_t color;

        switch (current_weapon) {
            case WEAPON_SWORD:
                flags = sword_sprite.flags[player_dir][weapon_frame];
                sprite_pointer =
                    sword_sprite.pointers[player_dir][weapon_frame];
                color = COLOR_WHITE;
                break;

            case WEAPON_FLAIL:
                flags = flail_sprite.flags[weapon_frame];
                sprite_pointer = flail_sprite.pointers[weapon_frame];
                color = COLOR_GRAY2;
                break;
        }

        sprite_pointers_shadow[WEAPON_SPRITE_IDX] = sprite_pointer;

        VICII_SPRITE_COLOR[WEAPON_SPRITE_IDX] = color;

        if (flags & SPRITE_IMAGE_EXPAND_X) {
            sprite_x_expand |= _BV(WEAPON_SPRITE_IDX);
        }

        if (flags & SPRITE_IMAGE_EXPAND_Y) {
            sprite_y_expand |= _BV(WEAPON_SPRITE_IDX);
        }

        if (flags & SPRITE_IMAGE_MULTICOLOR) {
            sprite_multicolor |= _BV(WEAPON_SPRITE_IDX);
        }

        VICII_SPRITE_POSITION[WEAPON_SPRITE_IDX].x = weapon_x & 0xFF;
        VICII_SPRITE_POSITION[WEAPON_SPRITE_IDX].y = weapon_y;
        if (weapon_x & 0x100) {
            sprite_msb |= _BV(WEAPON_SPRITE_IDX);
        }
    }

    {
        struct direction_sprite const* sprite = (weapon_state == WEAPON_VISIBLE)
                                                    ? &player_attack_sprite
                                                    : &player_sprite;

        sprite_enable |= _BV(PLAYER_SPRITE_IDX);
        uint8_t flags = sprite->flags[player_dir][player_frame];

        if (flags & SPRITE_IMAGE_EXPAND_X) {
            sprite_x_expand |= _BV(PLAYER_SPRITE_IDX);
        }

        if (flags & SPRITE_IMAGE_EXPAND_Y) {
            sprite_y_expand |= _BV(PLAYER_SPRITE_IDX);
        }

        if (flags & SPRITE_IMAGE_MULTICOLOR) {
            sprite_multicolor |= _BV(PLAYER_SPRITE_IDX);
        }

        uint16_t x = player_get_x();

        VICII_SPRITE_POSITION[PLAYER_SPRITE_IDX].x = x & 0xFF;
        VICII_SPRITE_POSITION[PLAYER_SPRITE_IDX].y = player_get_y();
        if (x & 0x100) {
            sprite_msb |= _BV(PLAYER_SPRITE_IDX);
        }

        switch (player_temp_invulnerable & 0x3) {
            case 0:
                VICII_SPRITE_COLOR[PLAYER_SPRITE_IDX] = PLAYER_COLOR;
                break;

            case 1:
                VICII_SPRITE_COLOR[PLAYER_SPRITE_IDX] = PLAYER_HIT_COLOR_1;
                break;

            case 2:
                VICII_SPRITE_COLOR[PLAYER_SPRITE_IDX] = PLAYER_HIT_COLOR_2;
                break;

            case 3:
                VICII_SPRITE_COLOR[PLAYER_SPRITE_IDX] = PLAYER_HIT_COLOR_3;
                break;
        }
        sprite_pointers_shadow[PLAYER_SPRITE_IDX] =
            sprite->pointers[player_dir][player_frame];
    }

    VICII_SPRITE_ENABLE = sprite_enable;
    VICII_SPRITE_MSB = sprite_msb;
    VICII_SPRITE_Y_EXPAND = sprite_y_expand;
    VICII_SPRITE_X_EXPAND = sprite_x_expand;
    VICII_SPRITE_MULTICOLOR = sprite_multicolor;
}

void tick_player(void) {
    if (player_temp_invulnerable) {
        player_temp_invulnerable--;
    }

    if (player_push_timer) {
        player_push_timer--;
    } else {
        player_push_x = 0;
        player_push_y = 0;
    }

    if (weapon_state == WEAPON_VISIBLE) {
        struct bb16 const sword_bb16 =
            bb_add_offset(get_weapon_bb(), weapon_x, weapon_y);

#pragma clang loop unroll(full)
        for (uint8_t idx = 0; idx < MAX_MOBS; idx++) {
            if (mob_has_weapon_collision(idx) &&
                check_mob_collision(idx, sword_bb16)) {
                switch (current_weapon) {
                    case WEAPON_SWORD:
                        mob_trigger_weapon_collision(idx, player_weapon_damage);
                        weapon_state = WEAPON_ATTACKED;
                        break;

                    case WEAPON_FLAIL:
                        mob_trigger_weapon_collision(idx, flail_damage);
                        flail_damage--;
                        flail_timer = 0;
                        if (!flail_damage) {
                            weapon_state = WEAPON_ATTACKED;
                        }
                        break;
                }
            }
        }
    } else {
        weapon_move_counter = 0;
    }

    struct bb16 const player_bb16 =
        bb_add_offset(get_player_bb(), player_get_x(), player_get_y());

#pragma clang loop unroll(full)
    for (uint8_t idx = 0; idx < MAX_MOBS; idx++) {
        if (mob_has_player_collision(idx) &&
            check_mob_collision(idx, player_bb16)) {
            mob_trigger_player_collision(idx);
        }
    }

    int8_t move_delta_x = player_push_x;
    int8_t move_delta_y = player_push_y;

    if (player_health) {
        uint8_t m = read_joystick_2();
        if (m & _BV(JOYSTICK_FIRE_BIT)) {
            if (weapon_state == WEAPON_AWAY) {
                weapon_state = WEAPON_VISIBLE;
            }
            if (m & _BV(JOYSTICK_UP_BIT)) {
                move_delta_y -= PLAYER_SPEED / 2;
            }
            if (m & _BV(JOYSTICK_DOWN_BIT)) {
                move_delta_y += PLAYER_SPEED / 2;
            }
            if (m & _BV(JOYSTICK_LEFT_BIT)) {
                move_delta_x -= PLAYER_SPEED / 2;
            }
            if (m & _BV(JOYSTICK_RIGHT_BIT)) {
                move_delta_x += PLAYER_SPEED / 2;
            }
        } else {
            uint8_t new_dir = DIRECTION_COUNT;
            weapon_state = WEAPON_AWAY;

            if (m & _BV(JOYSTICK_UP_BIT)) {
                new_dir = NORTH;
            } else if (m & _BV(JOYSTICK_DOWN_BIT)) {
                new_dir = SOUTH;
            } else if (m & _BV(JOYSTICK_LEFT_BIT)) {
                new_dir = WEST;
            } else if (m & _BV(JOYSTICK_RIGHT_BIT)) {
                new_dir = EAST;
            }

            if (new_dir == player_dir) {
                switch (new_dir) {
                    case NORTH:
                        move_delta_y -= PLAYER_SPEED;
                        break;

                    case SOUTH:
                        move_delta_y += PLAYER_SPEED;
                        break;

                    case EAST:
                        move_delta_x += PLAYER_SPEED;
                        break;

                    case WEST:
                        move_delta_x -= PLAYER_SPEED;
                        break;
                }
            } else {
                if (new_dir != DIRECTION_COUNT) {
                    player_dir = new_dir;
                }
            }
        }
    }
    check_move(player_map_x, player_map_y, &move_delta_x, &move_delta_y);

    if (move_delta_x || move_delta_y) {
        if (player_animation_counter == 0) {
            player_frame++;
            player_animation_counter = PLAYER_ANIMATION_RATE;
        } else {
            player_animation_counter--;
        }
    }

    struct direction_sprite const* sprite = (weapon_state == WEAPON_VISIBLE)
                                                ? &player_attack_sprite
                                                : &player_sprite;

    if (player_frame && player_frame >= sprite->num_frames[player_dir]) {
        player_frame = 0;
    }
    player_map_x += move_delta_x;
    player_map_y += move_delta_y;

    if (weapon_state == WEAPON_VISIBLE) {
        if (weapon_animation_counter == 0) {
            weapon_frame++;
            weapon_animation_counter = WEAPON_ANIMATION_RATE;
        } else {
            weapon_animation_counter = 0;
        }

        uint8_t max_frames;
        switch (current_weapon) {
            case WEAPON_SWORD:
                max_frames = sword_sprite.num_frames[player_dir];
                weapon_x = player_get_x() + sword_offset[player_dir].x;
                weapon_y = player_get_y() + sword_offset[player_dir].y;
                break;

            case WEAPON_FLAIL:
                max_frames = flail_sprite.num_frames;
                if (weapon_move_counter < 0x80) {
                    flail_damage = 1;
                    flail_timer = 0;
                    switch (player_dir) {
                        case NORTH:
                            weapon_x = player_get_x();
                            weapon_y = player_get_y() - weapon_move_counter;
                            break;

                        case SOUTH:
                            weapon_x = player_get_x();
                            weapon_y = player_get_y() + weapon_move_counter;
                            break;

                        case EAST:
                            weapon_x = player_get_x() + weapon_move_counter;
                            weapon_y = player_get_y();
                            break;

                        case WEST:
                            weapon_x = player_get_x() - weapon_move_counter;
                            weapon_y = player_get_y();
                            break;
                    }
                    if (weapon_move_counter >= FLAIL_R) {
                        switch (player_dir) {
                            case SOUTH:
                                weapon_move_counter =
                                    0x80 +
                                    (ARRAY_SIZE(flail_table) * 2 * 1) / 4;
                                break;
                            case NORTH:
                                weapon_move_counter =
                                    0x80 +
                                    (ARRAY_SIZE(flail_table) * 2 * 3) / 4;
                                break;
                            case WEST:
                                weapon_move_counter = 0x80;
                                break;
                            case EAST:
                                weapon_move_counter =
                                    0x80 + (ARRAY_SIZE(flail_table) * 2) / 2;
                                break;
                        }
                    } else {
                        weapon_move_counter += 5;
                    }
                } else {
                    if ((weapon_move_counter & 0x7F) >=
                        ARRAY_SIZE(flail_table) * 2) {
                        weapon_move_counter -= ARRAY_SIZE(flail_table) * 2;
                    }

                    uint8_t y_idx = weapon_move_counter & 0x7F;
                    if (y_idx >= ARRAY_SIZE(flail_table)) {
                        weapon_y = player_get_y() -
                                   flail_table[y_idx - ARRAY_SIZE(flail_table)];
                    } else {
                        weapon_y = player_get_y() + flail_table[y_idx];
                    }

                    uint8_t x_idx = (weapon_move_counter & 0x7F) +
                                    ARRAY_SIZE(flail_table) / 2;
                    if (x_idx >= ARRAY_SIZE(flail_table) * 2) {
                        weapon_x =
                            player_get_x() -
                            flail_table[x_idx - ARRAY_SIZE(flail_table) * 2];
                    } else if (x_idx >= ARRAY_SIZE(flail_table)) {
                        weapon_x = player_get_x() +
                                   flail_table[x_idx - ARRAY_SIZE(flail_table)];
                    } else {
                        weapon_x = player_get_x() - flail_table[x_idx];
                    }

                    switch (flail_damage) {
                        case 1:
                            weapon_move_counter++;
                            flail_timer++;
                            break;

                        case 2:
                            weapon_move_counter += 2;
                            flail_timer += 2;
                            break;

                        case 3:
                            weapon_move_counter += 4;
                            break;
                    }

                    if (flail_damage < 3 &&
                        flail_timer >= ARRAY_SIZE(flail_table) * 2) {
                        flail_damage++;
                        flail_timer = 0;
                    }
                }
                break;
        }
        if (weapon_frame >= max_frames) {
            weapon_frame = 0;
        }
    }
}

extern const struct bb player_north_bb;
extern const struct bb player_south_bb;
extern const struct bb player_east_bb;
extern const struct bb player_west_bb;

extern const struct bb sword_north_bb;
extern const struct bb sword_south_bb;
extern const struct bb sword_east_bb;
extern const struct bb sword_west_bb;

extern const struct bb flail_bb;

struct bb const* get_player_bb(void) {
    switch (player_dir) {
        case NORTH:
            return &player_north_bb;
        case SOUTH:
            return &player_south_bb;
        case EAST:
            return &player_east_bb;
        case WEST:
            return &player_west_bb;
    }
}

struct bb const* get_weapon_bb(void) {
    switch (player_dir) {
        case NORTH:
            return &sword_north_bb;
        case SOUTH:
            return &sword_south_bb;
        case EAST:
            return &sword_east_bb;
        case WEST:
            return &sword_west_bb;
    }
}

