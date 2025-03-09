#include "bcd.h"
#include "coin.spm.h"
#include "mobs.h"
#include "player.h"

static const struct sprite coin = COIN_SPRITE;

static struct {
    bcd_u8 value[MAX_MOBS];
    int16_t ttl[MAX_MOBS];
} coin_data;

static void coin_weapon_collision(uint8_t idx, uint8_t damage,
                                  enum direction dir) {
    player_add_coins(coin_data.value[idx]);
    kill_mob(idx);
}

static void coin_player_collision(uint8_t idx) {
    player_add_coins(coin_data.value[idx]);
    kill_mob(idx);
}

static void coin_update(uint8_t idx, uint8_t num_frames) {
    coin_data.ttl[idx] -= num_frames;
    if (coin_data.ttl[idx] < 0) {
        kill_mob(idx);
        return;
    }

    if (coin_data.ttl[idx] <= 120) {
        mob_set_sprite(idx, mob_has_sprite(idx) ? NULL : &coin);
    }
}

uint8_t create_coin(uint16_t map_x, uint8_t map_y, bcd_u8 value) {
    uint8_t idx = alloc_mob();
    if (idx == MAX_MOBS) {
        return idx;
    }

    mob_set_sprite(idx, &coin);
    mob_set_bb(idx, coin_bb);
    mob_set_position(idx, map_x, map_y);
    mob_set_hp(idx, 0);
    mob_set_color(idx, COLOR_YELLOW);
    mob_set_animation_rate(idx, 15);
    mob_set_hostile(idx, false);

    mob_set_weapon_collision_handler(idx, coin_weapon_collision);
    mob_set_player_collision_handler(idx, coin_player_collision);
    mob_set_update_handler(idx, coin_update);

    coin_data.value[idx] = value;
    coin_data.ttl[idx] = 300;

    return idx;
}

