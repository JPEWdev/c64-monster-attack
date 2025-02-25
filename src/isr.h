/*
 * SPDX-License-Identifier: MIT
 */
#ifndef _ISR_H
#define _ISR_H

#include <stdint.h>

extern uint8_t volatile interrupt_count;
extern uint8_t frame_count;
extern uint8_t __zeropage vicii_ctrl_1_next;
extern uint8_t __zeropage vicii_raster_next;
extern uint8_t __zeropage vicii_ctrl_2_next;
extern uint8_t __zeropage vicii_bg_0_next;

void isr_handler(void);

#endif
