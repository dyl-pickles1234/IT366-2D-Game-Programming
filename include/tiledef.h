#ifndef __TILEDEF_H__
#define __TILEDEF_H__

#include "gf2d_sprite.h"

typedef struct {
    Sprite* sprite;
    Uint32 width, height, fpl;
} TileDef;

#endif