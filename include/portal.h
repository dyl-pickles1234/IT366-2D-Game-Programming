#ifndef __PORTAL_H__
#define __PORTAL_H__

#include "entity.h"

typedef enum {
    PORTAL_CUBE = 0,
    PORTAL_SHIP,
    PORTAL_BALL,
    PORTAL_WAVE,
    PORTAL_UFO,
} PortalType;

typedef struct {
    PortalType type;
} PortalData;

Entity* portal_entity_new(PortalType type, GFC_Vector2D pos);
void portal_think(Entity* portal);
void portal_update(Entity* portal);
void portal_draw(Entity* portal);

#endif