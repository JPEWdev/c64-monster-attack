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

#endif
