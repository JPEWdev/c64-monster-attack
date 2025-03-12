/*
 * SPDX-License-Identifier: MIT
 */

#include "store.h"

#include <cbm.h>
#include <stdbool.h>
#include <stdint.h>

#include "bcd.h"
#include "chars.h"
#include "draw.h"
#include "input.h"
#include "isr.h"
#include "player.h"
#include "reg.h"
#include "util.h"

#define TEXT_COLOR COLOR_GRAY3
#define HIGHLIGHT_COLOR COLOR_CYAN

#define HEALTH_X_TILE (0)
#define HEALTH_Y_TILE (0)
#define COIN_X_TILE (0)
#define COIN_Y_TILE (1)

#define POINTER_COL (1)
#define NAME_COL (3)
#define PRICE_COL (30)

#define START_Y (3)

static bool items_enabled[STORE_ITEM_COUNT];
static bcd_u16 items_cost[STORE_ITEM_COUNT];

static const char* const item_names[] = {
    "+1 HEALTH",
    "+2 MAX HEALTH",
    "EXIT",
};
_Static_assert(ARRAY_SIZE(item_names) == STORE_ITEM_COUNT + 1, "Missing items");

void store_reset() {
    for (uint8_t i = 0; i < STORE_ITEM_COUNT; i++) {
        items_enabled[i] = false;
    }
}

void store_set_item(enum store_item item, bcd_u16 cost) {
    items_enabled[item] = true;
    items_cost[item] = cost;
}

static void print_item(enum store_item item, uint8_t y, bool selected) {
    if (selected) {
        put_char_xy(POINTER_COL, y, ARROW_RIGHT_CHAR);
        set_color(POINTER_COL, y, COLOR_CYAN);
    } else {
        put_char_xy(POINTER_COL, y, BLANK_CHAR);
    }

    put_string_xy_color(NAME_COL, y, item_names[item],
                        selected ? HIGHLIGHT_COLOR : TEXT_COLOR);
}

void store_show(void) {
    char coin_string_buf[] = "$####";
    char player_health_buf[PLAYER_HEALTH_STR_LEN];

    DISABLE_INTERRUPTS() { VICII_INTERRUPT_ENABLE = 0; }

    // Finish current screen
    while (get_raster() != 0);

    VICII_CTRL_1 =
        _BV(VICII_DEN_BIT) | _BV(VICII_RSEL_BIT) | (3 << VICII_YSCROLL_BIT);
    VICII_CTRL_2 = _BV(VICII_CSEL_BIT);

    VICII_BG_0 = COLOR_BLACK;
    VICII_SPRITE_ENABLE = 0;

    fill_char(0, 0, SCREEN_WIDTH_TILE - 1, SCREEN_HEIGHT_TILE - 1, BLANK_CHAR);
    fill_color(0, 0, SCREEN_WIDTH_TILE - 1, SCREEN_HEIGHT_TILE - 1,
               COLOR_BLACK);

    enum store_item current_item = 0;
    while (true) {
        uint8_t y = START_Y;
        while (current_item != STORE_ITEM_COUNT &&
               !items_enabled[current_item]) {
            current_item++;
        }
        pad_string(&coin_string_buf[1], sizeof(coin_string_buf) - 1,
                   u16_to_string(player_get_coins(), &coin_string_buf[1]));
        put_string_xy_color(COIN_X_TILE, COIN_Y_TILE, coin_string_buf,
                            COLOR_GREEN);

        player_get_health_str(player_health_buf);
        put_string_xy_color(HEALTH_X_TILE, HEALTH_Y_TILE, player_health_buf,
                            COLOR_RED);

        for (enum store_item i = 0; i < STORE_ITEM_COUNT; i++) {
            char price_buf[6];
            if (!items_enabled[i]) {
                continue;
            }

            print_item(i, y, current_item == i);
            price_buf[0] = '$';
            u16_to_string(items_cost[i], &price_buf[1]);
            put_string_xy_color(PRICE_COL, y, price_buf, COLOR_GREEN);
            y++;
        }
        print_item(STORE_ITEM_COUNT, y, current_item == STORE_ITEM_COUNT);

        uint8_t input;
        while (true) {
            input = read_joystick_2();
            if (input & _BV(JOYSTICK_UP_BIT)) {
                while (true) {
                    if (current_item == 0) {
                        current_item = STORE_ITEM_COUNT;
                        break;
                    }
                    current_item--;
                    if (items_enabled[current_item]) {
                        break;
                    }
                }
                break;

            } else if (input & _BV(JOYSTICK_DOWN_BIT)) {
                while (true) {
                    if (current_item == STORE_ITEM_COUNT) {
                        current_item = 0;
                    } else {
                        current_item++;
                        if (current_item == STORE_ITEM_COUNT) {
                            break;
                        }
                    }

                    if (items_enabled[current_item]) {
                        break;
                    }
                }
                break;
            } else if (input & _BV(JOYSTICK_FIRE_BIT)) {
                if (current_item == STORE_ITEM_COUNT) {
                    return;
                }
                if (player_get_coins() >= items_cost[current_item]) {
                    switch (current_item) {
                        case STORE_ITEM_HEAL:
                            if (player_health < player_full_health) {
                                player_health++;
                                player_sub_coins(items_cost[current_item]);
                            }
                            break;

                        case STORE_ITEM_MAX_HEALTH:
                            if (player_full_health < PLAYER_MAX_HEALTH) {
                                player_full_health += 2;
                                player_health = player_full_health;
                                player_sub_coins(items_cost[current_item]);
                            }
                            break;

                        case STORE_ITEM_COUNT:
                            break;
                    }
                }
                break;
            }
        }

        // wait for release
        while (read_joystick_2());
    }
}

