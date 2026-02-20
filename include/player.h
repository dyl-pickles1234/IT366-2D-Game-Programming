#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "entity.h"

void player_entity_new(GFC_Vector2D pos);
void player_think();
void player_update();

#endif