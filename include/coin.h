#ifndef __COIN_H__
#define __COIN_H__

#include "player.h"

Entity* coin_entity_new(GFC_Vector2D pos, Uint8 index);
void coin_think(Entity* coin);
void coin_update(Entity* coin);

#endif