/*
 * SPDX-License-Identifier: MIT
 */
#include <cbm.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#define PLAYER_START_X_QUAD ((MAP_WIDTH_QUAD / 2) + 3)
#define PLAYER_START_Y_QUAD (MAP_HEIGHT_QUAD / 2)

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
    static uint8_t next_interrupt = 0;
    while (interrupt_count == next_interrupt);
    while (interrupt_count == next_interrupt + 1);
    next_interrupt = interrupt_count;
    frame_count++;

    prepare_status_int();
}

void wait_frames(uint16_t num_frames) {
    for (; num_frames; num_frames--) {
        frame_wait();
    }
}

static uint16_t score = 0;
static bool score_updated;

static uint8_t quad_distance(uint8_t quad_x1, uint8_t quad_y1, uint8_t quad_x2,
                             uint8_t quad_y2) {
    uint16_t xdist;
    uint16_t ydist;

    if (quad_x1 > quad_x2) {
        xdist = quad_x1 - quad_x2;
    } else {
        xdist = quad_x2 - quad_x2;
    }

    if (quad_y1 > quad_y2) {
        ydist = quad_y1 - quad_y2;
    } else {
        ydist = quad_y2 - quad_y1;
    }
    return xdist + ydist;
}

static void skeleton_reached_target(uint8_t idx) {
    uint8_t mob_quad_x = mob_get_quad_x(idx);
    uint8_t mob_quad_y = mob_get_quad_y(idx);
    uint8_t next_quad_x = mob_quad_x;
    uint8_t next_quad_y = mob_quad_y;
    uint8_t player_quad_x = player_get_quad_x();
    uint8_t player_quad_y = player_get_quad_y();

    uint8_t r = rand();
    if (r & 0xC) {
        // Move toward the player
        //
        // Check all 4 directions, starting with a random one, and choose the
        // one that has the shortest distance to the player
        uint8_t best_dist = 0xFF;
        for (uint8_t i = 0; i < 4; i++) {
            switch ((r + i & 0x3)) {
                case NORTH:
                    if (mob_quad_y > 0 &&
                        map_tile_is_passable(mob_quad_x, mob_quad_y - 1)) {
                        uint8_t d = quad_distance(mob_quad_x, mob_quad_y - 1,
                                                  player_quad_x, player_quad_y);
                        if (d < best_dist) {
                            best_dist = d;
                            next_quad_x = mob_quad_x;
                            next_quad_y = mob_quad_y - 1;
                        }
                    }
                    break;
                case SOUTH:
                    if (mob_quad_y < MAP_HEIGHT_QUAD - 1 &&
                        map_tile_is_passable(mob_quad_x, mob_quad_y + 1)) {
                        uint8_t d = quad_distance(mob_quad_x, mob_quad_y + 1,
                                                  player_quad_x, player_quad_y);
                        if (d < best_dist) {
                            best_dist = d;
                            next_quad_x = mob_quad_x;
                            next_quad_y = mob_quad_y + 1;
                        }
                    }
                    break;
                case EAST:
                    if (mob_quad_x < MAP_WIDTH_QUAD - 1 &&
                        map_tile_is_passable(mob_quad_x + 1, mob_quad_y)) {
                        uint16_t d =
                            quad_distance(mob_quad_x + 1, mob_quad_y,
                                          player_quad_x, player_quad_y);
                        if (d < best_dist) {
                            best_dist = d;
                            next_quad_x = mob_quad_x + 1;
                            next_quad_y = mob_quad_y;
                        }
                    }
                    break;
                case WEST:
                    if (mob_quad_x > 0 &&
                        map_tile_is_passable(mob_quad_x - 1, mob_quad_y)) {
                        uint16_t d =
                            quad_distance(mob_quad_x - 1, mob_quad_y,
                                          player_quad_x, player_quad_y);
                        if (d < best_dist) {
                            best_dist = d;
                            next_quad_x = mob_quad_x - 1;
                            next_quad_y = mob_quad_y;
                        }
                    }
                    break;
            }
        }
    } else {
        // Random move
        //
        // Randomly pick a direction. If that quad is passable, move there
        // otherwise check the other directions
        for (uint8_t i = 0; i < 4; i++) {
            switch ((r + i & 0x3)) {
                case NORTH:
                    if (mob_quad_y > 0 &&
                        map_tile_is_passable(mob_quad_x, mob_quad_y - 1)) {
                        next_quad_x = mob_quad_x;
                        next_quad_y = mob_quad_y - 1;
                        goto done;
                    }
                    break;
                case SOUTH:
                    if (mob_quad_y < MAP_HEIGHT_QUAD - 1 &&
                        map_tile_is_passable(mob_quad_x, mob_quad_y + 1)) {
                        next_quad_x = mob_quad_x;
                        next_quad_y = mob_quad_y + 1;
                        goto done;
                    }
                    break;
                case EAST:
                    if (mob_quad_x < MAP_WIDTH_QUAD - 1 &&
                        map_tile_is_passable(mob_quad_x + 1, mob_quad_y)) {
                        next_quad_x = mob_quad_x + 1;
                        next_quad_y = mob_quad_y;
                        goto done;
                    }
                    break;
                case WEST:
                    if (mob_quad_x > 0 &&
                        map_tile_is_passable(mob_quad_x - 1, mob_quad_y)) {
                        next_quad_x = mob_quad_x - 1;
                        next_quad_y = mob_quad_y;
                        goto done;
                    }
                    break;
            }
        }
    }

done:
    mob_set_target(idx, next_quad_x * QUAD_WIDTH_PX + QUAD_WIDTH_PX / 2,
                   next_quad_y * QUAD_HEIGHT_PX + QUAD_HEIGHT_PX / 2);
}

static void skeleton_player_collision(uint8_t idx) {
    enum direction dir = dir_from(mob_get_map_x(idx), mob_get_map_y(idx),
                                  player_map_x, player_map_y);
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

static void on_skeleton_kill(uint8_t idx);

static bool new_skeleton(void) {
    uint8_t quad_x;
    uint8_t quad_y;

    do {
        uint8_t p = rand();

        switch (p & 0x03) {
            case 0:
                // North side
                quad_x = (p >> 2) % MAP_WIDTH_QUAD;
                quad_y = 0;
                break;

            case 1:
                // South side
                quad_x = (p >> 2) % MAP_WIDTH_QUAD;
                quad_y = MAP_HEIGHT_QUAD - 1;
                break;

            case 2:
                // East side
                quad_x = MAP_WIDTH_QUAD - 1;
                quad_y = (p >> 2) % MAP_HEIGHT_QUAD;
                break;

            case 3:
                // West side
                quad_x = 0;
                quad_y = (p >> 2) % MAP_HEIGHT_QUAD;
                break;
        }
    } while (!map_tile_is_passable(quad_x, quad_y));

    uint16_t x = quad_x * QUAD_WIDTH_PX + QUAD_WIDTH_PX / 2;
    uint8_t y = quad_y * QUAD_HEIGHT_PX + QUAD_HEIGHT_PX / 2;

    uint8_t idx = create_skeleton(x, y);
    if (idx == MAX_MOBS) {
        return false;
    }

    mob_set_death_handler(idx, on_skeleton_kill);
    mob_set_target(idx, x, y);
    mob_set_reached_target_handler(idx, skeleton_reached_target);
    mob_set_player_collision_handler(idx, skeleton_player_collision);
    mob_set_speed(idx, 1, 1 + (rand() & 0x3));

    skeleton_reached_target(idx);

    return true;
}

static void on_powerup_kill(uint8_t idx) {
    destroy_mob(idx);
    new_skeleton();
}

static void on_skeleton_kill(uint8_t idx) {
    uint16_t x = mob_get_map_x(idx);
    uint8_t y = mob_get_map_y(idx);
    destroy_mob(idx);

    score += 10;
    score_updated = true;

    switch (rand() & 0x3) {
        case 0:
            idx = create_coin(x, y, 1);
            if (idx != MAX_MOBS) {
                mob_set_death_handler(idx, on_powerup_kill);
            }
            break;

        case 1:
            idx = create_heart(x, y);
            if (idx != MAX_MOBS) {
                mob_set_death_handler(idx, on_powerup_kill);
            }
            break;

        default:
            new_skeleton();
            break;
    }
}

#ifdef DEBUG
static uint16_t get_raster(void) {
    return (VICII_CTRL_1 & _BV(VICII_RST8_BIT)) << 1 | VICII_RASTER;
}
#endif

void game_loop(void) {
    uint8_t last_coins_drawn = 0xFF;

#ifdef DEBUG
    uint16_t worst_raster_wait = 0xFFFF;
    uint16_t raster_sum = 0;
#endif

    score = 0;
    score_updated = true;

    fill_color(HEALTH_X_TILE, HEALTH_Y_TILE,
               HEALTH_X_TILE + PLAYER_MAX_HEALTH / 2, HEALTH_Y_TILE, COLOR_RED);

    while (true) {
        // Wait for next frame interrupt
        DEBUG_COLOR(COLOR_BLACK);

#ifdef DEBUG
        uint16_t raster_wait_start = get_raster();
#endif
        frame_wait();
#ifdef DEBUG
        uint16_t raster_wait_end = get_raster();
        // There is a change that we read the raster register right as it rolls
        // over. If so, it will be zero. Rather than deal with this, ignore
        // zeros
        if (raster_wait_start && raster_wait_end) {
            uint16_t elapsed = raster_wait_end - raster_wait_start;
            if (elapsed < worst_raster_wait) {
                worst_raster_wait = elapsed;
            }
            raster_sum += elapsed;
        }
#endif

        // Frame critical. These need to be done before the next frame
        // starts
        DEBUG_COLOR(COLOR_RED);

        draw_player();

        for (uint8_t i = 0; i < PLAYER_MAX_HEALTH / 2; i++) {
            if (player_health <= i * 2) {
                put_char_xy(HEALTH_X_TILE + i, HEALTH_Y_TILE, BLANK_CHAR);
            } else if (player_health == (i * 2) + 1) {
                put_char_xy(HEALTH_X_TILE + i, HEALTH_Y_TILE, HALF_HEART_CHAR);
            } else {
                put_char_xy(HEALTH_X_TILE + i, HEALTH_Y_TILE, HEART_CHAR);
            }
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
#ifdef DEBUG
        if ((frame_count & 0x3F) == 0) {
            char buf[14];
            fill_char(SCORE_TEXT_X - 14, SCORE_TEXT_Y + 1, SCORE_TEXT_X,
                      SCORE_TEXT_Y + 1, ' ');
            raster_sum = raster_sum >> 6;
            int_to_string(raster_sum, buf);
            strcat(buf, "/");
            int_to_string(worst_raster_wait, &buf[strlen(buf)]);
            put_string_xy(SCORE_TEXT_X - strlen(buf), SCORE_TEXT_Y + 1,
                          COLOR_CYAN, buf);
        }

        if (frame_count == 0) {
            worst_raster_wait = 0xFFFF;
        }
#endif

        DEBUG_COLOR(COLOR_YELLOW);
        draw_mobs();

        // Frame non critical. These can be done during the frame since they
        // don't affect graphics
        DEBUG_COLOR(COLOR_GREEN);

        update_player();
        DEBUG_COLOR(COLOR_PURPLE);
        update_mobs();

        if (player_health <= 0 && !player_temp_invulnerable) {
            break;
        }
    }
}

void load_data(char const* fn, uint8_t* dest) {
    uint8_t buffer[256];

    cbm_k_clall();
    cbm_k_setnam(fn);
    cbm_k_setlfs(15, 8, 2);
    cbm_k_open();
    cbm_k_chkin(15);
    uint8_t i = 0;
    while (!cbm_k_readst()) {
        buffer[i] = cbm_k_chrin();
        i++;
        if (i == 0) {
            ALL_RAM() { memcpy(dest, buffer, sizeof(buffer)); }
            dest += sizeof(buffer);
        }
    }
    ALL_RAM() { memcpy(dest, buffer, i); }
    cbm_k_close(15);
}

int main() {
    // Blank screen while setting up
    VICII_CTRL_1 &= ~_BV(VICII_DEN_BIT);

    load_data("GRAPHICS", &video_base);

    disable_interrupts();

    // Configure interrupts
    ISR_VECTOR = isr_handler;

    prepare_status_int();

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
    VICII_SPRITE_MULTICOLOR_0 = COLOR_LIGHTRED;
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

        player_map_x = PLAYER_START_X_QUAD * QUAD_WIDTH_PX + QUAD_WIDTH_PX / 2;
        player_map_y =
            PLAYER_START_Y_QUAD * QUAD_HEIGHT_PX + QUAD_HEIGHT_PX / 2;
        player_dir = SOUTH;
        player_sword_damage = 1;
        player_health = 6;
        player_full_health = 6;
        player_coins = 0;

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
