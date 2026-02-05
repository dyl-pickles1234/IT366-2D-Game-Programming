#include "simple_logger.h"

#include "gfc_input.h"

#include "camera.h"
#include "player.h"

static Entity* player = NULL;

Entity* player_entity_new(GFC_Vector2D pos) {
    Entity* self;
    self = entity_new();

    if (!self) { slog("failed to create entity for player"); return NULL; }

    self->sprite = gf2d_sprite_load_all(
        "images/ed210_top.png",
        128,
        128,
        16,
        false);

    self->pos = pos;
    self->center = gfc_vector2d(64, 64);
    self->think = player_think;
    self->update = player_update;

    self->speed = 3;

    return self;
}

void player_think(Entity* player) {
    if (!player) return;

    GFC_Vector2D move = { 0 };

    move.x += gfc_input_key_down("d");
    move.x -= gfc_input_key_down("a");

    move.y += gfc_input_key_down("s");
    move.y -= gfc_input_key_down("w");

    player->vel = move;
}

void player_update(Entity* player) {
    if (!player) return;

    gfc_vector2d_normalize(&player->vel);
    gfc_vector2d_scale(player->vel, player->vel, player->speed);
    gfc_vector2d_add(player->pos, player->pos, player->vel);

    if (player->vel.x || player->vel.y) player->rotation = gfc_vector2d_angle(player->vel) * GFC_RADTODEG;

    camera_center_on(player->pos);
}