/*
 * SPDX-License-Identifier: MIT
 */
#ifndef _REG_H
#define _REG_H

#include <stdint.h>

#define REG(addr) (*((uint8_t volatile *)addr))
#define REG_ARRAY(addr) ((uint8_t volatile *)addr)

typedef void (*isr_proc)(void);
#define ISR_ADDR (*((isr_proc volatile *)0xFFFE))

#define PROCESSOR_DDR REG(0x0000)
#define MEMORY_MASK (0x07)
#define MEMORY_RAM_ONLY (0x00)
#define MEMORY_BASIC_OFF_KERNAL_OFF (0x01)
#define MEMORY_BASIC_OFF_KERNAL_ON (0x02)
#define MEMORY_BASIC_ON_KERNAL_ON (0x03)
#define MEMORY_IO_BIT (2)
#define PROCESSOR_PORT REG(0x0001)

#define VICII_SPRITE_POSITION \
    ((struct {                \
        uint8_t x;            \
        uint8_t y;            \
    } volatile *)0xd000)

#define VICII_SPRITE_MSB REG(0xd010)

#define VICII_RST8_BIT (7)
#define VICII_ECM_BIT (6)
#define VICII_BMM_BIT (5)
#define VICII_DEN_BIT (4)
#define VICII_RSEL_BIT (3)
#define VICII_YSCROLL_BIT (0)
#define VICII_YSCROLL_MASK (0x07)
#define VICII_CTRL_1 REG(0xd011)

#define VICII_RASTER REG(0xd012)

#define VICII_LIGHT_PEN_X REG(0xd013)

#define VICII_LIGHT_PEN_Y REG(0xd014)

#define VICII_SPRITE_ENABLE REG(0xd015)

#define VICII_RES_BIT (5)
#define VICII_MCM_BIT (4)
#define VICII_CSEL_BIT (3)
#define VICII_XSCROLL_BIT (0)
#define VICII_XSCROLL_MASK (0x07)

#define VICII_CTRL_2 REG(0xd016)

#define VICII_SPRITE_Y_EXPAND REG(0xd017)

#define VICII_VM_BIT (4)
#define VICII_VM_MASK (0xF0)
#define VICII_CB_BIT (1)
#define VICII_CB_MASK (0x0E)

#define VICII_MEM_PTR REG(0xd018)

#define VICII_LP_BIT (3)
#define VICII_MMC_BIT (2)
#define VICII_MBC_BIT (1)
#define VICII_RST_BIT (0)
#define VICII_INTERRUPT REG(0xd019)
#define VICII_INTERRUPT_ENABLE REG(0xd01a)

#define VICII_SPRITE_MULTICOLOR REG(0xd01c)

#define VICII_SPRITE_X_EXPAND REG(0xd01d)

#define VICII_SPRITE_COLLISION_MMC REG(0xd01e)

#define VICII_SPRITE_COLLISION_MBC REG(0xd01f)

#define VICII_BORDER_COLOR REG(0xd020)
#define VICII_BG_0 REG(0xd021)
#define VICII_BG_1 REG(0xd022)
#define VICII_BG_2 REG(0xd023)
#define VICII_BG_3 REG(0xd024)

#define VICII_SPRITE_MULTICOLOR_0 REG(0xd025)
#define VICII_SPRITE_MULTICOLOR_1 REG(0xd026)

#define VICII_SPRITE_COLOR REG_ARRAY(0xd027)
#define VICII_SPRITE_0_COLOR (uint8_t *)(0xd027)

#define CIA_PORT_A (0x0)
#define CIA_PORT_B (0x1)
#define CIA_DDR_A (0x2)
#define CIA_DDR_B (0x3)
#define CIA_TIMER_A_LO (0x4)
#define CIA_TIMER_A_HI (0x5)
#define CIA_TIMER_B_LO (0x6)
#define CIA_TIMER_B_HI (0x7)
#define CIA_TOD_TENTHS_BCD (0x8)
#define CIA_TOD_SECONDS_BCD (0x9)
#define CIA_TOD_MINUTES_BCD (0xA)
#define CIA_TOD_HOURS_BCD (0xB)
#define CIA_SERIAL_SHIFT (0xC)

#define CIA_INTERRUPT_TIMER_A_UNDERFLOW_BIT (0)
#define CIA_INTERRUPT_TIMER_B_UNDERFLOW_BIT (1)
#define CIA_INTERRUPT_TOD_BIT (2)
#define CIA_INTERRUPT_SERIAL_SHIFT_BIT (3)
#define CIA_INTERRUPT_FLAG_BIT (4)
#define CIA_INTERRUPT_FILL_BIT (7)
#define CIA_INTERRUPT (0xD)
#define CIA_TIMER_A_CTRL (0xE)
#define CIA_TIMER_B_CTRL (0xF)

#define CIA_REG(base, offset) REG((base) + CIA_##offset)

#define CIA_1_REG(offset) CIA_REG(0xdc00, offset)
#define CIA_1_PORT_A CIA_1_REG(PORT_A)
#define CIA_1_PORT_B CIA_1_REG(PORT_B)
#define CIA_1_DDR_A CIA_1_REG(DDR_A)
#define CIA_1_DDR_B CIA_1_REG(DDR_B)
#define CIA_1_TIMER_A_LO CIA_1_REG(TIMER_A_LO)
#define CIA_1_TIMER_A_HI CIA_1_REG(TIMER_A_HI)
#define CIA_1_TIMER_B_LO CIA_1_REG(TIMER_B_LO)
#define CIA_1_TIMER_B_HI CIA_1_REG(TIMER_B_HI)
#define CIA_1_TOD_TENTHS_BCD CIA_1_REG(TOD_TENTHS_BCD)
#define CIA_1_TOD_SECONDS_BCD CIA_1_REG(TOD_SECONDS_BCD)
#define CIA_1_TOD_MINUTES_BCD CIA_1_REG(TOD_MINUTES_BCD)
#define CIA_1_TOD_HOURS_BCD CIA_1_REG(TOD_HOURS_BCD)
#define CIA_1_SERIAL_SHIFT CIA_1_REG(SERIAL_SHIFT)
#define CIA_1_INTERRUPT CIA_1_REG(INTERRUPT)
#define CIA_1_TIMER_A_CTRL CIA_1_REG(TIMER_A_CTRL)
#define CIA_1_TIMER_B_CTRL CIA_1_REG(TIMER_B_CTRL)

#define CIA_2_REG(offset) CIA_REG(0xdd00, offset)
#define CIA_2_PORT_A CIA_2_REG(PORT_A)
#define CIA_2_PORT_B CIA_2_REG(PORT_B)
#define CIA_2_DDR_A CIA_2_REG(DDR_A)
#define CIA_2_DDR_B CIA_2_REG(DDR_B)
#define CIA_2_TIMER_A_LO CIA_2_REG(TIMER_A_LO)
#define CIA_2_TIMER_A_HI CIA_2_REG(TIMER_A_HI)
#define CIA_2_TIMER_B_LO CIA_2_REG(TIMER_B_LO)
#define CIA_2_TIMER_B_HI CIA_2_REG(TIMER_B_HI)
#define CIA_2_TOD_TENTHS_BCD CIA_2_REG(TOD_TENTHS_BCD)
#define CIA_2_TOD_SECONDS_BCD CIA_2_REG(TOD_SECONDS_BCD)
#define CIA_2_TOD_MINUTES_BCD CIA_2_REG(TOD_MINUTES_BCD)
#define CIA_2_TOD_HOURS_BCD CIA_2_REG(TOD_HOURS_BCD)
#define CIA_2_SERIAL_SHIFT CIA_2_REG(SERIAL_SHIFT)
#define CIA_2_INTERRUPT CIA_2_REG(INTERRUPT)
#define CIA_2_TIMER_A_CTRL CIA_2_REG(TIMER_A_CTRL)
#define CIA_2_TIMER_B_CTRL CIA_2_REG(TIMER_B_CTRL)

extern const uint8_t video_base;
#define VIC_BASE ((uint16_t)&video_base)

#define SCREEN_WIDTH_TILE (40)
#define SCREEN_HEIGHT_TILE (25)
#define BORDER_WIDTH_PX (24)
#define BORDER_HEIGHT_PX (50)

extern uint8_t screen_data[SCREEN_HEIGHT_TILE][SCREEN_WIDTH_TILE];
extern uint8_t sprite_pointers[8];

extern uint8_t color_data[SCREEN_HEIGHT_TILE][SCREEN_WIDTH_TILE];

#endif
