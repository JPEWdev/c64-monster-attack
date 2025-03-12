/*
 * SPDX-License-Identifier: MIT
 */
#ifndef _STORE_H
#define _STORE_H

#include <stdint.h>

#include "bcd.h"

enum store_item {
    STORE_ITEM_HEAL,
    STORE_ITEM_MAX_HEALTH,

    STORE_ITEM_COUNT,
};

void store_reset(void);
void store_set_item(enum store_item item, bcd_u16 cost);
void store_show(void);

#endif
