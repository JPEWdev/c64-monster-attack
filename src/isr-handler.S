/*
 * SPDX-License-Identifier: MIT
 */
.include "c64.inc"

    .global isr_handler

temp_collision:
    .byte 0

isr_handler:
#ifdef DEBUG
    lda VIC_BORDERCOLOR
    pha

    lda #3
    sta VIC_BORDERCOLOR
#endif

    lda #$01
    and VIC_IRR
    beq skip_rst

    ldx raster_cmd_idx

    lda #$01
    and raster_flags,x
    beq skip_bg0

    ldy vicii_bg_0_next,x

    ; Synchronize with next rst 
    lda VIC_HLINE
raster_loop:
    cmp VIC_HLINE
    beq raster_loop

    sty VIC_BG_COLOR0
skip_bg0:

    lda #$02
    and raster_flags,x
    beq skip_ctrl_2

    lda vicii_ctrl_2_next,x
    sta VIC_CTRL2

skip_ctrl_2:

    lda vicii_raster_next,x
    sta VIC_HLINE

    inc raster_cmd_idx

    lda #$04
    and raster_flags,x
    beq skip_last

    lda #0
    sta raster_cmd_idx

    inc frame_count

skip_last:

    lda #$01
    sta VIC_IRR

skip_rst:

#ifdef DEBUG
    pla
    sta VIC_BORDERCOLOR
#endif

    ; The KERNAL ISR pushes these, so we must pop them off to call RTI
    pla
    tay

    pla
    tax

    pla

    rti
