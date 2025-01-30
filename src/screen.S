/*
 * SPDX-License-Identifier: MIT
 */
    ; Screen Data
    .section screendata, "a"
    .align 1<<10
    .global screen_data
screen_data:
    .space 1016

    .global sprite_pointers
sprite_pointers:
    .space 8

    ; Color Data
    .section colordata, "a"
    .global color_data
color_data:
    .space 1000
