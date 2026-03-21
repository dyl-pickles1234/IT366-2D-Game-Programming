#include "simple_logger.h"

#include "gfc_input.h"

#include "camera.h"
#include "level.h"

#include "player.h"
#include "pad.h"

#define PAD_NORMAL_BOOST 7.58

Entity* pad_entity_new(PadType type, GFC_Vector2D pos) {
    Entity* self;
    self = entity_new();

    if (!self) { slog("failed to create entity for pad"); return NULL; }

    self->sprite = gf2d_sprite_load_all(
        "images/ed210.png",
        128,
        128,
        16,
        false);

    self->pos = pos;
    self->center = gfc_vector2d(64, 96);
    self->scale = gfc_vector2d(0.25, 0.0625);
    self->think = pad_think;
    self->update = pad_update;

    self->speed = (type == PAD_NORMAL) ? PAD_NORMAL_BOOST : -1;
    self->hitbox = gfc_rect(pos.x - 8, pos.y - 4, 16, 4);

    return self;
}

void pad_think(Entity* pad) {
    if (!pad) return;

    Entity* player = player_get();

    int playerTest = gfc_rect_overlap(pad->hitbox, player->hitbox);

    if (playerTest) {
        if (pad->speed != -1) { // if speed is valid, boost player by that much
            player->vel.y = -pad->speed;
        }
        else { // treat as gravity pad
            player_gravity_set(-1 * player_gravity_get());
            player->vel.y = -4;
        }
    }
}

void pad_update(Entity* pad) {
    if (!pad) return;
}