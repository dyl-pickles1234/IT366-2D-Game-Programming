#include "simple_logger.h"

#include "player.h"

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

    return self;
}