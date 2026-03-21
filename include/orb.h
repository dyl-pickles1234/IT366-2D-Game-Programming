#ifndef __ORB_H__
#define __ORB_H__

#include "entity.h"

typedef enum {
    ORB_NORMAL,
    ORB_GRAVITY
} OrbType;

Entity* orb_entity_new(OrbType type, GFC_Vector2D pos);
void orb_think(Entity* orb);
void orb_update(Entity* orb);

#endif