#if 0
#include "simple_logger.h"

#include "gfc_input.h"

#include "level.h"
#include "player.h"

#include "boss.h"

Entity* boss_entity_new(char* spriteName, GFC_Vector2D pos, float size, GFC_Vector2D vel, Uint8 hasGravity, int ttl) {
    Entity* self;
    BossData* data;
    self = entity_new();
    data = gfc_allocate_array(sizeof(BossData), 1);

    if (!self) { slog("failed to create entity for boss"); return NULL; }

    gfc_line_cpy(self->name, "boss");
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
    self->think = boss_think;
    self->update = boss_update;

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

void boss_think(Entity* boss) {
    if (!boss) return;

    GFC_List* enemies = level_enemies_get();
    Entity* enemy;
    int enemyTest;

    for (int i = 0; i < gfc_list_get_count(enemies); i++) {
        enemy = gfc_list_get_nth(enemies, i);

        enemyTest = gfc_rect_overlap(boss->hitbox, enemy->hitbox);

        if (enemyTest) {
            slog("HIT EM");
            gfc_list_delete_nth(enemies, i);
            entity_free(enemy);
            entity_free(boss);
            break;
        }
    }
}

void boss_update(Entity* boss) {
    if (!boss) return;

    // apply velocity
    gfc_vector2d_add(boss->pos, boss->pos, boss->vel);

    // update boss hitbox
    boss->hitbox.x = boss->pos.x - boss->hitbox.w / 2;
    boss->hitbox.y = boss->pos.y - boss->hitbox.h / 2;

    // apply gravity
    float gravity = BOSS_GRAVITY;

    BossData* data = boss->data;

    if (data->hasGravity) boss->vel.y += gravity * player_gravity_get();

    if (data->ttl > 0) data->ttl--;
    else if (data->ttl <= 0) entity_free(boss);
}

#endif