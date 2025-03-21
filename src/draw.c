/*
 * SPDX-License-Identifier: MIT
 */
#include "draw.h"

#include <cbm.h>
#include <stdbool.h>
#include <string.h>

#include "chars.h"
#include "isr.h"
#include "map.h"
#include "reg.h"
#include "util.h"

uint16_t get_raster(void) {
    return (VICII_CTRL_1 & _BV(VICII_RST8_BIT)) << 1 | VICII_RASTER;
}

void put_char_xy(uint8_t x, uint8_t y, uint8_t c) {
    DISABLE_INTERRUPTS() {
        ALL_RAM() { screen_data[y][x] = c; }
    }
}

void put_string_xy(uint8_t x, uint8_t y, char const *c) {
    uint8_t *p = &screen_data[y][x];
    DISABLE_INTERRUPTS() {
        ALL_RAM() {
            while (*c) {
                *p = *c;
                p++;
                c++;
            }
        }
    }
}

void put_char_xy_color(uint8_t x, uint8_t y, uint8_t c, uint8_t color) {
    put_char_xy(x, y, c);
    set_color(x, y, color);
}

void put_string_xy_color(uint8_t x, uint8_t y, char const *c, uint8_t color) {
    put_string_xy(x, y, c);
    fill_color(x, y, x + strlen(c), y, color);
}

void set_color(uint8_t x, uint8_t y, uint8_t color) {
    color_data[y][x] = color;
}

void fill_char(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t c) {
    DISABLE_INTERRUPTS() {
        ALL_RAM() {
            for (uint8_t y = y1; y <= y2; y++) {
                memset(&screen_data[y][x1], c, (x2 - x1) + 1);
            }
        }
    }
}

void fill_color(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color) {
    for (uint8_t y = y1; y <= y2; y++) {
        memset(&color_data[y][x1], color, (x2 - x1) + 1);
    }
}

static void put_quad_chars(uint8_t x, uint8_t y, uint8_t nw, uint8_t ne,
                           uint8_t sw, uint8_t se) {
    put_char_xy(x, y, nw);
    put_char_xy(x + 1, y, ne);
    put_char_xy(x, y + 1, sw);
    put_char_xy(x + 1, y + 1, se);
}

void draw_map_quad(uint8_t x, uint8_t y, uint8_t image, uint8_t color) {
    x = (x << 1) + MAP_OFFSET_X_TILE;
    y = (y << 1) + MAP_OFFSET_Y_TILE;
    fill_color(x, y, x + 1, y + 1, 0x8 | color);

    switch (image) {
        case MAP_IMAGE_BLANK:
            put_quad_chars(x, y, 0x20, 0x20, 0x20, 0x20);
            break;

        case MAP_IMAGE_SOLID:
            put_quad_chars(x, y, FILL_CHAR, FILL_CHAR, FILL_CHAR, FILL_CHAR);
            break;

        case MAP_IMAGE_BRICKS:
            put_quad_chars(x, y, BRICKS_1_CHAR, BRICKS_1_CHAR, BRICKS_2_CHAR,
                           BRICKS_2_CHAR);
            break;

        case MAP_IMAGE_WAVES:
            put_quad_chars(x, y, WAVES_1_CHAR, WAVES_2_CHAR, WAVES_1_CHAR,
                           WAVES_2_CHAR);
            break;

        case MAP_IMAGE_SHORE_NORTH:
            put_quad_chars(x, y, SHORE_NORTH_CHAR, SHORE_NORTH_CHAR, FILL_CHAR,
                           FILL_CHAR);
            break;

        case MAP_IMAGE_SHORE_NORTH_EAST:
            put_quad_chars(x, y, SHORE_NORTH_CHAR, SHORE_NORTH_EAST_CHAR,
                           FILL_CHAR, SHORE_EAST_CHAR);
            break;

        case MAP_IMAGE_SHORE_NORTH_WEST:
            put_quad_chars(x, y, SHORE_NORTH_WEST_CHAR, SHORE_NORTH_CHAR,
                           SHORE_WEST_CHAR, FILL_CHAR);
            break;

        case MAP_IMAGE_SHORE_EAST:
            put_quad_chars(x, y, FILL_CHAR, SHORE_EAST_CHAR, FILL_CHAR,
                           SHORE_EAST_CHAR);
            break;

        case MAP_IMAGE_SHORE_WEST:
            put_quad_chars(x, y, SHORE_WEST_CHAR, FILL_CHAR, SHORE_WEST_CHAR,
                           FILL_CHAR);
            break;

        case MAP_IMAGE_SHORE_SOUTH:
            put_quad_chars(x, y, FILL_CHAR, FILL_CHAR, SHORE_SOUTH_CHAR,
                           SHORE_SOUTH_CHAR);
            break;

        case MAP_IMAGE_SHORE_SOUTH_EAST:
            put_quad_chars(x, y, FILL_CHAR, SHORE_EAST_CHAR, SHORE_SOUTH_CHAR,
                           SHORE_SOUTH_EAST_CHAR);
            break;

        case MAP_IMAGE_SHORE_SOUTH_WEST:
            put_quad_chars(x, y, SHORE_WEST_CHAR, FILL_CHAR,
                           SHORE_SOUTH_WEST_CHAR, SHORE_SOUTH_CHAR);
            break;

        case MAP_IMAGE_POOL:
            put_quad_chars(x, y, SHORE_NORTH_WEST_CHAR, SHORE_NORTH_EAST_CHAR,
                           SHORE_SOUTH_WEST_CHAR, SHORE_SOUTH_EAST_CHAR);
            break;

        default: {
            uint8_t q = 128 + (image << 2);
            put_quad_chars(x, y, q, q + 1, q + 2, q + 3);
        } break;
    }
}

void draw_background(struct map_screen const *screen) {
    VICII_BG_1 = screen->bg_color_1;
    VICII_BG_2 = screen->bg_color_2;

    // Fill any parts of the map area not drawn with black, including the
    // transition between the status bar and the map
    fill_char(0, MAP_OFFSET_Y_TILE - 1, SCREEN_WIDTH_TILE - 1,
              SCREEN_HEIGHT_TILE - 1, FILL_CHAR);
    fill_color(0, MAP_OFFSET_Y_TILE - 1, SCREEN_WIDTH_TILE - 1,
               SCREEN_HEIGHT_TILE - 1, COLOR_BLACK);

    for (uint8_t y = 0; y < MAP_HEIGHT_QUAD; y++) {
        for (uint8_t x = 0; x < MAP_WIDTH_QUAD; x++) {
            draw_map_quad(x, y, map_tile_get_image(x, y),
                          map_tile_get_color(x, y));
        }
    }
}

void position_to_quad(uint16_t x, uint16_t y, int8_t *qx, int8_t *qy) {
    if (x < MAP_OFFSET_X_PX) {
        *qx = -1;
    } else if (x >= MAP_OFFSET_X_PX + MAP_WIDTH_PX) {
        *qx = -1;
    } else {
        *qx = (x - MAP_OFFSET_X_PX) / QUAD_WIDTH_PX;
    }

    if (y < MAP_OFFSET_Y_PX) {
        *qy = -1;
    } else if (y >= MAP_OFFSET_Y_PX + MAP_HEIGHT_PX) {
        *qy = -1;
    } else {
        *qy = (y - MAP_OFFSET_Y_PX) / QUAD_HEIGHT_PX;
    }
}

