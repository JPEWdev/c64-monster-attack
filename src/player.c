/*
 * SPDX-License-Identifier: MIT
 */
#include "player.h"

#include <stdlib.h>

#include "input.h"
#include "map.h"
#include "mobs.h"
#include "move.h"
#include "player-sprite.h"
#include "reg.h"

#define PLAYER_SPEED (2)

#define PLAYER_ANIMATION_RATE (10)
#define SWORD_ANIMATION_RATE (10)

uint16_t player_map_x;
uint8_t player_map_y;
enum direction player_dir = SOUTH;
uint8_t player_sword_damage;
uint8_t player_health;
uint8_t player_full_health;
uint8_t player_coins;
uint8_t player_temp_invulnerable;
static int8_t player_push_x;
static int8_t player_push_y;
static uint8_t player_push_timer;
static uint8_t player_frame;
static uint8_t player_animation_counter;
static uint8_t sword_frame;
static uint8_t sword_animation_counter;

uint8_t sword_state;
uint16_t sword_x = 0;
uint16_t sword_y = 0;

// static enum direction player_sprite_dir = SOUTH;
static bool player_moving = false;

static const struct {
    int8_t x;
    int8_t y;
} sword_offset[] = {
    /* NORTH */ {10, -14},
    /* SOUTH */ {-8, 15},
    /* EAST */ {18, 2},
    /* WEST */ {-18, 2},
};

void init_player(void) {
    player_push_x = 0;
    player_push_y = 0;
    player_push_timer = 0;
    sword_state = SWORD_AWAY;
    player_temp_invulnerable = 0;
    player_frame = 0;
    sword_frame = 0;
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
}

uint16_t player_get_x(void) {
    return player_map_x + MAP_OFFSET_X_PX - SPRITE_WIDTH_PX / 2;
}

uint8_t player_get_y(void) {
    return player_map_y + MAP_OFFSET_Y_PX - SPRITE_HEIGHT_PX / 2;
}

uint8_t player_get_quad_x(void) { return player_map_x / QUAD_WIDTH_PX; }

uint8_t player_get_quad_y(void) { return player_map_y / QUAD_HEIGHT_PX; }

void draw_player(void) {
    uint8_t sprite_enable = VICII_SPRITE_ENABLE & 0xFC;
    uint8_t sprite_msb = VICII_SPRITE_MSB & 0xFC;
    uint8_t sprite_y_expand = VICII_SPRITE_Y_EXPAND & 0xFC;
    uint8_t sprite_x_expand = VICII_SPRITE_X_EXPAND & 0xFC;
    uint8_t sprite_multicolor = VICII_SPRITE_MULTICOLOR & 0xFC;

    if (sword_state == SWORD_VISIBLE) {
        sprite_enable |= _BV(SWORD_SPRITE_IDX);

        // In debug mode, always setup the sword registers for a better idea of
        // the worst case frame time
#ifdef DEBUG
    }
    {
#endif
        if (sword_animation_counter == 0) {
            sword_frame++;
            sword_animation_counter = SWORD_ANIMATION_RATE;
        } else {
            sword_animation_counter = 0;
        }

        if (sword_frame && sword_frame >= sword_sprite.num_frames[player_dir]) {
            sword_frame = 0;
        }

        uint8_t flags = sword_sprite.flags[player_dir][sword_frame];

        if (flags & SPRITE_IMAGE_EXPAND_X) {
            sprite_x_expand |= _BV(SWORD_SPRITE_IDX);
        }

        if (flags & SPRITE_IMAGE_EXPAND_Y) {
            sprite_y_expand |= _BV(SWORD_SPRITE_IDX);
        }

        if (flags & SPRITE_IMAGE_MULTICOLOR) {
            sprite_multicolor |= _BV(SWORD_SPRITE_IDX);
        }

        VICII_SPRITE_POSITION[SWORD_SPRITE_IDX].x = sword_x & 0xFF;
        VICII_SPRITE_POSITION[SWORD_SPRITE_IDX].y = sword_y;
        if (sword_x & 0x100) {
            sprite_msb |= _BV(SWORD_SPRITE_IDX);
        }

        VICII_SPRITE_COLOR[SWORD_SPRITE_IDX] = COLOR_WHITE;
    }

    struct direction_sprite const* sprite =
        (sword_state == SWORD_VISIBLE) ? &player_attack_sprite : &player_sprite;
    {
        if (player_moving) {
            if (player_animation_counter == 0) {
                player_frame++;
                player_animation_counter = PLAYER_ANIMATION_RATE;
            } else {
                player_animation_counter--;
            }
        }

        if (player_frame && player_frame >= sprite->num_frames[player_dir]) {
            player_frame = 0;
        }

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
    }

    DISABLE_INTERRUPTS() {
        ALL_RAM() {
            sprite_pointers[PLAYER_SPRITE_IDX] =
                sprite->pointers[player_dir][player_frame];
            if (sword_state == SWORD_VISIBLE) {
                sprite_pointers[SWORD_SPRITE_IDX] =
                    sword_sprite.pointers[player_dir][sword_frame];
            }
        }
    }
    VICII_SPRITE_ENABLE = sprite_enable;
    VICII_SPRITE_MSB = sprite_msb;
    VICII_SPRITE_Y_EXPAND = sprite_y_expand;
    VICII_SPRITE_X_EXPAND = sprite_x_expand;
    VICII_SPRITE_MULTICOLOR = sprite_multicolor;

    player_moving = false;
}

void update_player(void) {
    if (player_temp_invulnerable) {
        player_temp_invulnerable--;
    }

    if (player_push_timer) {
        player_push_timer--;
    } else {
        player_push_x = 0;
        player_push_y = 0;
    }

    if (sword_state == SWORD_VISIBLE) {
        struct bb16 const sword_bb16 =
            bb_add_offset(get_sword_bb(), sword_x, sword_y);

        for (uint8_t idx = 0; idx < MAX_MOBS; idx++) {
            if (check_mob_collision(idx, sword_bb16)) {
                mob_trigger_sword_collision(idx, player_sword_damage);
                sword_state = SWORD_ATTACKED;
            }
        }
    }

    struct bb16 const player_bb16 =
        bb_add_offset(get_player_bb(), player_get_x(), player_get_y());

    for (uint8_t idx = 0; idx < MAX_MOBS; idx++) {
        if (check_mob_collision(idx, player_bb16)) {
            mob_trigger_player_collision(idx);
        }
    }

    int8_t move_delta_x = player_push_x;
    int8_t move_delta_y = player_push_y;

    if (player_health) {
        uint8_t m = read_joystick_2();
        if (m & _BV(JOYSTICK_FIRE_BIT)) {
            if (sword_state == SWORD_AWAY) {
                sword_state = SWORD_VISIBLE;
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
            sword_state = SWORD_AWAY;

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
        player_moving = true;
    }

    player_map_x += move_delta_x;
    player_map_y += move_delta_y;
    sword_x = player_get_x() + sword_offset[player_dir].x;
    sword_y = player_get_y() + sword_offset[player_dir].y;
}

extern const struct bb player_north_bb;
extern const struct bb player_south_bb;
extern const struct bb player_east_bb;
extern const struct bb player_west_bb;

extern const struct bb sword_north_bb;
extern const struct bb sword_south_bb;
extern const struct bb sword_east_bb;
extern const struct bb sword_west_bb;

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

struct bb const* get_sword_bb(void) {
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

