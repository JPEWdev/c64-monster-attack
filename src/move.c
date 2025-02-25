/*
 * SPDX-License-Identifier: MIT
 */
#include "move.h"

#include "draw.h"
#include "map.h"
#include "sprite.h"

#define BB_PAD_NORTH (4)
#define BB_PAD_SOUTH (4)
#define BB_PAD_EAST (4)
#define BB_PAD_WEST (4)

void check_move(uint16_t map_x, uint8_t map_y, int8_t* move_x, int8_t* move_y) {
    int8_t check_x = 0;
    int8_t check_y = 0;
    if (*move_x < 0) {
        check_x = *move_x - BB_PAD_WEST;
        if (-check_x > map_x) {
            check_x = 0;
            *move_x = 0;
        }
    } else if (*move_x > 0) {
        check_x = *move_x + BB_PAD_EAST;
        if (map_x > MAP_WIDTH_PX - check_x) {
            check_x = 0;
            *move_x = 0;
        }
    }

    if (*move_y < 0) {
        check_y = *move_y - BB_PAD_NORTH;
        if (-check_y > map_y) {
            check_y = 0;
            *move_y = 0;
        }
    } else if (*move_y > 0) {
        check_y = *move_y + BB_PAD_EAST;
        if (map_y > MAP_HEIGHT_PX - check_y) {
            check_y = 0;
            *move_y = 0;
        }
    }

    if (!*move_x && !*move_y) {
        return;
    }

    if (!map_tile_is_passable((map_x + check_x) / QUAD_HEIGHT_PX,
                              map_y / QUAD_HEIGHT_PX)) {
        *move_x = 0;
    }

    if (!map_tile_is_passable(map_x / QUAD_WIDTH_PX,
                              (map_y + check_y) / QUAD_HEIGHT_PX)) {
        *move_y = 0;
    }
}
