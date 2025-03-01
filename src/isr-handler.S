/*
 * SPDX-License-Identifier: MIT
 */
.include "c64.inc"

    .global isr_handler

temp_collision:
    .byte 0

    
repeat_raster_sync:
    ; The raster interrupt is repeating. Wait until the current VIC raster is
    ; the correct value before starting
    tya

repeat_raster_loop:
    cmp VIC_HLINE
    beq repeat_raster
    bcc repeat_raster

    jmp repeat_raster_loop

skip_rst_trampoline:
    jmp skip_rst

handle_missed_sprite:
    pla
    lda #$01
    sta missed_sprite,x
    jmp skip_sprite

isr_handler:
#ifdef DEBUG
    lda VIC_BORDERCOLOR
    pha

    lda #3
    sta VIC_BORDERCOLOR
#endif

    lda #$01
    and VIC_IRR
    beq skip_rst_trampoline

repeat_raster:
    ldx raster_cmd_idx

    ; Check VICII BG
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

    ; Check VICII CTRL 2
    lda #$02
    and raster_flags,x
    beq skip_ctrl_2

    lda vicii_ctrl_2_next,x
    sta VIC_CTRL2

skip_ctrl_2:

    ; Check Sprite
    lda #$08
    and raster_flags,x
    beq skip_sprite

    lda raster_sprite_idx,x
    pha

    ; Multiply sprite index by 2
    asl
    tay

    lda raster_sprite_y,x
    sta VIC_SPR0_Y,y
    cmp VIC_HLINE
    bcc handle_missed_sprite
    beq handle_missed_sprite

    lda raster_sprite_msb,x
    sta VIC_SPR_HI_X

    lda raster_sprite_x,x
    sta VIC_SPR0_X,y

    ; y = sprite index
    pla
    tay

    lda raster_sprite_x_expand,x
    sta VIC_SPR_EXP_X

    lda raster_sprite_y_expand,x
    sta VIC_SPR_EXP_Y

    lda raster_sprite_multicolor,x
    sta VIC_SPR_MCOLOR

    lda LORAM
    pha
    and #$f8
    sta LORAM

    lda raster_sprite_pointer,x
    sta sprite_pointers,y

    pla
    sta LORAM

    lda raster_sprite_color,x
    sta VIC_SPR0_COLOR,y

skip_sprite:
    inc raster_cmd_idx

    ; Load next raster line to y
    ldy vicii_raster_next,x

    ; Check Last flag
    lda #$04
    and raster_flags,x
    beq skip_last

    lda #0
    sta raster_cmd_idx

    inc frame_count
    jmp set_next_raster

skip_last:
    tya
    ; Decimal mode must be cleared in case the interrupt occurred during a BCD
    ; calclulation
    cld
    sec
    sbc #$03
    cmp VIC_HLINE
    bcs set_next_raster
    jmp repeat_raster_sync

set_next_raster:
    sty VIC_HLINE

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
