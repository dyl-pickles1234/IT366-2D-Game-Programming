#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "entity.h"

void player_entity_new(GFC_Vector2D pos);
void player_think();
void player_update();

Entity* player_get();

int player_gravity_get();
void player_gravity_set(int newGravity);

void player_reset();
#endif