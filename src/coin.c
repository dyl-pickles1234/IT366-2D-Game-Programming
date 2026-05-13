#include "simple_logger.h"

#include "gfc_input.h"

#include "camera.h"
#include "level.h"

#include "player.h"
#include "coin.h"

Entity* coin_entity_new(GFC_Vector2D pos, Uint8 index) {
    Entity* self;
    self = entity_new();

    if (!self) { slog("failed to create entity for coin"); return NULL; }

    gfc_line_cpy(self->name, "coin");
    self->sprite = gf2d_sprite_load_all(
        "images/objects/coin.png",
        32,
        32,
        1,
        false);

    self->pos = pos;
    self->center = gfc_vector2d(16, 16);
    self->scale = gfc_vector2d(1, 1);
    self->think = coin_think;
    // self->update = coin_update;
    self->data = (void*)index; // HACK use the data pointer to just store the index teehee

    self->hitbox = gfc_rect(pos.x - 16, pos.y - 16, 32, 32);

    return self;
}

void coin_think(Entity* coin) {
    if (!coin) return;

    Entity* player = player_get();
    int playerTest = gfc_rect_overlap(coin->hitbox, player->hitbox);

    if (playerTest) {
        // slog("coin collected");
        player_add_coin((Uint8)coin->data);
        entity_free(coin);
    }
}

void coin_update(Entity* coin) {
    if (!coin) return;
}