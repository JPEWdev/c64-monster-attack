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

    lda vicii_interrupt_enable_next
    sta VIC_IMR

    lda vicii_raster_next
    sta VIC_HLINE

    inc frame_count

    lda #$01
    sta VIC_IRR

skip_rst:
    ; Clear the MMC interrupt bit
    lda #$04
    sta VIC_IRR

    ; Read collisions (clears the collision register)
    lda $d01e
    sta temp_collision

    ; Check for sword collisions
    lda #$01
    and temp_collision
    beq skip_sword_collisions

    lda temp_collision
    ora sword_collisions
    sta sword_collisions

skip_sword_collisions:

    ; Check for player collisions
    lda #$02
    and temp_collision
    beq skip_player_collisions

    lda temp_collision
    ora player_collisions
    sta player_collisions

skip_player_collisions:

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
