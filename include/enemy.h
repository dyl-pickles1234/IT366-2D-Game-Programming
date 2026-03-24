#ifndef __ENEMY_H__
#define __ENEMY_H__

#include "entity.h"

typedef enum {
    ENEMY_SAW = 0,
    ENEMY_BLOCK,
    ENEMY_CHOMP,
    ENEMY_TURRET,
    ENEMY_LASER
} EnemyType;

typedef struct {
    EnemyType type;
    GFC_Vector2D facing;
    int timer;
} EnemyData;

Entity* enemy_entity_new(EnemyType type, GFC_Vector2D pos);
void enemy_think(Entity* enemy);
void enemy_update(Entity* enemy);

#endif