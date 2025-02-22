/*
 * SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "color.h"
#include "draw.h"
#include "input.h"
#include "isr.h"
#include "map.h"
#include "mobs.h"
#include "move.h"
#include "player-sprite.h"
#include "player.h"
#include "reg.h"
#include "sprite.h"

// 67 is a bad line so we start right after that
#define STATUS_INT_LINE (68)

// We don't need to avoid badlines here because it at the end of the display
#define DONE_INT_LINE ((uint16_t)(250))

#define DEFAULT_VICII_CTRL_1 (0x1B)
#define DEFAULT_VICII_CTRL_2 (0xC8)

#define HEART_CHAR (0x53)
#define HALF_HEART_CHAR (0x54)
#define BLANK_CHAR (0x20)
#define ZERO_CHAR (0x30)
#define DOLLAR_CHAR (0x24)

#define HEALTH_X_TILE (0)
#define HEALTH_Y_TILE (0)
#define COIN_X_TILE (0)
#define COIN_Y_TILE (1)

extern const uint8_t game_tiles[256];

#ifdef DEBUG
#define DEBUG_COLOR(_c)          \
    do {                         \
        VICII_BORDER_COLOR = _c; \
    } while (0)
#else
#define DEBUG_COLOR(_c)
#endif

#define START_TEXT_X (10)
#define START_TEXT_Y (0)
static const char start_text[] = "PRESS FIRE TO START";

#define GAME_OVER_TEXT_X (15)
#define GAME_OVER_TEXT_Y (1)
static const char game_over_text[] = "GAME OVER";

// SCORE (right justified)
#define SCORE_TEXT_X (SCREEN_WIDTH_TILE)
#define SCORE_TEXT_Y (0)

static const char score_text[] = "SCORE ";

void prepare_status_int(void) {
    VICII_BG_0 = COLOR_BLACK;
    VICII_CTRL_1 = DEFAULT_VICII_CTRL_1;
    VICII_CTRL_2 = DEFAULT_VICII_CTRL_2;
    VICII_INTERRUPT_ENABLE = _BV(VICII_RST_BIT);
    VICII_RASTER = STATUS_INT_LINE;
}

void frame_wait(void) {
    static uint8_t next_frame = 0;
    while (frame_count == next_frame);
    while (frame_count == next_frame + 1);
    next_frame = frame_count;

    prepare_status_int();
}

void wait_frames(uint16_t num_frames) {
    for (; num_frames; num_frames--) {
        frame_wait();
    }
}

static uint16_t score = 0;
static bool score_updated;

uint8_t get_collision(uint8_t* cdata) {
    disable_interrupts();
    uint8_t c = *cdata;
    *cdata = 0;
    enable_interrupts();
    return c;
}

static void skeleton_reached_target(struct mob* mob) {
    mob->target_x = player_x;
    mob->target_y = player_y;
}

static void skeleton_player_collision(struct mob* mob) {
    enum direction dir = dir_from(mob->x, mob->y, player_x, player_y);
    switch (dir) {
        case NORTH:
            damage_player_push(1, 0, -1);
            break;

        case SOUTH:
            damage_player_push(1, 0, 1);
            break;

        case EAST:
            damage_player_push(1, 1, 0);
            break;

        case WEST:
            damage_player_push(1, -1, 0);
    }
}

static void on_skeleton_kill(struct mob* mob);

#define CENTER_SPRITE_X ((QUAD_WIDTH_PX / 2) - (SPRITE_WIDTH_PX / 2))
#define CENTER_SPRITE_Y ((QUAD_HEIGHT_PX / 2) - (SPRITE_HEIGHT_PX / 2))

static bool new_skeleton(void) {
    uint8_t p = rand();
    uint16_t y;
    uint16_t x;

    switch (p & 0x03) {
        case 0:
            // North side
            x = QUAD_X_TO_PX((p >> 2) % MAP_WIDTH_QUAD) + CENTER_SPRITE_X;
            y = QUAD_Y_TO_PX(0) + CENTER_SPRITE_Y;
            break;

        case 1:
            // South side
            x = QUAD_X_TO_PX((p >> 2) % MAP_WIDTH_QUAD) + CENTER_SPRITE_X;
            y = QUAD_Y_TO_PX(MAP_HEIGHT_QUAD - 1) + CENTER_SPRITE_Y;
            break;

        case 2:
            // East side
            x = QUAD_X_TO_PX(MAP_WIDTH_QUAD - 1) + CENTER_SPRITE_X;
            y = QUAD_Y_TO_PX((p >> 2) % MAP_HEIGHT_QUAD) + CENTER_SPRITE_Y;
            break;

        case 3:
            // West side
            x = QUAD_X_TO_PX(0) + CENTER_SPRITE_X;
            y = QUAD_Y_TO_PX((p >> 2) % MAP_HEIGHT_QUAD) + CENTER_SPRITE_Y;
            break;
    }

    struct mob* mob = create_skeleton(x, y);
    if (!mob) {
        return false;
    }

    mob->on_death = on_skeleton_kill;
    mob->target_x = player_x - (SKELETON_OFFSET_X_PX + SKELETON_WIDTH_PX / 2);
    mob->target_y = player_y - (SKELETON_OFFSET_Y_PX + SKELETON_HEIGHT_PX / 2);
    mob->on_reached_target = skeleton_reached_target;
    mob->on_player_collision = skeleton_player_collision;
    mob->speed = 1 + (rand() & 0x7);

    return true;
}

static void on_powerup_kill(struct mob* mob) {
    destroy_mob(mob);
    new_skeleton();
}

static void on_skeleton_kill(struct mob* mob) {
    uint16_t x = mob->x;
    uint16_t y = mob->y;
    destroy_mob(mob);

    score += 10;
    score_updated = true;

    switch (rand() & 0x3) {
        case 0:
            mob = create_coin(x, y, 1);
            if (mob) {
                mob->on_death = on_powerup_kill;
            }
            break;

        case 1:
            mob = create_heart(x, y);
            if (mob) {
                mob->on_death = on_powerup_kill;
            }
            break;

        default:
            new_skeleton();
            break;
    }
}

void game_loop(void) {
    uint8_t last_coins_drawn = 0xFF;

    score = 0;
    score_updated = true;

    while (true) {
        // Wait for next frame interrupt
        DEBUG_COLOR(COLOR_BLACK);
        frame_wait();

        // Frame critical. These need to be done before the next frame
        // starts
        DEBUG_COLOR(COLOR_RED);

        capture_mob_collisions();

        draw_player();

        for (uint8_t i = 0; i < PLAYER_MAX_HEALTH / 2; i++) {
            if (player_health <= i * 2) {
                put_char_xy(HEALTH_X_TILE + i, HEALTH_Y_TILE, BLANK_CHAR);
            } else if (player_health == (i * 2) + 1) {
                put_char_xy(HEALTH_X_TILE + i, HEALTH_Y_TILE, HALF_HEART_CHAR);
            } else {
                put_char_xy(HEALTH_X_TILE + i, HEALTH_Y_TILE, HEART_CHAR);
            }
            set_color(HEALTH_X_TILE + i, HEALTH_Y_TILE, COLOR_RED);
        }

        if (player_coins != last_coins_drawn) {
            char buf[8];
            buf[0] = '$';
            int_to_string(player_coins, &buf[1]);
            put_string_xy(COIN_X_TILE, COIN_Y_TILE, COLOR_GREEN, buf);
            last_coins_drawn = player_coins;
        }

        if (score_updated) {
            char buf[sizeof(score_text) + 7];
            memcpy(buf, score_text, sizeof(score_text));
            int_to_string(score, &buf[strlen(score_text)]);

            put_string_xy(SCORE_TEXT_X - strlen(buf), SCORE_TEXT_Y, COLOR_WHITE,
                          buf);

            score_updated = false;
        }

        DEBUG_COLOR(COLOR_YELLOW);
        draw_mobs();

        // Frame non critical. These can be done during the frame since they
        // don't affect graphics
        DEBUG_COLOR(COLOR_GREEN);

        update_player();
        DEBUG_COLOR(COLOR_VIOLET);
        update_mobs();

        if (player_health <= 0 && !player_temp_invulnerable) {
            break;
        }
    }
}

int main() {
    // Blank screen while setting up
    VICII_CTRL_1 &= ~_BV(VICII_DEN_BIT);

    disable_interrupts();

    // Configure interrupts
    ISR_VECTOR = isr_handler;

    prepare_status_int();

    vicii_interrupt_enable_next = _BV(VICII_RST_BIT) | _BV(VICII_MMC_BIT);

    vicii_raster_next = (DONE_INT_LINE & 0xFF);

    vicii_ctrl_1_next = DEFAULT_VICII_CTRL_1;
    vicii_ctrl_1_next &= ~_BV(VICII_RST8_BIT);
    vicii_ctrl_1_next |= (DONE_INT_LINE >> 8) << VICII_RST8_BIT;

    // Enable multicolor character mode
    vicii_ctrl_2_next = DEFAULT_VICII_CTRL_2 | _BV(VICII_MCM_BIT);

    // Disable all CIA interrupts
    CIA_1_INTERRUPT = 0x7F;
    CIA_2_INTERRUPT = 0x7F;

    // Configure VIC Memory
    CIA_2_PORT_A &= 0xFC;
    CIA_2_PORT_A |= (~(VIC_BASE >> 14) & 0x03);

    VICII_MEM_PTR = ((((uint16_t)&game_tiles) - VIC_BASE) >> 10) |
                    ((((uint16_t)&screen_data) - VIC_BASE) >> 6);

    // Set colors
    VICII_BORDER_COLOR = COLOR_BLACK;
    VICII_SPRITE_MULTICOLOR_0 = COLOR_LIGHT_RED;
    VICII_SPRITE_MULTICOLOR_1 = COLOR_BLACK;

    current_screen = &main_screen;

    enable_interrupts();
    frame_wait();

    while (true) {
        fill_char(0, 0, SCREEN_WIDTH_TILE - 1, SCREEN_HEIGHT_TILE - 1,
                  BLANK_CHAR);
        fill_color(0, 0, SCREEN_WIDTH_TILE - 1, SCREEN_HEIGHT_TILE - 1,
                   COLOR_BLACK);
        draw_background(current_screen);

        player_x = 200;
        player_y = 100;
        player_dir = SOUTH;
        player_sword_damage = 1;
        player_health = 6;
        player_full_health = 6;
        player_coins = 0;
        sword_state = SWORD_AWAY;
        player_temp_invulnerable = 0;

        init_player();

        destroy_all_mobs();

        while (new_skeleton());

        put_string_xy(START_TEXT_X, START_TEXT_Y, COLOR_WHITE, start_text);

        wait_frames(30);

        // Wait for fire button to start
        while ((read_joystick_2() & _BV(JOYSTICK_FIRE_BIT)) == 0) {
            frame_wait();
        }

        fill_char(START_TEXT_X, START_TEXT_Y, START_TEXT_X + strlen(start_text),
                  START_TEXT_Y, ' ');

        fill_char(GAME_OVER_TEXT_X, GAME_OVER_TEXT_Y,
                  GAME_OVER_TEXT_X + strlen(game_over_text), GAME_OVER_TEXT_Y,
                  ' ');

        game_loop();

        put_string_xy(GAME_OVER_TEXT_X + 1, GAME_OVER_TEXT_Y, COLOR_RED,
                      game_over_text);
        wait_frames(4 * 60);
    }
}
