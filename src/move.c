/*
 * SPDX-License-Identifier: MIT
 */
#include "move.h"

#include "draw.h"
#include "map.h"
#include "sprite.h"

bool check_quad(uint16_t x, uint16_t y) {
    int8_t quad_x;
    int8_t quad_y;
    position_to_quad(x, y, &quad_x, &quad_y);

    if (quad_x < 0 || quad_y < 0) {
        return false;
    }

    return map_tile_is_passable(quad_x, quad_y);
}

#define BB_PAD_NORTH (4)
#define BB_PAD_SOUTH (4)
#define BB_PAD_EAST (4)
#define BB_PAD_WEST (4)

void check_move(uint16_t x, uint16_t y, int8_t* move_x, int8_t* move_y) {
    uint8_t bb_check = 0;
    int8_t tx = *move_x;
    int8_t ty = *move_y;

    if (tx > 0) {
        bb_check |= _BV(EAST);
    } else if (tx < 0) {
        bb_check |= _BV(WEST);
    }

    if (ty > 0) {
        bb_check |= _BV(SOUTH);
    } else if (ty < 0) {
        bb_check |= _BV(NORTH);
    }

    switch (bb_check) {
        case _BV(NORTH):
            if (!check_quad(x + BB_PAD_EAST + tx, y - BB_PAD_NORTH + ty) ||
                !check_quad(x - BB_PAD_WEST + tx, y - BB_PAD_NORTH + ty)) {
                *move_y = 0;
            }
            break;

        case _BV(SOUTH):
            if (!check_quad(x + BB_PAD_EAST + tx, y + BB_PAD_SOUTH + ty) ||
                !check_quad(x - BB_PAD_WEST + tx, y + BB_PAD_SOUTH + ty)) {
                *move_y = 0;
            }
            break;

        case _BV(EAST):
            if (!check_quad(x + BB_PAD_EAST + tx, y - BB_PAD_NORTH + ty) ||
                !check_quad(x + BB_PAD_EAST + tx, y + BB_PAD_SOUTH + ty)) {
                *move_x = 0;
            }
            break;

        case _BV(WEST):
            if (!check_quad(x - BB_PAD_WEST + tx, y - BB_PAD_NORTH + ty) ||
                !check_quad(x - BB_PAD_WEST + tx, y + BB_PAD_SOUTH + ty)) {
                *move_x = 0;
            }
            break;

        case _BV(NORTH) | _BV(EAST):
            if (!check_quad(x + BB_PAD_EAST + tx, y - BB_PAD_NORTH + ty)) {
                *move_x = 0;
                *move_y = 0;
            }
            break;

        case _BV(NORTH) | _BV(WEST):
            if (!check_quad(x - BB_PAD_WEST + tx, y - BB_PAD_NORTH + ty)) {
                *move_x = 0;
                *move_y = 0;
            }
            break;

        case _BV(SOUTH) | _BV(EAST):
            if (!check_quad(x + BB_PAD_EAST + tx, y + BB_PAD_SOUTH + ty)) {
                *move_x = 0;
                *move_y = 0;
            }
            break;

        case _BV(SOUTH) | _BV(WEST):
            if (!check_quad(x - BB_PAD_WEST + tx, y + BB_PAD_SOUTH + ty)) {
                *move_x = 0;
                *move_y = 0;
            }
            break;
    }
}

bool check_move_fast(uint16_t x, uint16_t y, int8_t move_x, int8_t move_y) {
    int8_t new_quad_x;
    int8_t new_quad_y;
    position_to_quad(x + move_x, y + move_y, &new_quad_x, &new_quad_y);

    if (new_quad_x < 0 || new_quad_y < 0) {
        return false;
    }

    return map_tile_is_passable(new_quad_x, new_quad_y);
}
