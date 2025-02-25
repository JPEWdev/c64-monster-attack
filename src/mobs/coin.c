#include "mobs.h"
#include "player.h"

extern const struct sprite_frame coin_0;
extern const struct sprite_frame coin_1;
extern const struct sprite_frame coin_2;
extern const struct sprite_frame coin_3;

static const struct sprite_frame* const coin_frames[] = {
    &coin_0,
    &coin_1,
    &coin_2,
    &coin_3,
};

static const struct sprite coin = {FRAMES(coin_frames)};
extern const struct bb coin_bb;

static struct {
    uint8_t value[MAX_MOBS];
    int16_t ttl[MAX_MOBS];
} coin_data;

static void coin_sword_collision(uint8_t idx, uint8_t damage) {
    player_coins += coin_data.value[idx];
    kill_mob(idx);
}

static void coin_player_collision(uint8_t idx) {
    player_coins += coin_data.value[idx];
    kill_mob(idx);
}

static void coin_update(uint8_t idx, uint8_t num_frames) {
    coin_data.ttl[idx] -= num_frames;
    if (coin_data.ttl[idx] < 0) {
        kill_mob(idx);
        return;
    }

    if (coin_data.ttl[idx] <= 120) {
        mob_set_sprite(idx, mob_get_sprite(idx) ? NULL : &coin);
    }
}

uint8_t create_coin(uint16_t x, uint16_t y, uint8_t value) {
    uint8_t idx = alloc_mob();
    if (idx == MAX_MOBS) {
        return idx;
    }

    mob_set_sprite(idx, &coin);
    mob_set_bb(idx, coin_bb);
    mob_set_position(idx, x, y);
    mob_set_hp(idx, 0);
    mob_set_color(idx, COLOR_YELLOW);
    mob_set_animation_rate(idx, 15);

    mob_set_sword_collision_handler(idx, coin_sword_collision);
    mob_set_player_collision_handler(idx, coin_player_collision);
    mob_set_update_handler(idx, coin_update);

    coin_data.value[idx] = value;
    coin_data.ttl[idx] = 300;

    return idx;
}

