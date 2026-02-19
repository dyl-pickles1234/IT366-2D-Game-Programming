#ifndef __MONSTER_H__
#define __MONSTER_H__

#include "entity.h"

Entity* monster_new(GFC_Vector2D pos);
void monster_think(Entity* monster);
void monster_update(Entity* monster);

#endif