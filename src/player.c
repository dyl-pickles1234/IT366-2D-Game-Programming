#include "simple_logger.h"

#include "gfc_input.h"

#include "camera.h"
#include "level.h"

#include "player.h"

#define PLAYER_SPEED 1
#define PLAYER_JUMP_SPEED 5.3
#define PLAYER_GRAVITY 0.2

static Entity* player = NULL;

void player_entity_new(GFC_Vector2D pos) {
    Entity* self;
    self = entity_new();

    if (!self) { slog("failed to create entity for player"); return; }

    self->sprite = gf2d_sprite_load_all(
        "images/ed210_top.png",
        128,
        128,
        16,
        false);

    self->pos = pos;
    self->center = gfc_vector2d(64, 64);
    self->scale = gfc_vector2d(0.5, 0.5);
    self->think = player_think;
    self->update = player_update;

    self->speed = PLAYER_SPEED;

    player = self;
}

void player_think() {
    if (!player) return;

    GFC_Vector2D move = { 0 };

    move.x += player->speed * gfc_input_key_down("d");
    move.x -= player->speed * gfc_input_key_down("a");

    player->vel.x = level_get()->speed + move.x;

    int testGround = level_test_rect(level_get(), gfc_rect(player->pos.x - 16, player->pos.y - 15, 31, 31));
    if (testGround && testGround != 3) {
        player->onGround = 1;
    }
    else {
        player->onGround = 0;
    }

    if (gfc_input_key_down(" ") && player->onGround) {
        player->vel.y = -PLAYER_JUMP_SPEED;
    }

}

void player_update() {
    if (!player) return;

    // slog("player - pos: %f %f   vel: %f %f", player->pos.x, player->pos.y - 465.0f, player->vel.x, player->vel.y);

    int testH = level_test_rect(level_get(), gfc_rect(player->pos.x - 16 + player->vel.x, player->pos.y - 16, 31, 31));
    int testV = level_test_rect(level_get(), gfc_rect(player->pos.x - 16, player->pos.y - 16 + player->vel.y, 31, 31));

    if (testH) {
        player->vel.x = 0;
        player->pos.x = 100;
    }

    if (testV) {
        if (testV == 3) { // hit spike
            player->pos.x = 100;
        }
        player->vel.y = 0;
    }

    gfc_vector2d_add(player->pos, player->pos, player->vel);

    if (player->vel.x || player->vel.y) player->rotation = gfc_vector2d_angle(player->vel) * GFC_RADTODEG;

    // apply gravity
    player->vel.y += PLAYER_GRAVITY;

    GFC_Vector2D cameraFocus = player->pos;
    cameraFocus.y += 50;
    camera_center_on(cameraFocus);
}