#include "mobs.h"
#include "player.h"

#include "skeleton_archer.spm.h"

static const struct sprite skeleton_archer = SKELETON_ARCHER_SPRITE;

uint8_t create_skeleton_archer(uint16_t map_x, uint8_t map_y) {
    uint8_t idx = alloc_mob();
    if (idx == MAX_MOBS) {
        return MAX_MOBS;
    }

    mob_set_sprite(idx, &skeleton_archer);
    mob_set_bb(idx, skeleton_archer_bb);
    mob_set_position(idx, map_x, map_y);
    mob_set_hp(idx, 3);
    mob_set_color(idx, COLOR_WHITE);
    mob_set_damage_color(idx, COLOR_ORANGE);
    mob_set_animation_rate(idx, 60);
    mob_set_hostile(idx, true);

    mob_set_weapon_collision_handler(idx, damage_mob_pushback);
    mob_set_speed(idx, 1, 5);

    return idx;
}


