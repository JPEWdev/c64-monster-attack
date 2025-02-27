/*
 * SPDX-License-Identifier: MIT
 */
#ifndef _ISR_H
#define _ISR_H

#include <stdint.h>

#define MAX_RASTER_CMDS (4)

extern uint8_t frame_count;

void isr_handler(void);

void prepare_raster_cmds(void);
void finish_raster_cmds(void);

uint8_t alloc_raster_cmd(uint8_t raster_line);

void raster_set_vicii_bg_color(uint8_t idx, uint8_t bg_color);
void raster_set_vicii_ctrl_2(uint8_t idx, uint8_t ctrl_1);
void raster_set_sprite(uint8_t idx, uint8_t sprite_idx, uint8_t sprite_pointer,
                       uint8_t sprite_x, uint8_t sprite_y, uint8_t color,
                       uint8_t sprite_msb, uint8_t sprite_x_expand,
                       uint8_t sprite_y_expand, uint8_t multicolor);

#endif
