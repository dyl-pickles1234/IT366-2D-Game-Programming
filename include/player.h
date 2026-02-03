#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "entity.h"

Entity* player_entity_new(GFC_Vector2D pos);
void player_think(Entity* player);
void player_update(Entity* player);

#endif