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

struct {
    uint8_t value;
    int16_t ttl;
} coin_data[MAX_MOBS];

static void coin_sword_collision(struct mob* mob, uint8_t damage) {
    player_coins += coin_data[mob->idx].value;
    kill_mob(mob);
}

static void coin_player_collision(struct mob* mob) {
    player_coins += coin_data[mob->idx].value;
    kill_mob(mob);
}

static void coin_update(struct mob* mob, uint8_t num_frames) {
    coin_data[mob->idx].ttl -= num_frames;
    if (coin_data[mob->idx].ttl < 0) {
        kill_mob(mob);
        return;
    }

    if (coin_data[mob->idx].ttl <= 120) {
        mob->sprite = mob->sprite ? NULL : &coin;
    }
}

struct mob* create_coin(uint16_t x, uint16_t y, uint8_t value) {
    struct mob* m = alloc_mob();
    if (!m) {
        return NULL;
    }
    m->sprite = &coin;
    m->bb = coin_bb;
    m->x = x;
    m->y = y;
    m->hp = 0;
    m->color = COLOR_YELLOW;
    m->animation.rate = 15;
    m->on_sword_collision = coin_sword_collision;
    m->on_player_collision = coin_player_collision;
    m->on_update = coin_update;

    coin_data[m->idx].value = value;
    coin_data[m->idx].ttl = 300;
    return m;
}

