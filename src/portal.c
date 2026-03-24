#include "simple_logger.h"

#include "gfc_input.h"

#include "camera.h"
#include "level.h"

#include "player.h"
#include "portal.h"

Entity* portal_entity_new(PortalType type, GFC_Vector2D pos) {
    Entity* self;
    PortalData* data;

    self = entity_new();
    data = gfc_allocate_array(sizeof(PortalData), 1);

    if (!self || !data) { slog("failed to create entity for portal"); return NULL; }

    self->sprite = gf2d_sprite_load_all(
        "images/objects/portal.png",
        32,
        32,
        1,
        false);

    self->pos = pos;
    self->center = gfc_vector2d(16, 16);
    self->scale = gfc_vector2d(2, 2);
    self->think = portal_think;
    self->update = portal_update;
    self->draw = portal_draw;

    self->hitbox = gfc_rect(pos.x - 16, pos.y - 32, 32, 64);

    data->type = type;
    self->data = data;

    return self;
}

void portal_think(Entity* portal) {
    if (!portal) return;

    Entity* player = player_get();
    PortalData* data = portal->data;

    int playerTest = gfc_rect_overlap(portal->hitbox, player->hitbox);

    if (playerTest) {
        player_mode_set(data->type);
    }
}

void portal_update(Entity* portal) {
    if (!portal) return;
}

void portal_draw(Entity* portal) {
    if (!portal) return;
    PortalData* data = portal->data;

    GFC_Vector2D pos;
    GFC_Vector2D scale = camera_get_zoom();

    gfc_vector2d_add(pos, portal->pos, camera_get_offset());
    pos = gfc_vector2d_multiply(pos, scale);

    scale.x *= 0.5;
    scale.y *= 0.5;

    Sprite* sprite;

    switch (data->type)
    {
    case PORTAL_CUBE:
        sprite = gf2d_sprite_load_all(
            "images/player/cube.png",
            32,
            32,
            1,
            false);
        break;
    case PORTAL_SHIP:
        sprite = gf2d_sprite_load_all(
            "images/player/ship.png",
            32,
            32,
            1,
            false);
        break;
    case PORTAL_BALL:
        sprite = gf2d_sprite_load_all(
            "images/player/ball.png",
            32,
            32,
            1,
            false);
        break;
    case PORTAL_WAVE:
        sprite = gf2d_sprite_load_all(
            "images/player/wave.png",
            32,
            32,
            1,
            false);
        break;
    case PORTAL_UFO:
        sprite = gf2d_sprite_load_all(
            "images/player/ufo.png",
            32,
            32,
            1,
            false);
        break;
    }
    gf2d_sprite_draw(
        sprite,
        pos,
        &scale,
        NULL,
        &portal->rotation,
        NULL,
        NULL,
        (Uint32)portal->frame);
}