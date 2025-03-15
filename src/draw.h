/*
 * SPDX-License-Identifier: MIT
 */
#ifndef _DRAW_H
#define _DRAW_H

#include <stdbool.h>
#include <stdint.h>

struct map_screen;

uint16_t get_raster(void);

void put_char_xy(uint8_t x, uint8_t y, uint8_t c);
void put_string_xy(uint8_t x, uint8_t y, char const *c);
void put_char_xy_color(uint8_t x, uint8_t y, uint8_t c, uint8_t color);
void put_string_xy_color(uint8_t x, uint8_t y, char const *c, uint8_t color);
void set_color(uint8_t x, uint8_t y, uint8_t color);
void fill_char(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t c);
void fill_color(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color);
void draw_map_quad(uint8_t x, uint8_t y, uint8_t quad, uint8_t color);
void draw_background(struct map_screen const *screen);
void position_to_quad(uint16_t x, uint16_t y, int8_t *qx, int8_t *qy);

#endif
