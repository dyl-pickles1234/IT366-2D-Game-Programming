#ifndef __BOSS_H__
#define __BOSS_H__

#include "entity.h"

typedef enum {
    BOSS_STATE_IDLE = 0,
    BOSS_STATE_ROAMING,
    BOSS_STATE_TARGETING,
    BOSS_STATE_ATTACKING,
    BOSS_STATE_END
} BossState;

typedef enum {
    BOSS_SIDESCROLLER,
    BOSS_ARENA
} BossType;

typedef struct {
    BossType type;
    BossState state;
    Uint32 timer;
    Uint32 cycles;
    int health;
} BossData;

Entity* boss_new(BossType type, GFC_Vector2D pos);
void boss_think(Entity* boss);
void boss_update(Entity* boss);

#endif