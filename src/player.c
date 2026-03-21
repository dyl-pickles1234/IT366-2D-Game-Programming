#include "simple_logger.h"

#include "gfc_input.h"

#include "camera.h"
#include "level.h"

#include "player.h"

#define PLAYER_SPEED 1
#define PLAYER_JUMP_SPEED 5.05
#define CUBE_GRAVITY 0.19
#define SHIP_GRAVITY 0.13

static Entity* player = NULL;
static int gravityMult = 1;
static PlayerMode playerMode = PLAYER_CUBE;

void player_entity_new(GFC_Vector2D pos) {
    Entity* self;
    self = entity_new();

    if (!self) { slog("failed to create entity for player"); return; }

    self->sprite = gf2d_sprite_load_all(
        "images/player/cube.png",
        32,
        32,
        1,
        false);

    self->pos = pos;
    self->center = gfc_vector2d(16, 16);
    self->scale = gfc_vector2d(1, 1);
    self->think = player_think;
    self->update = player_update;

    self->speed = PLAYER_SPEED;
    self->hitbox = gfc_rect(pos.x - 16, pos.y - 16, 31, 31);

    player = self;
}

void player_think() {
    if (!player) return;

    switch (playerMode)
    {
    case PLAYER_CUBE:
        player->sprite = gf2d_sprite_load_all(
            "images/player/cube.png",
            32,
            32,
            1,
            false);

        GFC_Vector2D move = { 0 };

        move.x += player->speed * gfc_input_key_down("d");
        move.x -= player->speed * gfc_input_key_down("a");

        player->vel.x = level_get()->speed + move.x;

        GFC_Rect groundCheck = player->hitbox;
        groundCheck.y += 1 * gravityMult;
        int testGround = level_test_rect(level_get(), groundCheck);
        if (testGround && testGround != 3) {
            player->onGround = 1;
        }
        else {
            player->onGround = 0;
        }

        if (gfc_input_key_down(" ") && player->onGround) {
            player->vel.y = -PLAYER_JUMP_SPEED * gravityMult;
            slog("jumped at tile %i %i", (int)(player->pos.x / 32), (int)(player->pos.y / 32));
        }
        break;
    case PLAYER_SHIP:
        player->sprite = gf2d_sprite_load_all(
            "images/player/ship.png",
            32,
            32,
            1,
            false);

        GFC_Vector2D moveShip = { 0 };

        moveShip.x += player->speed * gfc_input_key_down("d");
        moveShip.x -= player->speed * gfc_input_key_down("a");

        player->vel.x = level_get()->speed + moveShip.x;

        player->onGround = 0;

        if (gfc_input_key_down(" ")) {
            // player->vel.y += -CUBE_GRAVITY * gravityMult;
            gravityMult = -1;
        }
        else {
            gravityMult = 1;
        }
        break;
    }
}

void player_update() {
    if (!player) return;

    // slog("player - pos: %f %f   vel: %f %f", player->pos.x, player->pos.y, player->vel.x, player->vel.y);

    // do collision checks for permission
    GFC_Rect levelVCheck = player->hitbox;
    levelVCheck.y += player->vel.y;

    int testV = level_test_rect(level_get(), levelVCheck);

    if (testV) {
        if (testV == 3 || player->vel.y * gravityMult < 0) { // hit spike or jumped into ceiling
            player_reset();
        }
        player->vel.y = 0;
        player->pos.y = (int)(player->pos.y / 32) * 32 + 16;

        if (gravityMult < 0) player->pos.y += 0.001;
    }

    // update player hitbox
    player->hitbox.x = player->pos.x - 16;
    player->hitbox.y = player->pos.y - 16;

    GFC_Rect levelHCheck = player->hitbox;
    levelHCheck.x += player->vel.x;

    int testH = level_test_rect(level_get(), levelHCheck);

    if (testH) {
        player_reset();
    }

    gfc_vector2d_add(player->pos, player->pos, player->vel);

    // update player hitbox
    player->hitbox.x = player->pos.x - 16;
    player->hitbox.y = player->pos.y - 16;

    if (player->vel.x || player->vel.y) player->rotation = gfc_vector2d_angle(player->vel) * GFC_RADTODEG - 90;

    // apply gravity
    float gravity;
    switch (playerMode)
    {
    case PLAYER_CUBE:
        gravity = CUBE_GRAVITY;
        break;
    case PLAYER_SHIP:
        gravity = SHIP_GRAVITY;
        break;
    }
    if (!player->onGround) player->vel.y += gravity * gravityMult;

    GFC_Vector2D cameraFocus = player->pos;
    cameraFocus.x += 125;
    // cameraFocus.y += 100 * gravityMult;
    if (playerMode == PLAYER_SHIP) {
        cameraFocus.y = 700;
    }
    camera_center_on(cameraFocus);
}

Entity* player_get() {
    return player;
}

int player_gravity_get() {
    return gravityMult;
}

void player_gravity_set(int newGravity) {
    gravityMult = newGravity;
}

void player_reset() {
    player->vel.x = 0;
    player->vel.y = 0;
    player->pos.x = 100;
    player->pos.y = 464;
    gravityMult = 1;
    slog("player reset");
}

void player_mode_set(PlayerMode mode) {
    playerMode = mode;
}