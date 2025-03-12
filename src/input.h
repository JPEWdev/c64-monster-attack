/*
 * SPDX-License-Identifier: MIT
 */
#ifndef _INPUT_H
#define _INPUT_H

#include <stdint.h>

#define _KEY(row, col) ((((row) * 8) + (col)))

#define KEY_1 _KEY(7, 0)
#define KEY_LEFT_ARROW _KEY(7, 1)
#define KEY_CTRL _KEY(7, 2)
#define KEY_2 _KEY(7, 3)
#define KEY_SPACE _KEY(7, 4)
#define KEY_COMMODORE _KEY(7, 5)
#define KEY_Q _KEY(7, 6)
#define KEY_RUN_STOP _KEY(7, 7)

#define KEY_3 _KEY(1, 0)
#define KEY_W _KEY(1, 1)
#define KEY_A _KEY(1, 2)
#define KEY_4 _KEY(1, 3)
#define KEY_Z _KEY(1, 4)
#define KEY_S _KEY(1, 5)
#define KEY_E _KEY(1, 6)
#define KEY_LEFT_SHIFT _KEY(1, 7)

#define KEY_5 _KEY(2, 0)
#define KEY_R _KEY(2, 1)
#define KEY_D _KEY(2, 2)
#define KEY_6 _KEY(2, 3)
#define KEY_C _KEY(2, 4)
#define KEY_F _KEY(2, 5)
#define KEY_T _KEY(2, 6)
#define KEY_X _KEY(2, 7)

#define KEY_7 _KEY(3, 0)
#define KEY_Y _KEY(3, 1)
#define KEY_G _KEY(3, 2)
#define KEY_8 _KEY(3, 3)
#define KEY_B _KEY(3, 4)
#define KEY_H _KEY(3, 5)
#define KEY_U _KEY(3, 6)
#define KEY_V _KEY(3, 7)

#define KEY_9 _KEY(4, 0)
#define KEY_I _KEY(4, 1)
#define KEY_J _KEY(4, 2)
#define KEY_0 _KEY(4, 3)
#define KEY_M _KEY(4, 4)
#define KEY_K _KEY(4, 5)
#define KEY_O _KEY(4, 6)
#define KEY_N _KEY(4, 7)

#define KEY_PLUS _KEY(5, 0)
#define KEY_P _KEY(5, 1)
#define KEY_L _KEY(5, 2)
#define KEY_MINUS _KEY(5, 3)
#define KEY_PERIOD _KEY(5, 4)
#define KEY_COLON _KEY(5, 5)
#define KEY_AT _KEY(5, 6)
#define KEY_COMMA _KEY(5, 7)

#define KEY_POUND _KEY(6, 0)
#define KEY_ASTERIX _KEY(6, 1)
#define KEY_SEMICOLON _KEY(6, 2)
#define KEY_HOME _KEY(6, 3)
#define KEY_RIGHT_SHIFT _KEY(6, 4)
#define KEY_EQUAL _KEY(6, 5)
#define KEY_UP_ARROW _KEY(6, 6)
#define KEY_SLASH _KEY(6, 7)

#define KEY_DEL _KEY(0, 0)
#define KEY_RETURN _KEY(0, 1)
#define KEY_UP_DOWN _KEY(0, 2)
#define KEY_F7 _KEY(0, 3)
#define KEY_F1 _KEY(0, 4)
#define KEY_F3 _KEY(0, 5)
#define KEY_F5 _KEY(0, 6)
#define KEY_LEFT_RIGHT _KEY(0, 7)

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
