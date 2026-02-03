#ifndef __ENTITY_H__
#define __ENTITY_H__

#include <SDL.h>

#include "gfc_text.h"
#include "gf2d_sprite.h"

typedef struct Entity_s {
    Uint8 _inuse;
    GFC_TextLine name; // entity name
    GFC_Vector2D pos;
    GFC_Vector2D vel;
    float speed;
    GFC_Vector2D scale;
    GFC_Vector2D center;
    float rotation;
    Sprite* sprite;
    float frame;

    void (*think) (struct Entity_s* self);
    void (*update) (struct Entity_s* self);
    void (*free) (struct Entity_s* self);
} Entity;

/**
 * @brief init the entity system
 * @param max maxium num of entities that can exist at once
 */
void entity_manager_init(Uint32 max);

/**
 * @brief draw all active entities
 */
void entity_manager_draw_all();

/**
 * @brief think all active entities
 */
void entity_manager_think_all();

/**
 * @brief update all active entities
 */
void entity_manager_update_all();

/**
 * @brief get free entity
 * @return NULL if out of ents; otherwise pointer to blank entity
 */
Entity* entity_new();

/**
 * @brief free a new'd entity
 * @param self entity to free
 */
void entity_free(Entity* self);

#endif