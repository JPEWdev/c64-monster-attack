/*
 * SPDX-License-Identifier: MIT
 */
#include "isr.h"

#include <cbm.h>
#include <stdint.h>

#include "player.h"
#include "reg.h"
#include "util.h"

uint8_t volatile interrupt_count = 0;
uint8_t frame_count = 0;

uint8_t __zeropage vicii_bg_0_next;
uint8_t __zeropage vicii_ctrl_1_next;
uint8_t __zeropage vicii_ctrl_2_next;
uint8_t __zeropage vicii_raster_next;
