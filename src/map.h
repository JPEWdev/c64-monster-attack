/*
 * SPDX-License-Identifier: MIT
 */
#ifndef MAP_H
#define MAP_H

#include <stdbool.h>
#include <stdint.h>

#include "reg.h"

// Width of the map in quads
#define MAP_WIDTH_QUAD (19)

// height of the map, in quads
#define MAP_HEIGHT_QUAD (11)

// Map size in tiles
#define MAP_WIDTH_TILE (MAP_WIDTH_QUAD * 2)
#define MAP_HEIGHT_TILE (MAP_HEIGHT_QUAD * 2)

// Map size in pixels
#define MAP_WIDTH_PX (MAP_WIDTH_TILE * 8)
#define MAP_HEIGHT_PX (MAP_HEIGHT_TILE * 8)

// Map offsets in tiles
#define MAP_OFFSET_X_TILE ((SCREEN_WIDTH_TILE / 2) - MAP_WIDTH_QUAD)
#define MAP_OFFSET_Y_TILE (SCREEN_HEIGHT_TILE - (MAP_HEIGHT_QUAD * 2))

// Map offsets in pixels
#define MAP_OFFSET_X_PX ((MAP_OFFSET_X_TILE * 8) + BORDER_WIDTH_PX)
#define MAP_OFFSET_Y_PX ((MAP_OFFSET_Y_TILE * 8) + BORDER_HEIGHT_PX)

// Size of a quad in pixels
#define QUAD_WIDTH_PX (16)
#define QUAD_HEIGHT_PX (16)

#define MAKE_LEGEND(image, color) ((image) | (color << 5))
#define LEGEND_IMAGE(l) ((l) & 0x1F)
#define LEGEND_COLOR(l) ((l) >> 5)

#define TILE_IS_PASSABLE(t) ((t) & 0x80)
#define TILE_LEGEND_IDX(t) ((t) & 0x7F)

#define QUAD_X_TO_PX(x) (MAP_OFFSET_X_PX + ((x) * QUAD_WIDTH_PX))
#define QUAD_Y_TO_PX(y) (MAP_OFFSET_Y_PX + ((y) * QUAD_WIDTH_PX))

#define SAME_QUAD_X(x1, x2) \
    (((x1) & ~(QUAD_WIDTH_PX - 1)) == ((x2) & ~(QUAD_WIDTH_PX - 1)))

#define SAME_QUAD_Y(y1, y2) \
    (((y1) & ~(QUAD_HEIGHT_PX - 1)) == ((y2) & ~(QUAD_HEIGHT_PX - 1)))

enum {
    MAP_IMAGE_TREE,
    MAP_IMAGE_ROCK,
    MAP_IMAGE_STONE,

    // Special Indexes
    MAP_IMAGE_BLANK,
    MAP_IMAGE_SOLID,
    MAP_IMAGE_WAVES,
    MAP_IMAGE_BRICKS,
    MAP_IMAGE_SHORE_NORTH,
    MAP_IMAGE_SHORE_NORTH_EAST,
    MAP_IMAGE_SHORE_NORTH_WEST,
    MAP_IMAGE_SHORE_EAST,
    MAP_IMAGE_SHORE_WEST,
    MAP_IMAGE_SHORE_SOUTH,
    MAP_IMAGE_SHORE_SOUTH_EAST,
    MAP_IMAGE_SHORE_SOUTH_WEST,
    MAP_IMAGE_POOL,
};

struct map_screen {
    uint8_t bg_color_0;
    uint8_t bg_color_1;
    uint8_t bg_color_2;
    uint8_t const* legend;
    uint8_t tiles[MAP_HEIGHT_QUAD][MAP_WIDTH_QUAD];
};

extern const struct map_screen* current_screen;

bool map_tile_is_passable(uint8_t x, uint8_t y);
uint8_t map_tile_get_image(uint8_t x, uint8_t y);
uint8_t map_tile_get_color(uint8_t x, uint8_t y);

extern const struct map_screen main_screen;

#endif

