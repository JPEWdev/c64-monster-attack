#include "mobs.h"
#include "player.h"

extern const struct sprite_frame skeleton_0;
extern const struct sprite_frame skeleton_1;
static const struct sprite_frame* const skeleton_frames[] = {
    &skeleton_0,
    &skeleton_1,
};
const struct sprite skeleton = {FRAMES(skeleton_frames)};
extern const struct bb skeleton_bb;

struct mob* create_skeleton(uint16_t x, uint16_t y) {
    struct mob* m = alloc_mob();
    if (!m) {
        return NULL;
    }

    m->sprite = &skeleton;
    m->bb = skeleton_bb;
    m->x = x;
    m->y = y;
    m->hp = 10;
    m->color = COLOR_WHITE;
    m->damage_color = COLOR_ORANGE;
    m->animation.rate = 60;
    m->on_sword_collision = damage_mob_pushback;
    m->speed_pixels = 1;
    m->speed_frames = 5;

    return m;
}

