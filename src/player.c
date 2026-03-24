#include "simple_logger.h"

#include "gfc_input.h"

#include "camera.h"
#include "level.h"

#include "bullet.h"

#include "player.h"

#define PLAYER_SPEED 3

#define CUBE_JUMP_SPEED 5.05
#define SHIP_BOOST_SPEED 0.3
#define UFO_JUMP_SPEED 3.15

#define CUBE_GRAVITY 0.19
#define SHIP_GRAVITY 0.13
#define UFO_GRAVITY 0.1

#define MAX_CHARGE 60
#define PRACTICE_TIMER 150

static Entity* player = NULL;
static PlayerMode playerMode = PLAYER_CUBE;

static int gravityMult = 1;
static Uint8 flipped = 0;
static int charge = 0;
static int shootTimer = 0;

static int practiceMode = 0;
static int practiceTimer = PRACTICE_TIMER;
static GFC_Vector2D practiceCheckpointPos = { 0 };
static int practiceCheckpointGravity = 1;
static Uint8 practiceCheckpointFlipped = 0;
static PlayerMode practiceCheckpointMode = PLAYER_CUBE;

static int mouseX, mouseY;
static int prevLClick, prevRClick;
static int lClick, rClick;

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
    self->draw = player_draw;

    self->speed = PLAYER_SPEED;
    self->hitbox = gfc_rect(pos.x - 16, pos.y - 16, 31, 31);

    player = self;
}

void player_think() {
    if (!player) return;

    GFC_Vector2D move = { 0 };

    move.x += player->speed * gfc_input_key_down("d");
    move.x -= player->speed * gfc_input_key_down("a");

    if (flipped) move.x *= -1;

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

    if (gfc_input_key_pressed("p")) {
        practiceMode = practiceMode ? 0 : 1;
    }

    if (practiceMode && player->onGround && practiceTimer <= 0) {
        practiceCheckpointPos = player->pos;
        practiceCheckpointGravity = gravityMult;
        practiceCheckpointFlipped = flipped;
        practiceCheckpointMode = playerMode;
        practiceTimer = PRACTICE_TIMER;
    }

    prevLClick = lClick;
    prevRClick = rClick;

    Uint32 clicks = SDL_GetMouseState(&mouseX, &mouseY);
    lClick = (clicks & SDL_BUTTON(1)) != 0;
    rClick = (clicks & SDL_BUTTON(3)) != 0;

    if (flipped) {
        mouseX = 1200 - mouseX;
    }

    GFC_Vector2D mouse = gfc_vector2d(mouseX, mouseY);
    GFC_Vector2D bulletPos = gfc_vector2d(player->pos.x, player->pos.y);
    GFC_Vector2D playerScreenPos;
    GFC_Vector2D playerToMouse;

    gfc_vector2d_add(playerScreenPos, player->pos, camera_get_offset());
    playerScreenPos = gfc_vector2d_multiply(playerScreenPos, camera_get_zoom());

    gfc_vector2d_sub(playerToMouse, mouse, playerScreenPos);
    gfc_vector2d_normalize(&playerToMouse);

    if (shootTimer > 0) shootTimer--;

    switch (playerMode)
    {
    case PLAYER_CUBE:
        player->sprite = gf2d_sprite_load_all(
            "images/player/cube.png",
            32,
            32,
            1,
            false);

        if (gfc_input_key_down(" ") && player->onGround) {
            player->vel.y = -CUBE_JUMP_SPEED * gravityMult;
            // slog("jumped at tile %i %i", (int)(player->pos.x / 32), (int)(player->pos.y / 32));
        }

        // left click shoot
        if (lClick && !prevLClick) {
            GFC_Vector2D bulletVel = playerToMouse;

            // set speed
            gfc_vector2d_scale(bulletVel, bulletVel, 8);
            // gfc_vector2d_add(bulletVel, bulletVel, player->vel);

            bullet_entity_new("images/objects/bullet.png", bulletPos, 16, bulletVel, 0, -1);
        }

        // right click shoot
        if (charge >= MAX_CHARGE && rClick && !prevRClick) {
            GFC_Vector2D bulletVel = playerToMouse;

            for (int i = 0; i < 5; i++) {
                GFC_Vector2D bulletVelRand;

                // set speed
                gfc_vector2d_scale(bulletVelRand, bulletVel, gfc_random() + 5);

                bulletVelRand = gfc_vector2d_rotate(bulletVelRand, gfc_random() * 0.5 - 0.25);
                bullet_entity_new("images/objects/bullet.png", bulletPos, 8, bulletVelRand, 0, -1);
            }

            charge = -1;
        }

        break;
    case PLAYER_SHIP:
        player->sprite = gf2d_sprite_load_all(
            "images/player/ship.png",
            32,
            32,
            1,
            false);

        if (gfc_input_key_down(" ")) {
            player->vel.y += -SHIP_BOOST_SPEED;
        }

        // left click shoot
        if (lClick && shootTimer <= 0) {
            GFC_Vector2D bulletVel = playerToMouse;

            // set speed
            gfc_vector2d_scale(bulletVel, bulletVel, 3);
            gfc_vector2d_add(bulletVel, bulletVel, player->vel);

            bullet_entity_new("images/objects/bullet.png", bulletPos, 8, bulletVel, 0, -1);
            shootTimer = 15;
        }

        // right click shoot
        if (charge >= 0 && rClick) {
            if (shootTimer <= 0) {
                GFC_Vector2D bulletVel = playerToMouse;

                for (int i = 0; i < 5; i++) {
                    GFC_Vector2D bulletVelRand;

                    // set speed
                    gfc_vector2d_scale(bulletVelRand, bulletVel, gfc_random() + 2);

                    bulletVelRand = gfc_vector2d_rotate(bulletVelRand, gfc_random() * 0.5 - 0.25);
                    bullet_entity_new("images/objects/bullet.png", bulletPos, 8, bulletVelRand, 0, -1);
                }
                shootTimer = 15;
            }
            charge -= 2;
        }

        if (charge == -1 && rClick && shootTimer <= 0) charge = -100;

        break;
    case PLAYER_BALL:
        player->sprite = gf2d_sprite_load_all(
            "images/player/ball.png",
            32,
            32,
            1,
            false);

        if (gfc_input_key_pressed(" ") && player->onGround) {
            // player->vel.y = -CUBE_JUMP_SPEED * gravityMult;
            player->vel.y = 0;
            gravityMult *= -1;
        }

        // left click shoot
        if (lClick && !prevLClick && shootTimer <= 0) {
            GFC_Vector2D bulletVel = playerToMouse;

            // set speed
            gfc_vector2d_scale(bulletVel, bulletVel, 1);
            gfc_vector2d_add(bulletVel, bulletVel, player->vel);

            bullet_entity_new("images/objects/bullet.png", bulletPos, 64, bulletVel, 0, -1);
            shootTimer = 120;
        }

        // right click shoot
        if (charge >= MAX_CHARGE && rClick && !prevRClick) {
            bullet_entity_new("images/objects/bullet.png", bulletPos, 256, gfc_vector2d(0, 0), 0, 2);
            charge = -1;
        }

        break;
    case PLAYER_WAVE:
        player->sprite = gf2d_sprite_load_all(
            "images/player/wave.png",
            32,
            32,
            1,
            false);

        if (gfc_input_key_down(" ")) {
            // player->vel.y = -CUBE_JUMP_SPEED * gravityMult;
            player->vel.y = -level_get()->speed;
        }
        else {
            player->vel.y = level_get()->speed;
        }

        // left click shoot
        if (lClick && !prevLClick) {
            GFC_List* enemies = level_enemies_get();
            Entity* enemy;
            GFC_Vector2D enemyScreenPos;
            GFC_Rect hitboxScreen;
            int enemyTest;

            for (int i = 0; i < gfc_list_get_count(enemies); i++) {
                enemy = gfc_list_get_nth(enemies, i);

                gfc_vector2d_add(enemyScreenPos, enemy->pos, camera_get_offset());
                enemyScreenPos = gfc_vector2d_multiply(enemyScreenPos, camera_get_zoom());

                hitboxScreen.w = enemy->hitbox.w * camera_get_zoom().x;
                hitboxScreen.h = enemy->hitbox.h * camera_get_zoom().y;
                hitboxScreen.x = enemyScreenPos.x - hitboxScreen.w / 2;
                hitboxScreen.y = enemyScreenPos.y - hitboxScreen.h / 2;

                enemyTest = gfc_point_in_rect(mouse, hitboxScreen);

                if (enemyTest) {
                    slog("zapped his ass");
                    gfc_list_delete_nth(enemies, i);
                    entity_free(enemy);
                    break;
                }
            }
        }

        // right click shoot
        if (charge >= MAX_CHARGE && rClick && !prevRClick) {
            GFC_List* enemies = level_enemies_get();
            Entity* enemy;
            GFC_Vector2D enemyScreenPos;

            for (int i = 0; i < gfc_list_get_count(enemies); i++) {
                enemy = gfc_list_get_nth(enemies, i);

                gfc_vector2d_add(enemyScreenPos, enemy->pos, camera_get_offset());
                enemyScreenPos = gfc_vector2d_multiply(enemyScreenPos, camera_get_zoom());

                if (gfc_point_in_rect(enemyScreenPos, gfc_rect(0, 0, 1200, 768))) {
                    slog("zapped his ass");
                    gfc_list_delete_nth(enemies, i);
                    entity_free(enemy);
                    break;
                }
            }
            charge = -1;
        }

        break;
    case PLAYER_UFO:
        player->sprite = gf2d_sprite_load_all(
            "images/player/ufo.png",
            32,
            32,
            1,
            false);

        if (gfc_input_key_pressed(" ")) {
            player->vel.y = -UFO_JUMP_SPEED * gravityMult;
        }

        // left click shoot
        if (lClick && !prevLClick && shootTimer <= 0) {
            GFC_Vector2D bulletVel = playerToMouse;

            // set speed
            gfc_vector2d_scale(bulletVel, bulletVel, 3);
            gfc_vector2d_add(bulletVel, bulletVel, player->vel);

            bullet_entity_new("images/objects/bullet.png", bulletPos, 32, bulletVel, 1, -1);
            shootTimer = 120;
        }

        // right click shoot
        if (charge >= MAX_CHARGE && rClick && !prevRClick) {
            GFC_Vector2D bulletVel = playerToMouse;

            // set speed
            gfc_vector2d_scale(bulletVel, bulletVel, 2);

            for (int i = 0; i < 360; i += 10) {
                bulletVel = gfc_vector2d_rotate(bulletVel, 10 * GFC_DEGTORAD);
                GFC_Vector2D thisVel = bulletVel;
                gfc_vector2d_add(thisVel, thisVel, player->vel);
                bullet_entity_new("images/objects/bullet.png", bulletPos, 8, thisVel, 0, -1);
            }

            charge = -1;
        }

        break;
    }

    if (charge < MAX_CHARGE) charge++;
    if (practiceTimer > 0) practiceTimer--;
}

void player_update() {
    if (!player) return;

    // slog("player - pos: %f %f   vel: %f %f", player->pos.x, player->pos.y, player->vel.x, player->vel.y);

    // do collision checks for permission
    GFC_Rect levelVCheck = player->hitbox;
    levelVCheck.y += player->vel.y;

    int testV = level_test_rect(level_get(), levelVCheck);

    if (testV) {
        if (testV == 3 || (playerMode == PLAYER_CUBE && player->vel.y * gravityMult < 0) || playerMode == PLAYER_WAVE) { // hit spike, cube jumped into ceiling, or wave
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
    case PLAYER_SHIP:
        gravity = SHIP_GRAVITY;
        break;
    case PLAYER_UFO:
        gravity = UFO_GRAVITY;
        break;
    default:
        gravity = CUBE_GRAVITY;
    }

    if (!player->onGround) player->vel.y += gravity * gravityMult;

    // cap speed if necessary
    if (playerMode == PLAYER_SHIP) {
        // cap going down
        if (player->vel.y * gravityMult >= level_get()->speed) {
            player->vel.y = gravityMult * level_get()->speed;
        }
        // cap going up
        else if (player->vel.y * gravityMult <= -level_get()->speed * 1.5) {
            player->vel.y = gravityMult * -level_get()->speed * 1.5;
        }
    }
    else if (playerMode == PLAYER_UFO && player->vel.y * gravityMult >= level_get()->speed) {
        player->vel.y = gravityMult * level_get()->speed;
    }


    GFC_Vector2D cameraFocus = player->pos;
    // cameraFocus.x += 125;
    // cameraFocus.y += 100 * gravityMult;
    if (playerMode != PLAYER_CUBE) {
        cameraFocus.y = camera_get_center().y;
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
    if (practiceMode) {
        player->pos = practiceCheckpointPos;
        gravityMult = practiceCheckpointGravity;
        flipped = practiceCheckpointFlipped;
        playerMode = practiceCheckpointMode;
    }
    else {
        player->pos.x = 100;
        player->pos.y = 464;
        gravityMult = 1;
        playerMode = PLAYER_CUBE;
    }
    slog("player reset");
    SDL_Delay(250);
}

void player_mode_set(PlayerMode mode) {
    playerMode = mode;
}

PlayerMode player_mode_get() {
    return playerMode;
}

Uint8 player_flipped_get() {
    return flipped;
}

void player_flipped_set(Uint8 flip) {
    flipped = flip;
}

float player_charge_get() {
    return (float)charge / MAX_CHARGE;
}

void player_draw(Entity* player) {
    if (!player || !practiceMode) return;
    GFC_Vector2D pos;
    GFC_Vector2D scale = camera_get_zoom();

    gfc_vector2d_add(pos, practiceCheckpointPos, camera_get_offset());
    pos = gfc_vector2d_multiply(pos, scale);

    scale.x *= 0.5;
    scale.y *= 0.5;

    Sprite* sprite;

    sprite = gf2d_sprite_load_all(
        "images/player/wave2.png",
        32,
        32,
        1,
        false);

    GFC_Vector2D center = { 16, 16 };

    gf2d_sprite_draw(
        sprite,
        pos,
        &scale,
        &center,
        NULL,
        NULL,
        NULL,
        0);
}