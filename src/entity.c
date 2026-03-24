#include "simple_logger.h"

#include "gf2d_draw.h"

#include "camera.h"
#include "entity.h"

typedef struct {
    Entity* entityList;
    Uint32 maxEntities;
} EntityManager;

static EntityManager entityManager = { 0 };

void entity_manager_close();

void entity_manager_init(Uint32 max) {
    if (!max) { slog("cannot init ent system with 0 ents"); return; }

    entityManager.entityList = gfc_allocate_array(sizeof(Entity), max);

    if (!entityManager.entityList) { slog("failed to allocate %i entities", max); return; }

    entityManager.maxEntities = max;
    slog("Entity manager initialized");

    atexit(entity_manager_close);
}

void entity_manager_close() {
    if (!entityManager.entityList) return;

    for (int i = 0; i < entityManager.maxEntities; i++) {
        entity_free(&entityManager.entityList[i]);
    }
    free(entityManager.entityList);
    memset(&entityManager, 0, sizeof(EntityManager));
    slog("Entity manager closed");
}

Entity* entity_new() {
    if (!entityManager.entityList) { slog("Entity manager not initialized"); return NULL; }

    for (int i = 0; i < entityManager.maxEntities; i++) {
        if (entityManager.entityList[i]._inuse) continue;

        entityManager.entityList[i]._inuse = 1;

        // set defaults
        entityManager.entityList[i].scale = gfc_vector2d(1, 1);

        return &entityManager.entityList[i];
    }

    return NULL;
}

void entity_free(Entity* self) {
    if (!self) return;

    if (self->sprite) gf2d_sprite_free(self->sprite);

    memset(self, 0, sizeof(Entity));
}

void entity_draw(Entity* ent) {
    if (!ent) return;

    GFC_Vector2D pos;
    GFC_Vector2D scale = camera_get_zoom();

    gfc_vector2d_add(pos, ent->pos, camera_get_offset());
    pos = gfc_vector2d_multiply(pos, scale);

    scale.x *= ent->scale.x;
    scale.y *= ent->scale.y;

    gf2d_sprite_draw(
        ent->sprite,
        pos,
        &scale,
        &ent->center,
        &ent->rotation,
        NULL,
        NULL,
        (Uint32)ent->frame);

    // debug draw all hitboxes
    GFC_Rect hitboxScreen;
    hitboxScreen.w = ent->hitbox.w * camera_get_zoom().x;
    hitboxScreen.h = ent->hitbox.h * camera_get_zoom().y;
    hitboxScreen.x = pos.x - hitboxScreen.w / 2;
    hitboxScreen.y = pos.y - hitboxScreen.h / 2;
    gf2d_draw_rect(hitboxScreen, GFC_COLOR_DARKRED);

    if (ent->draw) {
        ent->draw(ent);
    }
}

void entity_manager_draw_all() {
    if (!entityManager.entityList) { slog("Entity manager not initialized"); return; }

    for (int i = 0; i < entityManager.maxEntities; i++) {
        if (!entityManager.entityList[i]._inuse) continue;

        entity_draw(&entityManager.entityList[i]);
    }
}

void entity_manager_think_all() {
    if (!entityManager.entityList) { slog("Entity manager not initialized"); return; }

    for (int i = 0; i < entityManager.maxEntities; i++) {
        if (!entityManager.entityList[i]._inuse) continue;
        if (!entityManager.entityList[i].think) continue;

        entityManager.entityList[i].think(&entityManager.entityList[i]);
    }
}

void entity_manager_update_all() {
    if (!entityManager.entityList) { slog("Entity manager not initialized"); return; }

    for (int i = 0; i < entityManager.maxEntities; i++) {
        if (!entityManager.entityList[i]._inuse) continue;
        if (!entityManager.entityList[i].update) continue;

        entityManager.entityList[i].update(&entityManager.entityList[i]);
    }
}