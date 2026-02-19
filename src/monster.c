#include "simple_logger.h"

#include "monster.h"

static Entity* monster = NULL;

Entity* monster_new(GFC_Vector2D pos) {
    Entity* self;
    self = entity_new();

    if (!self) { slog("failed to create entity for monster"); return NULL; }

    self->sprite = gf2d_sprite_load_all(
        "images/space_bug_top.png",
        128,
        128,
        16,
        false);

    self->pos = pos;
    self->center = gfc_vector2d(64, 64);
    self->think = monster_think;
    self->update = monster_update;

    self->speed = 1;

    return self;
}

void monster_think(Entity* monster) {
    if (!monster) return;

    GFC_Vector2D move = { 0 };

    move.x += gfc_random() - 0.5f;

    move.y += gfc_random() - 0.5f;

    gfc_vector2d_scale(move, move, 0.5);

    gfc_vector2d_add(monster->vel, monster->vel, move);
}

void monster_update(Entity* monster) {
    if (!monster) return;

    gfc_vector2d_normalize(&monster->vel);
    gfc_vector2d_scale(monster->vel, monster->vel, monster->speed);
    gfc_vector2d_add(monster->pos, monster->pos, monster->vel);

    if (monster->vel.x || monster->vel.y) monster->rotation = gfc_vector2d_angle(monster->vel) * GFC_RADTODEG;
}