#ifndef __BULLET_H__
#define __BULLET_H__

#include "entity.h"

typedef struct {
    Uint8 hasGravity;
    int ttl;
} BulletData;

Entity* bullet_entity_new(char* spriteName, GFC_Vector2D pos, float size, GFC_Vector2D vel, Uint8 hasGravity, int ttl);
void bullet_think(Entity* bullet);
void bullet_update(Entity* bullet);

#endif