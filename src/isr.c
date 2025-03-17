/*
 * SPDX-License-Identifier: MIT
 */
#include "isr.h"

#include <cbm.h>
#include <stdint.h>

#include "player.h"
#include "reg.h"
#include "util.h"

#define RASTER_VICII_BG _BV(0)
#define RASTER_VICII_CTRL_2 _BV(1)
#define RASTER_LAST _BV(2)
#define RASTER_SPRITE _BV(3)

uint8_t frame_count = 0;

uint8_t __zeropage raster_cmd_idx;

uint8_t __zeropage vicii_bg_0_next[MAX_RASTER_CMDS];
uint8_t __zeropage vicii_ctrl_2_next[MAX_RASTER_CMDS];
uint8_t __zeropage vicii_raster_next[MAX_RASTER_CMDS];
uint8_t __zeropage raster_flags[MAX_RASTER_CMDS];
uint8_t __zeropage raster_sprite_idx[MAX_RASTER_CMDS];
uint8_t __zeropage raster_sprite_pointer[MAX_RASTER_CMDS];
uint8_t __zeropage raster_sprite_color[MAX_RASTER_CMDS];
uint8_t __zeropage raster_sprite_x[MAX_RASTER_CMDS];
uint8_t __zeropage raster_sprite_y[MAX_RASTER_CMDS];
uint8_t __zeropage raster_sprite_msb[MAX_RASTER_CMDS];
uint8_t __zeropage raster_sprite_x_expand[MAX_RASTER_CMDS];
uint8_t __zeropage raster_sprite_y_expand[MAX_RASTER_CMDS];
uint8_t __zeropage raster_sprite_multicolor[MAX_RASTER_CMDS];
uint8_t num_missed_sprites;
uint8_t last_num_missed_sprites;

static uint8_t first_line;

void frame_wait(void) {
    static uint8_t next_frame = 0;
    while (*(volatile uint8_t*)&frame_count == next_frame);
    next_frame = frame_count;
}

void wait_frames(uint16_t num_frames) {
    for (; num_frames; num_frames--) {
        frame_wait();
    }
}

void prepare_raster_cmds(void) {
    raster_cmd_idx = 0xFF;
    last_num_missed_sprites = num_missed_sprites;
    num_missed_sprites = 0;
}

void finish_raster_cmds(void) {
    vicii_raster_next[raster_cmd_idx] = first_line;
    raster_flags[raster_cmd_idx] |= RASTER_LAST;
    raster_cmd_idx = 0;
    VICII_INTERRUPT_ENABLE |= _BV(VICII_RST_BIT);
}

uint8_t alloc_raster_cmd(uint8_t raster_line) {
    if (raster_cmd_idx == 0xFF) {
        VICII_RASTER = raster_line;
        VICII_CTRL_1 &= ~_BV(VICII_RST8_BIT);
        first_line = raster_line;
    } else {
        vicii_raster_next[raster_cmd_idx] = raster_line;
    }
    raster_cmd_idx++;
    raster_flags[raster_cmd_idx] = 0;
    return raster_cmd_idx;
}

void raster_set_vicii_bg_color(uint8_t idx, uint8_t bg_color) {
    raster_flags[idx] |= RASTER_VICII_BG;
    vicii_bg_0_next[idx] = bg_color;
}

void raster_set_vicii_ctrl_2(uint8_t idx, uint8_t ctrl_2) {
    raster_flags[idx] |= RASTER_VICII_CTRL_2;
    vicii_ctrl_2_next[idx] = ctrl_2;
}

void raster_set_sprite(uint8_t idx, uint8_t sprite_idx, uint8_t sprite_pointer,
                       uint8_t sprite_x, uint8_t sprite_y, uint8_t color,
                       uint8_t sprite_msb, uint8_t sprite_x_expand,
                       uint8_t sprite_y_expand, uint8_t sprite_multicolor) {
    raster_flags[idx] |= RASTER_SPRITE;
    raster_sprite_idx[idx] = sprite_idx;
    raster_sprite_pointer[idx] = sprite_pointer;
    raster_sprite_x[idx] = sprite_x;
    raster_sprite_y[idx] = sprite_y;
    raster_sprite_color[idx] = color;
    raster_sprite_msb[idx] = sprite_msb;
    raster_sprite_x_expand[idx] = sprite_x_expand;
    raster_sprite_y_expand[idx] = sprite_y_expand;
    raster_sprite_multicolor[idx] = sprite_multicolor;
}
