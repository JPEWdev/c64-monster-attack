/*
 * SPDX-License-Identifier: MIT
 */
#include <cbm.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bcd.h"
#include "chars.h"
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
#include "tick.h"

_Static_assert(MAX_RASTER_CMDS == MAX_MOBS - 6 + 2,
               "Wrong number of raster command");

// 67 is a bad line so we start right after that
#define STATUS_INT_LINE (68)

// We don't need to avoid badlines here because it at the end of the display
#define DONE_INT_LINE ((uint16_t)(250))

#define DEFAULT_VICII_CTRL_1 (0x1B)
#define DEFAULT_VICII_CTRL_2 (0xC8)

#define HEALTH_X_TILE (0)
#define HEALTH_Y_TILE (0)
#define COIN_X_TILE (0)
#define COIN_Y_TILE (1)

#define WEAPON_X_TILE (18)
#define WEAPON_Y_TILE (0)

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

static bcd_u16 score = 0;
static bool score_updated;
static bool is_ntsc;

static struct {
    uint8_t arrow_cool_down[MAX_MOBS];
} skeleton_archer;

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

static void random_move(uint8_t mob_quad_x, uint8_t mob_quad_y,
                        uint8_t* next_quad_x, uint8_t* next_quad_y) {
    // Randomly pick a direction. If that quad is passable, move there
    // otherwise check the other directions
    uint8_t r = rand();
#pragma clang loop unroll(full)
    for (uint8_t i = 0; i < 4; i++) {
        switch ((r + i & 0x3)) {
            case NORTH:
                if (mob_quad_y > 0 &&
                    map_tile_is_passable(mob_quad_x, mob_quad_y - 1)) {
                    *next_quad_x = mob_quad_x;
                    *next_quad_y = mob_quad_y - 1;
                    return;
                }
                break;
            case SOUTH:
                if (mob_quad_y < MAP_HEIGHT_QUAD - 1 &&
                    map_tile_is_passable(mob_quad_x, mob_quad_y + 1)) {
                    *next_quad_x = mob_quad_x;
                    *next_quad_y = mob_quad_y + 1;
                    return;
                }
                break;
            case EAST:
                if (mob_quad_x < MAP_WIDTH_QUAD - 1 &&
                    map_tile_is_passable(mob_quad_x + 1, mob_quad_y)) {
                    *next_quad_x = mob_quad_x + 1;
                    *next_quad_y = mob_quad_y;
                    return;
                }
                break;
            case WEST:
                if (mob_quad_x > 0 &&
                    map_tile_is_passable(mob_quad_x - 1, mob_quad_y)) {
                    *next_quad_x = mob_quad_x - 1;
                    *next_quad_y = mob_quad_y;
                    return;
                }
                break;
        }
    }
}

static void move_relative_quad(bool toward, uint8_t r, uint8_t mob_quad_x,
                               uint8_t mob_quad_y, uint8_t target_quad_x,
                               uint8_t target_quad_y, uint8_t* next_quad_x,
                               uint8_t* next_quad_y) {
    // Move relative to the player
    //
    // Check all 4 directions, starting with a random one, and choose the
    // one that has the shortest distance
    uint8_t best_dist = 0xFF;
    if (toward) {
        best_dist = 0xFF;
    } else {
        best_dist = 0;
    }
#pragma clang loop unroll(full)
    for (uint8_t i = 0; i < 4; i++) {
        switch ((r + i & 0x3)) {
            case NORTH:
                if (mob_quad_y > 0 &&
                    map_tile_is_passable(mob_quad_x, mob_quad_y - 1)) {
                    uint8_t d = quad_distance(mob_quad_x, mob_quad_y - 1,
                                              target_quad_x, target_quad_y);
                    if ((toward && d < best_dist) ||
                        (!toward && d > best_dist)) {
                        best_dist = d;
                        *next_quad_x = mob_quad_x;
                        *next_quad_y = mob_quad_y - 1;
                    }
                }
                break;

            case SOUTH:
                if (mob_quad_y < MAP_HEIGHT_QUAD - 1 &&
                    map_tile_is_passable(mob_quad_x, mob_quad_y + 1)) {
                    uint8_t d = quad_distance(mob_quad_x, mob_quad_y + 1,
                                              target_quad_x, target_quad_y);
                    if ((toward && d < best_dist) ||
                        (!toward && d > best_dist)) {
                        best_dist = d;
                        *next_quad_x = mob_quad_x;
                        *next_quad_y = mob_quad_y + 1;
                    }
                }
                break;

            case EAST:
                if (mob_quad_x < MAP_WIDTH_QUAD - 1 &&
                    map_tile_is_passable(mob_quad_x + 1, mob_quad_y)) {
                    uint16_t d = quad_distance(mob_quad_x + 1, mob_quad_y,
                                               target_quad_x, target_quad_y);
                    if ((toward && d < best_dist) ||
                        (!toward && d > best_dist)) {
                        best_dist = d;
                        *next_quad_x = mob_quad_x + 1;
                        *next_quad_y = mob_quad_y;
                    }
                }
                break;

            case WEST:
                if (mob_quad_x > 0 &&
                    map_tile_is_passable(mob_quad_x - 1, mob_quad_y)) {
                    uint16_t d = quad_distance(mob_quad_x - 1, mob_quad_y,
                                               target_quad_x, target_quad_y);
                    if ((toward && d < best_dist) ||
                        (!toward && d > best_dist)) {
                        best_dist = d;
                        *next_quad_x = mob_quad_x - 1;
                        *next_quad_y = mob_quad_y;
                    }
                }
                break;
        }
    }
}

static void skeleton_reached_target(uint8_t idx) {
    uint8_t mob_quad_x = mob_get_quad_x(idx);
    uint8_t mob_quad_y = mob_get_quad_y(idx);
    uint8_t next_quad_x = mob_quad_x;
    uint8_t next_quad_y = mob_quad_y;

    uint8_t r = rand();
    if (r & 0xC) {
        move_relative_quad(true, r, mob_quad_x, mob_quad_y, player_get_quad_x(),
                           player_get_quad_y(), &next_quad_x, &next_quad_y);
    } else {
        random_move(mob_quad_x, mob_quad_y, &next_quad_x, &next_quad_y);
    }

    mob_set_target(idx, next_quad_x * QUAD_WIDTH_PX + QUAD_WIDTH_PX / 2,
                   next_quad_y * QUAD_HEIGHT_PX + QUAD_HEIGHT_PX / 2);
}

static void skeleton_archer_reached_target(uint8_t idx) {
    uint8_t mob_quad_x = mob_get_quad_x(idx);
    uint8_t mob_quad_y = mob_get_quad_y(idx);
    uint8_t next_quad_x = mob_quad_x;
    uint8_t next_quad_y = mob_quad_y;

    uint8_t r = rand();
    switch (r & 0x70) {
        case 0x00:
        case 0x10:
            // Move away from player
            move_relative_quad(false, r, mob_quad_x, mob_quad_y,
                               player_get_quad_x(), player_get_quad_y(),
                               &next_quad_x, &next_quad_y);
            break;

        case 0x20:
        case 0x30:
        case 0x40:
            // Move toward center of map
            move_relative_quad(true, r, mob_quad_x, mob_quad_y,
                               MAP_WIDTH_QUAD / 2, MAP_HEIGHT_QUAD / 2,
                               &next_quad_x, &next_quad_y);
            break;

        case 0x50:
        case 0x60:
        case 0x70:
            random_move(mob_quad_x, mob_quad_y, &next_quad_x, &next_quad_y);
            break;
    }

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

static void arrow_player_collision(uint8_t idx) {
    enum direction dir = arrow_get_direction(idx);
    uint16_t x = mob_get_map_x(idx);
    uint8_t y = mob_get_map_y(idx);
    kill_mob(idx);

    switch (dir) {
        case NORTH:
            if (player_dir == SOUTH && weapon_state != WEAPON_VISIBLE) {
                uint8_t arrow_idx = create_blocked_arrow(x, y);
                if (arrow_idx != MAX_MOBS) {
                    mob_set_target(arrow_idx, x + 5, y + 5);
                    mob_set_speed(arrow_idx, 1, 2);
                }
            } else {
                damage_player_push(1, 0, -1);
            }
            break;

        case SOUTH:
            if (player_dir == NORTH && weapon_state != WEAPON_VISIBLE) {
                uint8_t arrow_idx = create_blocked_arrow(x, y);
                if (arrow_idx != MAX_MOBS) {
                    mob_set_target(arrow_idx, x + 5, y - 5);
                    mob_set_speed(arrow_idx, 1, 2);
                }
            } else {
                damage_player_push(1, 0, 1);
            }
            break;

        case EAST:
            if (player_dir == WEST && weapon_state != WEAPON_VISIBLE) {
                uint8_t arrow_idx = create_blocked_arrow(x, y);
                if (arrow_idx != MAX_MOBS) {
                    mob_set_target(arrow_idx, x - 5, y + 5);
                    mob_set_speed(arrow_idx, 1, 2);
                }
            } else {
                damage_player_push(1, 1, 0);
            }
            break;

        case WEST:
            if (player_dir == EAST && weapon_state != WEAPON_VISIBLE) {
                uint8_t arrow_idx = create_blocked_arrow(x, y);
                if (arrow_idx != MAX_MOBS) {
                    mob_set_target(arrow_idx, x - 5, y + 5);
                    mob_set_speed(arrow_idx, 1, 2);
                }
            } else {
                damage_player_push(1, -1, 0);
            }
            break;
    }
}

static void shoot_arrow(uint8_t idx, enum direction direction,
                        uint16_t target_x, uint8_t target_y) {
    uint8_t arrow_idx = alloc_mob();

    if (arrow_idx == MAX_MOBS) {
        return;
    }
    create_arrow(arrow_idx, mob_get_map_x(idx), mob_get_map_y(idx), direction);

    mob_set_target(arrow_idx, target_x, target_y);
    mob_set_player_collision_handler(arrow_idx, arrow_player_collision);

    // 2 second cool down
    skeleton_archer.arrow_cool_down[idx] = 100;
}

static void skeleton_archer_update(uint8_t idx, uint8_t num_frames) {
    if (skeleton_archer.arrow_cool_down[idx] < num_frames) {
        skeleton_archer.arrow_cool_down[idx] = 0;
    } else {
        skeleton_archer.arrow_cool_down[idx] -= num_frames;
    }

    if (skeleton_archer.arrow_cool_down[idx] == 0) {
        if (SAME_QUAD_X(player_map_x, mob_get_map_x(idx))) {
            if (player_map_y < mob_get_map_y(idx)) {
                shoot_arrow(idx, NORTH, mob_get_map_x(idx), 0);
            } else {
                shoot_arrow(idx, SOUTH, mob_get_map_x(idx), MAP_HEIGHT_PX);
            }
        } else if (SAME_QUAD_Y(player_map_y, mob_get_map_y(idx))) {
            if (player_map_x < mob_get_map_x(idx)) {
                shoot_arrow(idx, WEST, 0, mob_get_map_y(idx));
            } else {
                shoot_arrow(idx, EAST, MAP_WIDTH_PX, mob_get_map_y(idx));
            }
        }
    }
}

static void on_skeleton_kill(uint8_t idx);

static bool new_skeleton(void) {
    uint8_t quad_x;
    uint8_t quad_y;

    uint8_t p;
    do {
        p = rand();
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

    if (p & 0x0C) {
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
    } else {
        uint8_t idx = create_skeleton_archer(x, y);
        if (idx == MAX_MOBS) {
            return false;
        }
        skeleton_archer.arrow_cool_down[idx] = 50;
        mob_set_death_handler(idx, on_skeleton_kill);
        mob_set_target(idx, x, y);
        mob_set_reached_target_handler(idx, skeleton_archer_reached_target);
        mob_set_player_collision_handler(idx, skeleton_player_collision);
        mob_set_update_handler(idx, skeleton_archer_update);

        skeleton_archer_reached_target(idx);
    }

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

    score = bcd_add_u16(score, BCD8(10));
    score_updated = true;

    switch (rand() & 0x3) {
        case 0:
            idx = create_coin(x, y, BCD8(1));
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

static void create_status_raster_cmd(void) {
    uint8_t idx = alloc_raster_cmd(STATUS_INT_LINE);
    raster_set_vicii_bg_color(idx, current_screen->bg_color_0);
    raster_set_vicii_ctrl_2(idx, DEFAULT_VICII_CTRL_2 | _BV(VICII_MCM_BIT));
}

static void create_done_raster_cmd(void) {
    uint8_t idx = alloc_raster_cmd(DONE_INT_LINE);
    raster_set_vicii_bg_color(idx, COLOR_BLACK);
    raster_set_vicii_ctrl_2(idx, DEFAULT_VICII_CTRL_2);
}

#ifdef DEBUG
static uint16_t get_raster(void) {
    return (VICII_CTRL_1 & _BV(VICII_RST8_BIT)) << 1 | VICII_RASTER;
}
#endif

static uint8_t fast_strlen(char* s) {
    uint8_t i;
    for (i = 0; s[i]; i++);
    return i;
}

static void pad_string(char* str, uint8_t size, uint8_t offset) {
    for (uint8_t i = offset; i < size - 1; i++) {
        str[i] = ' ';
    }

    str[size - 1] = '\0';
}

static void draw_current_weapon(void) {
    switch (player_get_weapon()) {
        case WEAPON_SWORD:
            put_char_xy(WEAPON_X_TILE + 1, WEAPON_Y_TILE, SWORD_LEFT_CHAR);
            set_color(WEAPON_X_TILE + 1, WEAPON_Y_TILE, COLOR_WHITE);
            put_char_xy(WEAPON_X_TILE + 2, WEAPON_Y_TILE, SWORD_RIGHT_CHAR);
            set_color(WEAPON_X_TILE + 2, WEAPON_Y_TILE, COLOR_WHITE);
            break;

        case WEAPON_FLAIL:
            put_char_xy(WEAPON_X_TILE + 1, WEAPON_Y_TILE, FLAIL_LEFT_CHAR);
            set_color(WEAPON_X_TILE + 1, WEAPON_Y_TILE, COLOR_GRAY2);
            put_char_xy(WEAPON_X_TILE + 2, WEAPON_Y_TILE, FLAIL_RIGHT_CHAR);
            set_color(WEAPON_X_TILE + 2, WEAPON_Y_TILE, COLOR_GRAY2);
            break;

        case WEAPON_BOW:
            put_char_xy(WEAPON_X_TILE + 1, WEAPON_Y_TILE, ARROW_LEFT_CHAR);
            set_color(WEAPON_X_TILE + 1, WEAPON_Y_TILE, COLOR_BROWN);
            put_char_xy(WEAPON_X_TILE + 2, WEAPON_Y_TILE, ARROW_RIGHT_CHAR);
            set_color(WEAPON_X_TILE + 2, WEAPON_Y_TILE, COLOR_BROWN);
            break;
    }
}

static bool draw_coins = false;
static char coin_string_buf[] = "$####";
static bool update_coin_string(void) {
    if (!player_coins_changed) {
        return false;
    }
    pad_string(&coin_string_buf[1], sizeof(coin_string_buf) - 1,
               u16_to_string(player_get_coins(), &coin_string_buf[1]));

    player_coins_changed = false;
    draw_coins = true;
    return true;
}

static bool draw_score = false;
static char score_string_buf[] = "    SCORE ####";
static bool update_score_string(void) {
    if (!score_updated) {
        return false;
    }
    u16_to_string(score, &score_string_buf[10]);
    score_updated = false;
    draw_score = true;
    return true;
}

static bool draw_health = false;
static char health_string_buf[(PLAYER_MAX_HEALTH / 2) + 1];
static bool update_health_string(void) {
    if (!player_health_changed) {
        return false;
    }
    for (uint8_t i = 0; i < PLAYER_MAX_HEALTH; i += 2) {
        if (player_health <= i) {
            health_string_buf[i / 2] = ' ';
        } else if (player_health == i + 1) {
            health_string_buf[i / 2] = HALF_HEART_CHAR;
        } else {
            health_string_buf[i / 2] = HEART_CHAR;
        }
    }
    player_health_changed = false;
    draw_health = true;
    return true;
}

#ifdef DEBUG
static char raster_avg_str[] = "    0X####";
static bool draw_raster_avg = false;
static bool update_raster_avg_string(uint16_t raster_avg, bool update) {
    if (update) {
        u16_to_string(raster_avg, &raster_avg_str[6]);
        draw_raster_avg = true;
        return true;
    }
    return false;
}
#else
#define update_raster_avg_string(avg, update) (false)
#endif

void game_loop(void) {
    memset(health_string_buf, ' ', sizeof(health_string_buf) - 1);
    health_string_buf[sizeof(health_string_buf) - 1] = '\0';

    score = 0;
    score_updated = true;
    player_health_changed = true;
    player_coins_changed = true;

    fill_color(HEALTH_X_TILE, HEALTH_Y_TILE,
               HEALTH_X_TILE + sizeof(health_string_buf) - 1, HEALTH_Y_TILE,
               COLOR_RED);

    fill_color(COIN_X_TILE, COIN_Y_TILE,
               COIN_X_TILE + sizeof(coin_string_buf) - 1, COIN_Y_TILE,
               COLOR_GREEN);

    fill_color(SCORE_TEXT_X - sizeof(score_string_buf), SCORE_TEXT_Y,
               SCORE_TEXT_X - 1, SCORE_TEXT_Y, COLOR_WHITE);

#ifdef DEBUG
    uint16_t raster_avg = 0;
    fill_color(SCORE_TEXT_X - sizeof(raster_avg_str), SCORE_TEXT_Y + 1,
               SCORE_TEXT_X - 1, SCORE_TEXT_Y + 1, COLOR_CYAN);
#endif

    put_char_xy(WEAPON_X_TILE, WEAPON_Y_TILE, '(');
    set_color(WEAPON_X_TILE, WEAPON_Y_TILE, COLOR_BLUE);
    put_char_xy(WEAPON_X_TILE + 3, WEAPON_Y_TILE, ')');
    set_color(WEAPON_X_TILE + 3, WEAPON_Y_TILE, COLOR_BLUE);
    draw_current_weapon();

    uint8_t delay_frame = 0;

    while (true) {
        // Wait for next frame interrupt
        DEBUG_COLOR(COLOR_BLACK);

#ifdef DEBUG
        uint16_t raster_wait_start = get_raster();
#endif
        frame_wait();
#ifdef DEBUG
        uint16_t raster_wait_end = get_raster();
        // There is a chance that we read the raster register right as it
        // rolls over. If so, it will be zero. Rather than deal with this,
        // ignore zeros
        if (raster_wait_start && raster_wait_end) {
            uint16_t elapsed = raster_wait_end - raster_wait_start;
            raster_avg = (raster_avg + elapsed) >> 1;
        }
#endif

        // Frame critical. These need to be done before the next frame
        // starts
        DEBUG_COLOR(COLOR_BLUE);

        if (draw_coins) {
            put_string_xy(COIN_X_TILE, COIN_Y_TILE, coin_string_buf);
            draw_coins = false;
        } else if (draw_score) {
            put_string_xy(SCORE_TEXT_X - fast_strlen(score_string_buf),
                          SCORE_TEXT_Y, score_string_buf);
            draw_score = false;
        } else if (draw_health) {
            put_string_xy(HEALTH_X_TILE, HEALTH_Y_TILE, health_string_buf);
            draw_health = false;
        }
#ifdef DEBUG
        else if (draw_raster_avg) {
            put_string_xy(SCORE_TEXT_X - fast_strlen(raster_avg_str),
                          SCORE_TEXT_Y + 1, raster_avg_str);
            draw_raster_avg = false;
        }
#endif

        DEBUG_COLOR(COLOR_RED);
        draw_player();

        DEBUG_COLOR(COLOR_YELLOW);
        DISABLE_INTERRUPTS() {
            prepare_raster_cmds();
            create_status_raster_cmd();
        }
        draw_mobs();
        DISABLE_INTERRUPTS() {
            create_done_raster_cmd();
            finish_raster_cmds();
        }

        update_sprite_pointers();

        // Frame non critical. These can be done during the frame since they
        // don't affect graphics

        // String updates. On NTSC, only update strings on "dead" frames where
        // the normal update tick is skipped to keep timing consistent with
        // PAL.
        //
        // On PAL, the strings are checked every frame, but only one of the
        // pending changes is performed, and the others will wait for a
        // subsequent frame
        DEBUG_COLOR(COLOR_ORANGE);
        delay_frame++;
        if (is_ntsc) {
            if (delay_frame >= 5) {
                delay_frame = 0;
                update_raster_avg_string(raster_avg, (frame_count & 0x3F) == 0);
                update_health_string();
                update_coin_string();
                update_score_string();

                // Update all MOBS to catch up if processing of callbacks is
                // behind
                for (uint8_t i = 0; i < MAX_MOBS; i++) {
                    update_mobs();
                }
                continue;
            }
        } else {
            update_raster_avg_string(raster_avg, (tick_count & 0x3F) == 0) ||
                update_health_string() || update_coin_string() ||
                update_score_string();
        }
        tick_count++;

        DEBUG_COLOR(COLOR_GREEN);
        tick_player();

        DEBUG_COLOR(COLOR_PURPLE);
        tick_mobs();

        if (player_health <= 0 && !player_temp_invulnerable) {
            break;
        }

        DEBUG_COLOR(COLOR_WHITE);
        switch (read_keyboard()) {
            case KEY_F1:
                player_set_weapon(WEAPON_SWORD);
                draw_current_weapon();
                break;

            case KEY_F3:
                player_set_weapon(WEAPON_FLAIL);
                draw_current_weapon();
                break;

            case KEY_F5:
                player_set_weapon(WEAPON_BOW);
                draw_current_weapon();
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
            DISABLE_INTERRUPTS() {
                ALL_RAM() { memcpy(dest, buffer, sizeof(buffer)); }
            }
            dest += sizeof(buffer);
        }
    }
    DISABLE_INTERRUPTS() {
        ALL_RAM() { memcpy(dest, buffer, i); }
    }
    cbm_k_close(15);
}

int main() {
    // Blank screen while setting up
    VICII_CTRL_1 &= ~_BV(VICII_DEN_BIT);

    load_data("GRAPHICS", &video_base);

    disable_interrupts();

    // Configure interrupts
    ISR_VECTOR = isr_handler;

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

    VICII_BG_0 = COLOR_BLACK;
    VICII_CTRL_1 = DEFAULT_VICII_CTRL_1;
    VICII_CTRL_2 = DEFAULT_VICII_CTRL_2;
    VICII_INTERRUPT_ENABLE = _BV(VICII_RST_BIT);

    prepare_raster_cmds();
    create_status_raster_cmd();
    create_done_raster_cmd();
    finish_raster_cmds();

    is_ntsc = (detect_video() != VIDEO_PAL_6596);

    enable_interrupts();

    init_mobs();

    frame_wait();

    while (true) {
        // Hide all sprites
        VICII_SPRITE_ENABLE = 0;

        fill_char(0, 0, SCREEN_WIDTH_TILE - 1, SCREEN_HEIGHT_TILE - 1,
                  BLANK_CHAR);
        fill_color(0, 0, SCREEN_WIDTH_TILE - 1, SCREEN_HEIGHT_TILE - 1,
                   COLOR_BLACK);
        draw_background(current_screen);

        player_map_x = PLAYER_START_X_QUAD * QUAD_WIDTH_PX + QUAD_WIDTH_PX / 2;
        player_map_y =
            PLAYER_START_Y_QUAD * QUAD_HEIGHT_PX + QUAD_HEIGHT_PX / 2;
        player_dir = SOUTH;
        player_weapon_damage = 1;
        player_health = 6;
        player_full_health = 6;
        player_set_coins(0);

        init_player();

        destroy_all_mobs();

        for (uint8_t i = 0; i < 6; i++) {
            new_skeleton();
        }

        fill_color(START_TEXT_X, START_TEXT_Y,
                   START_TEXT_X + sizeof(start_text), START_TEXT_Y,
                   COLOR_WHITE);
        put_string_xy(START_TEXT_X, START_TEXT_Y, start_text);

        wait_frames(30);

        // Wait for fire button to start
        while ((read_joystick_2() & _BV(JOYSTICK_FIRE_BIT)) == 0) {
            frame_wait();
        }

        fill_char(START_TEXT_X, START_TEXT_Y,
                  START_TEXT_X + sizeof(start_text) - 1, START_TEXT_Y, ' ');

        fill_char(GAME_OVER_TEXT_X, GAME_OVER_TEXT_Y,
                  GAME_OVER_TEXT_X + sizeof(game_over_text) - 1,
                  GAME_OVER_TEXT_Y, ' ');

        game_loop();

        fill_color(GAME_OVER_TEXT_X + 1, GAME_OVER_TEXT_Y,
                   GAME_OVER_TEXT_X + sizeof(game_over_text) - 1,
                   GAME_OVER_TEXT_Y, COLOR_RED);
        put_string_xy(GAME_OVER_TEXT_X + 1, GAME_OVER_TEXT_Y, game_over_text);
        wait_frames(4 * 60);
    }
}
