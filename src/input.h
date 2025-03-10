/*
 * SPDX-License-Identifier: MIT
 */
#ifndef _INPUT_H
#define _INPUT_H

#include <stdint.h>

#define KEY_1 (0)
#define KEY_LEFT_ARROW (1)
#define KEY_CTRL (2)
#define KEY_2 (3)
#define KEY_SPACE (4)
#define KEY_COMMODORE (5)
#define KEY_Q (6)
#define KEY_RUN_STOP (7)

#define KEY_3 (8)
#define KEY_W (9)
#define KEY_A (10)
#define KEY_4 (11)
#define KEY_Z (12)
#define KEY_S (13)
#define KEY_E (14)
#define KEY_LEFT_SHIFT (15)

#define KEY_5 (16)
#define KEY_R (17)
#define KEY_D (18)
#define KEY_6 (19)
#define KEY_C (20)
#define KEY_F (21)
#define KEY_T (22)
#define KEY_X (23)

#define KEY_7 (24)
#define KEY_Y (25)
#define KEY_G (26)
#define KEY_8 (27)
#define KEY_B (28)
#define KEY_H (29)
#define KEY_U (30)
#define KEY_V (31)

#define KEY_9 (32)
#define KEY_I (33)
#define KEY_J (34)
#define KEY_0 (35)
#define KEY_M (36)
#define KEY_K (37)
#define KEY_O (38)
#define KEY_N (39)

#define KEY_PLUS (40)
#define KEY_P (41)
#define KEY_L (42)
#define KEY_MINUS (43)
#define KEY_PERIOD (44)
#define KEY_COLON (45)
#define KEY_AT (46)
#define KEY_COMMA (47)

#define KEY_POUND (48)
#define KEY_ASTERIX (49)
#define KEY_SEMICOLON (50)
#define KEY_HOME (51)
#define KEY_RIGHT_SHIFT (52)
#define KEY_EQUAL (53)
#define KEY_UP_ARROW (54)
#define KEY_SLASH (55)

#define KEY_DEL (56)
#define KEY_RETURN (57)
#define KEY_UP_DOWN (58)
#define KEY_F7 (59)
#define KEY_F1 (60)
#define KEY_F3 (61)
#define KEY_F5 (62)
#define KEY_LEFT_RIGHT (63)

#define NO_KEY (0x100)
uint16_t read_keyboard(void);

#define JOYSTICK_UP_BIT (0)
#define JOYSTICK_DOWN_BIT (1)
#define JOYSTICK_LEFT_BIT (2)
#define JOYSTICK_RIGHT_BIT (3)
#define JOYSTICK_FIRE_BIT (4)

uint8_t read_joystick_1(void);
uint8_t read_joystick_2(void);

#endif
