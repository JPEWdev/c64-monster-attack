/*
 * SPDX-License-Identifier: MIT
 */
#include "player.h"

#include <stdlib.h>

#include "input.h"
#include "mobs.h"
#include "move.h"
#include "player-sprite.h"
#include "reg.h"

#define PLAYER_SPEED (2)

uint16_t player_x;
uint16_t player_y;
enum direction player_dir = SOUTH;
uint8_t player_sword_damage;
uint8_t player_health;
uint8_t player_full_health;
uint8_t player_coins;
uint8_t player_temp_invulnerable;
static int8_t player_push_x;
static int8_t player_push_y;
static uint8_t player_push_timer;

uint8_t sword_state;
uint16_t sword_x = 0;
uint16_t sword_y = 0;

static struct animation player_animation = {0};
static struct animation sword_animation = {0};

static enum direction player_sprite_dir = SOUTH;
static bool player_moving = false;
static struct direction_sprite const* current_player_sprite = &player_sprite;

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
    draw_direction_sprite(PLAYER_SPRITE_IDX, current_player_sprite, COLOR_GREEN,
                          player_dir);
    move_sprite(PLAYER_SPRITE_IDX, player_x, player_y);
    sprite_enable_collisions(PLAYER_SPRITE_IDX, true);
    sprite_enable_collisions(SWORD_SPRITE_IDX, true);

    player_animation.rate = 10;
    sword_animation.rate = 10;
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

void draw_player(void) {
    move_sprite(PLAYER_SPRITE_IDX, player_x, player_y);
    move_sprite(SWORD_SPRITE_IDX, sword_x, sword_y);
    if (sword_state == SWORD_VISIBLE && !sprite_is_visible(SWORD_SPRITE_IDX)) {
        draw_direction_sprite(SWORD_SPRITE_IDX, &sword_sprite, COLOR_WHITE,
                              player_dir);
        draw_direction_sprite(PLAYER_SPRITE_IDX, &player_attack_sprite,
                              COLOR_GREEN, player_dir);
        current_player_sprite = &player_attack_sprite;
    } else if (sword_state != SWORD_VISIBLE &&
               sprite_is_visible(SWORD_SPRITE_IDX)) {
        hide_sprite(SWORD_SPRITE_IDX);
        draw_direction_sprite(PLAYER_SPRITE_IDX, &player_sprite, COLOR_GREEN,
                              player_dir);
        current_player_sprite = &player_sprite;
    } else {
        if (player_sprite_dir == player_dir) {
            if (player_moving) {
                animate_step(PLAYER_SPRITE_IDX, &player_animation,
                             current_player_sprite->sprites[player_dir]);
                animate_step(SWORD_SPRITE_IDX, &sword_animation,
                             sword_sprite.sprites[player_dir]);
            }
        } else {
            if (sword_state == SWORD_VISIBLE) {
                draw_direction_sprite(SWORD_SPRITE_IDX, &sword_sprite,
                                      COLOR_WHITE, player_dir);
            }
            draw_direction_sprite(PLAYER_SPRITE_IDX, current_player_sprite,
                                  COLOR_GREEN, player_dir);
        }
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
    player_sprite_dir = player_dir;
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
        uint8_t idx = 0;
        struct mob* m;
        while ((m = check_mob_collision(&idx, SWORD_SPRITE_IDX)) != NULL) {
            if (m->on_sword_collision) {
                m->on_sword_collision(m, player_sword_damage);
            }
            sword_state = SWORD_ATTACKED;
        }
    }

    {
        uint8_t idx = 0;
        struct mob* m;
        while ((m = check_mob_collision(&idx, PLAYER_SPRITE_IDX)) != NULL) {
            if (m->on_player_collision) {
                m->on_player_collision(m);
            }
        }
    }

    int8_t move_delta_x = player_push_x;
    int8_t move_delta_y = player_push_y;

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
    check_move(player_x + SPRITE_WIDTH_PX / 2, player_y + SPRITE_HEIGHT_PX / 2,
               &move_delta_x, &move_delta_y);

    if (move_delta_x || move_delta_y) {
        player_moving = true;
    }

    player_x += move_delta_x;
    player_y += move_delta_y;
    sword_x = player_x + sword_offset[player_dir].x;
    sword_y = player_y + sword_offset[player_dir].y;
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

