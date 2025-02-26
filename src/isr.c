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

uint8_t frame_count = 0;

uint8_t __zeropage raster_cmd_idx;

uint8_t __zeropage vicii_bg_0_next[MAX_RASTER_CMDS];
uint8_t __zeropage vicii_ctrl_2_next[MAX_RASTER_CMDS];
uint8_t __zeropage vicii_raster_next[MAX_RASTER_CMDS];
uint8_t __zeropage raster_flags[MAX_RASTER_CMDS];

static uint8_t first_line;

void prepare_raster_cmds(void) { raster_cmd_idx = 0xFF; }

void finish_raster_cmds(void) {
    vicii_raster_next[raster_cmd_idx] = first_line;
    raster_flags[raster_cmd_idx] |= RASTER_LAST;
    raster_cmd_idx = 0;
}

uint8_t alloc_raster_cmd(uint8_t raster_line) {
    if (raster_cmd_idx == 0xFF) {
        VICII_RASTER = raster_line;
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
