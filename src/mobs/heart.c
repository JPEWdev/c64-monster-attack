#include "heart.spm.h"
#include "mobs.h"
#include "player.h"

static const struct sprite heart = HEART_SPRITE;

static struct {
    int16_t ttl[MAX_MOBS];
} heart_data;

static void heart_sword_collision(uint8_t idx, uint8_t damage,
                                  enum direction dir) {
    heal_player(1);
    kill_mob(idx);
}

static void heart_player_collision(uint8_t idx) {
    heal_player(1);
    kill_mob(idx);
}

static void heart_update(uint8_t idx, uint8_t num_frames) {
    heart_data.ttl[idx] -= num_frames;
    if (heart_data.ttl[idx] <= 0) {
        kill_mob(idx);
        return;
    }

    if (heart_data.ttl[idx] <= 120) {
        mob_set_sprite(idx, mob_has_sprite(idx) ? NULL : &heart);
    }
}

uint8_t create_heart(uint16_t map_x, uint8_t map_y) {
    uint8_t idx = alloc_mob();
    if (idx == MAX_MOBS) {
        return MAX_MOBS;
    }

    mob_set_sprite(idx, &heart);
    mob_set_bb(idx, heart_bb);
    mob_set_position(idx, map_x, map_y);
    mob_set_hp(idx, 0);
    mob_set_color(idx, COLOR_RED);
    mob_set_animation_rate(idx, 15);
    mob_set_hostile(idx, false);

    mob_set_weapon_collision_handler(idx, heart_sword_collision);
    mob_set_player_collision_handler(idx, heart_player_collision);
    mob_set_update_handler(idx, heart_update);

    heart_data.ttl[idx] = 300;

    return idx;
}

