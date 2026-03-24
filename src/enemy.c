#include "simple_logger.h"

#include "gfc_input.h"

#include "camera.h"
#include "level.h"

#include "player.h"
#include "enemy.h"

// #define ENEMY_GRAVITY 0.1
#define BLOCK_JUMP_SPEED 3

#define BLOCK_GRAVITY 0.1

Entity* enemy_entity_new(EnemyType type, GFC_Vector2D pos) {
    Entity* self;
    EnemyData* data;
    self = entity_new();
    data = gfc_allocate_array(sizeof(EnemyData), 1);

    if (!self) { slog("failed to create entity for enemy"); return NULL; }

    switch (type)
    {
    case ENEMY_SAW:
        self->sprite = gf2d_sprite_load_all(
            "images/enemies/saw.png",
            32,
            32,
            1,
            false);
        self->center = gfc_vector2d(16, 16);
        self->scale = gfc_vector2d(1, 1);
        self->hitbox = gfc_rect(pos.x - 16, pos.y - 16, 32, 32);
        break;
    case ENEMY_BLOCK:
        self->sprite = gf2d_sprite_load_all(
            "images/enemies/block.png",
            32,
            32,
            1,
            false);
        self->center = gfc_vector2d(16, 16);
        self->scale = gfc_vector2d(0.5, 0.5);
        self->hitbox = gfc_rect(pos.x - 8, pos.y - 8, 16, 16);
        break;
    }

    self->pos = pos;
    self->think = enemy_think;
    self->update = enemy_update;


    data->type = type;
    data->timer = 0;
    data->facing = gfc_vector2d(-1, 0);
    self->data = data;

    return self;
}

void enemy_think(Entity* enemy) {
    if (!enemy) return;
    Entity* player = player_get();
    EnemyData* data = enemy->data;
    int playerTest;

    playerTest = gfc_rect_overlap(enemy->hitbox, player->hitbox);

    if (playerTest) {
        slog("kille dplayer");
        player_reset();
        return;
    }

    GFC_Rect groundCheck = enemy->hitbox;
    groundCheck.y += 1 * player_gravity_get();
    int testGround = level_test_rect(level_get(), groundCheck);
    if (testGround && testGround != 3) {
        enemy->onGround = 1;
    }
    else {
        enemy->onGround = 0;
    }

    GFC_Vector2D toPlayer = player->pos;
    gfc_vector2d_sub(toPlayer, toPlayer, enemy->pos);

    switch (data->type)
    {
    case ENEMY_SAW:
        gfc_vector2d_normalize(&toPlayer);
        if (toPlayer.x > 0) { // if behind the player, start chasing
            gfc_vector2d_scale(enemy->vel, toPlayer, level_get()->speed + 0.5);
        }
        else {
            gfc_vector2d_scale(enemy->vel, toPlayer, 0);
        }
        break;
    case ENEMY_BLOCK:
        if (gfc_vector2d_magnitude_squared(toPlayer) <= 100 * 100 && enemy->onGround) { // if within one tile of player, jump
            enemy->vel.y -= BLOCK_JUMP_SPEED;
        }
        break;
    }
}

void enemy_update(Entity* enemy) {
    if (!enemy) return;

    // do collision checks for permission
    GFC_Rect levelVCheck = enemy->hitbox;
    levelVCheck.y += enemy->vel.y;

    int testV = level_test_rect(level_get(), levelVCheck);

    if (testV) {
        enemy->vel.y = 0;
        enemy->pos.y = (int)(enemy->pos.y / 32) * 32 + (32 - enemy->hitbox.h / 2);

        if (player_gravity_get() < 0) enemy->pos.y += 0.001;
    }

    // apply velocity
    gfc_vector2d_add(enemy->pos, enemy->pos, enemy->vel);

    // update enemy hitbox
    enemy->hitbox.x = enemy->pos.x - enemy->hitbox.w / 2;
    enemy->hitbox.y = enemy->pos.y - enemy->hitbox.h / 2;

    // apply gravity
    EnemyData* data = enemy->data;
    if (data->type == ENEMY_BLOCK) enemy->vel.y += BLOCK_GRAVITY * player_gravity_get();
}