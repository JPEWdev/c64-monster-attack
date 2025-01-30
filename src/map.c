/*
 * SPDX-License-Identifier: MIT
 */
#include "map.h"

const struct map_screen* current_screen;

bool map_tile_is_passable(uint8_t x, uint8_t y) {
    return (current_screen->tiles[y][x] & 0x80) != 0;
}

uint8_t map_tile_get_image(uint8_t x, uint8_t y) {
    uint8_t idx = TILE_LEGEND_IDX(current_screen->tiles[y][x]);
    return LEGEND_IMAGE(current_screen->legend[idx]);
}

uint8_t map_tile_get_color(uint8_t x, uint8_t y) {
    uint8_t idx = TILE_LEGEND_IDX(current_screen->tiles[y][x]);
    return LEGEND_COLOR(current_screen->legend[idx]);
}

