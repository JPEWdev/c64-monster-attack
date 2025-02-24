#include "mobs.h"
#include "player.h"

extern const struct sprite_frame heart_0;
extern const struct sprite_frame heart_1;

static const struct sprite_frame* const heart_frames[] = {
    &heart_0,
    &heart_1,
};

static const struct sprite heart = {FRAMES(heart_frames)};
extern const struct bb heart_bb;

struct {
    int16_t ttl;
} heart_data[MAX_MOBS];

static void heart_sword_collision(struct mob* mob, uint8_t damage) {
    heal_player(1);
    kill_mob(mob);
}

static void heart_player_collision(struct mob* mob) {
    heal_player(1);
    kill_mob(mob);
}

static void heart_update(struct mob* mob, uint8_t num_frames) {
    heart_data[mob->idx].ttl -= num_frames;
    if (heart_data[mob->idx].ttl <= 0) {
        kill_mob(mob);
        return;
    }

    if (heart_data[mob->idx].ttl <= 120) {
        mob->sprite = mob->sprite ? NULL : &heart;
    }
}

struct mob* create_heart(uint16_t x, uint16_t y) {
    struct mob* m = alloc_mob();
    if (!m) {
        return NULL;
    }

    m->sprite = &heart;
    m->bb = heart_bb;
    m->x = x;
    m->y = y;
    m->hp = 0;
    m->color = COLOR_RED;
    m->animation.rate = 15;
    m->on_sword_collision = heart_sword_collision;
    m->on_player_collision = heart_player_collision;
    m->on_update = heart_update;

    heart_data[m->idx].ttl = 300;

    return m;
}

