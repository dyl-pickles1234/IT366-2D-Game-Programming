#include "simple_logger.h"

#include "gfc_input.h"

#include "camera.h"
#include "level.h"

#include "player.h"
#include "orb.h"

#define ORB_NORMAL_BOOST 5.35
#define ORB_SMALL_BOOST 3.6

Entity* orb_entity_new(OrbType type, GFC_Vector2D pos) {
    Entity* self;
    self = entity_new();

    if (!self) { slog("failed to create entity for orb"); return NULL; }

    if (type == ORB_NORMAL) {
        gfc_line_cpy(self->name, "normal_orb");
        self->speed = ORB_NORMAL_BOOST;
    }
    else if (type == ORB_SMALL) {
        gfc_line_cpy(self->name, "small_orb");
        self->speed = ORB_SMALL_BOOST;
    }
    else if (type == ORB_GRAVITY) {
        gfc_line_cpy(self->name, "gravity_orb");
        self->speed = -1;
    }

    self->sprite = gf2d_sprite_load_all(
        "images/player/cube.png",
        32,
        32,
        1,
        false);

    self->pos = pos;
    self->center = gfc_vector2d(16, 16);
    self->scale = gfc_vector2d(0.75, 0.75);
    self->think = orb_think;
    self->update = orb_update;

    self->hitbox = gfc_rect(pos.x - 32, pos.y - 32, 64, 64);

    return self;
}

void orb_think(Entity* orb) {
    if (!orb) return;

    Entity* player = player_get();

    int playerTest = gfc_rect_overlap(orb->hitbox, player->hitbox);

    if (playerTest && gfc_input_key_pressed(" ")) {
        int grav = player_gravity_get();
        if (orb->speed != -1) { // if speed is valid, boost player by that much
            player->vel.y = -orb->speed * grav;
        }
        else { // treat as gravity orb
            player_gravity_set(-1 * grav);
            player->vel.y = -4 * grav;
        }
    }
}

void orb_update(Entity* orb) {
    if (!orb) return;
}