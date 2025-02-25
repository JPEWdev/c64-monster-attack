/*
 * SPDX-License-Identifier: MIT
 */
.include "c64.inc"

    .global isr_handler

temp_collision:
    .byte 0

isr_handler:
#ifdef DEBUG
    ldx VIC_BORDERCOLOR
    lda #3
    sta VIC_BORDERCOLOR
#endif

    lda #$01
    and VIC_IRR
    beq skip_rst

    ldy vicii_bg_0_next

    ; Synchronize with next rst 
    lda VIC_HLINE
raster_loop:
    cmp VIC_HLINE
    beq raster_loop

    sty VIC_BG_COLOR0

    lda vicii_ctrl_1_next
    sta VIC_CTRL1

    lda vicii_ctrl_2_next
    sta VIC_CTRL2

    lda vicii_raster_next
    sta VIC_HLINE

    inc interrupt_count

    lda #$01
    sta VIC_IRR

skip_rst:

#ifdef DEBUG
    stx VIC_BORDERCOLOR
#endif

    ; The KERNAL ISR pushes these, so we must pop them off to call RTI
    pla
    tay

    pla
    tax

    pla

    rti
