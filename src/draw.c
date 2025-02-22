/*
 * SPDX-License-Identifier: MIT
 */
#include "draw.h"

#include <stdbool.h>
#include <string.h>

#include "isr.h"
#include "map.h"
#include "reg.h"
#include "util.h"

void put_char_xy(uint8_t x, uint8_t y, uint8_t c) {
    ALL_RAM() { screen_data[y][x] = c; }
}

void put_string_xy(uint8_t x, uint8_t y, uint8_t color, char const *c) {
    while (*c) {
        switch (*c & 0x70) {
            case 0x00:
            case 0x10:
                // Control Character
                put_char_xy(x, y, 0x20);
                break;

            case 0x20:
                // Symbols
                put_char_xy(x, y, *c);
                break;

            case 0x30:
                // Numbers
                put_char_xy(x, y, *c);
                break;

            case 0x40:
            case 0x50:
                // Upper case
                put_char_xy(x, y, *c & 0x1F);
                break;

            case 0x60:
            case 0x70:
                // Lower case
                put_char_xy(x, y, *c & 0x1F);
                break;
        }
        set_color(x, y, color);
        x++;
        c++;
    }
}

void set_color(uint8_t x, uint8_t y, uint8_t color) {
    color_data[y][x] = color;
}

void fill_char(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t c) {
    ALL_RAM() {
        for (uint8_t y = y1; y <= y2; y++) {
            memset(&screen_data[y][x1], c, (x2 - x1) + 1);
        }
    }
}

void fill_color(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color) {
    for (uint8_t y = y1; y <= y2; y++) {
        memset(&color_data[y][x1], color, (x2 - x1) + 1);
    }
}

#define SHORE_EAST_CHAR (0x73)
#define SHORE_SOUTH_EAST_CHAR (0x74)
#define SHORE_SOUTH_CHAR (0x75)
#define SHORE_SOUTH_WEST_CHAR (0x76)
#define SHORE_WEST_CHAR (0x77)
#define SHORE_NORTH_WEST_CHAR (0x78)
#define SHORE_NORTH_CHAR (0x79)
#define SHORE_NORTH_EAST_CHAR (0x7A)
#define WAVES_1_CHAR (0x7B)
#define WAVES_2_CHAR (0x7C)
#define BRICKS_1_CHAR (0x7D)
#define BRICKS_2_CHAR (0x7E)
#define FILL_CHAR (0x7F)

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
    vicii_bg_0_next = screen->bg_color_0;
    VICII_BG_1 = screen->bg_color_1;
    VICII_BG_2 = screen->bg_color_2;

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

