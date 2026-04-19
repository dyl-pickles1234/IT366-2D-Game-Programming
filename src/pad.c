#include "simple_logger.h"

#include "gfc_input.h"

#include "camera.h"
#include "level.h"

#include "player.h"
#include "pad.h"

#define PAD_NORMAL_BOOST 7.4
#define PAD_SMALL_BOOST 4.8

Entity* pad_entity_new(PadType type, GFC_Vector2D pos) {
    Entity* self;
    self = entity_new();

    if (!self) { slog("failed to create entity for pad"); return NULL; }

    if (type == PAD_NORMAL) {
        gfc_line_cpy(self->name, "normal_pad");
        self->speed = PAD_NORMAL_BOOST;
    }
    else if (type == PAD_SMALL) {
        gfc_line_cpy(self->name, "small_pad");
        self->speed = PAD_SMALL_BOOST;
    }
    else if (type == PAD_GRAVITY) {
        gfc_line_cpy(self->name, "gravity_pad");
        self->speed = -1;
    }

    self->sprite = gf2d_sprite_load_all(
        "images/player/ball.png",
        32,
        32,
        1,
        false);

    self->pos = pos;
    self->center = gfc_vector2d(16, -80);
    self->scale = gfc_vector2d(0.6, 0.15);
    self->think = pad_think;
    self->update = pad_update;

    self->hitbox = gfc_rect(pos.x - 8, pos.y - 4, 16, 4);
    self->hitbox.y += 16;

    return self;
}

void pad_think(Entity* pad) {
    if (!pad) return;

    Entity* player = player_get();

    int playerTest = gfc_rect_overlap(pad->hitbox, player->hitbox);

    if (playerTest) {
        int grav = player_gravity_get();
        if (pad->speed != -1) { // if speed is valid, boost player by that much
            player->vel.y = -pad->speed * grav;
        }
        else { // treat as gravity pad
            player_gravity_set(-1 * grav);
            player->vel.y = -4;
        }
    }
}

void pad_update(Entity* pad) {
    if (!pad) return;
}