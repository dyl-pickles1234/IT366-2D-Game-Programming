#ifndef __PAD_H__
#define __PAD_H__

#include "entity.h"

typedef enum {
    PAD_NORMAL,
    PAD_GRAVITY
} PadType;

Entity* pad_entity_new(PadType type, GFC_Vector2D pos);
void pad_think(Entity* pad);
void pad_update(Entity* pad);

#endif