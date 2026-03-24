#include "simple_logger.h"

#include "gfc_input.h"

#include "camera.h"
#include "level.h"

#include "player.h"
#include "bullet.h"

#define BULLET_GRAVITY 0.1

Entity* bullet_entity_new(char* spriteName, GFC_Vector2D pos, float size, GFC_Vector2D vel, Uint8 hasGravity, int ttl) {
    Entity* self;
    BulletData* data;
    self = entity_new();
    data = gfc_allocate_array(sizeof(BulletData), 1);

    if (!self) { slog("failed to create entity for bullet"); return NULL; }

    self->sprite = gf2d_sprite_load_all(
        spriteName,
        32,
        32,
        1,
        false);

    self->pos = pos;
    self->vel = vel;
    self->center = gfc_vector2d(16, 16);
    self->scale = gfc_vector2d(size / 32, size / 32);
    self->think = bullet_think;
    self->update = bullet_update;

    self->hitbox = gfc_rect(pos.x - size / 2, pos.y - size / 2, size, size);

    data->hasGravity = hasGravity;

    if (ttl > 0) {
        data->ttl = ttl;
    }
    else {
        data->ttl = 300;
    }

    self->data = data;

    return self;
}

void bullet_think(Entity* bullet) {
    if (!bullet) return;

    GFC_List* enemies = level_enemies_get();
    Entity* enemy;
    int enemyTest;

    for (int i = 0; i < gfc_list_get_count(enemies); i++) {
        enemy = gfc_list_get_nth(enemies, i);

        enemyTest = gfc_rect_overlap(bullet->hitbox, enemy->hitbox);

        if (enemyTest) {
            slog("HIT EM");
            gfc_list_delete_nth(enemies, i);
            entity_free(enemy);
            entity_free(bullet);
            break;
        }
    }
}

void bullet_update(Entity* bullet) {
    if (!bullet) return;

    // apply velocity
    gfc_vector2d_add(bullet->pos, bullet->pos, bullet->vel);

    // update bullet hitbox
    bullet->hitbox.x = bullet->pos.x - bullet->hitbox.w / 2;
    bullet->hitbox.y = bullet->pos.y - bullet->hitbox.h / 2;

    // apply gravity
    float gravity = BULLET_GRAVITY;

    BulletData* data = bullet->data;

    if (data->hasGravity) bullet->vel.y += gravity * player_gravity_get();

    if (data->ttl > 0) data->ttl--;
    else if (data->ttl <= 0) entity_free(bullet);
}