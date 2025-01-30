/*
 * SPDX-License-Identifier: MIT
 */
#ifndef _MOVE_H
#define _MOVE_H

#include <stdbool.h>
#include <stdint.h>

bool check_quad(uint16_t x, uint16_t y);
void check_move(uint16_t x, uint16_t y, int8_t *move_x, int8_t *move_y);
bool check_move_fast(uint16_t x, uint16_t y, int8_t move_x, int8_t move_y);

#endif
