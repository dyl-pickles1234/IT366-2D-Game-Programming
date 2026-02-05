#ifndef __LEVEL_H__
#define __LEVEL_H__

#include "gf2d_sprite.h"

typedef struct {
    Sprite* bg;
    Sprite* tileset; // sprite containing tiles
    Uint8* tilemap; // pointer to tilemap data
    Uint32 width, height; // size of tilemap
    Uint32 tileWidth, tileHeight; // size of tile
} Level;

/**
 * @brief make a new level
 */
Level* level_new();

/**
 * @brief allocate and generate a level with given parameters
 * @param bg filepath to background image
 * @param tileset filepath to tileset image
 * @param tileWidth width of each tile
 * @param tileHeight height of each tile
 * @param tilesPerLine num of tiles in each line of tileset
 * @param width width of tilemap
 * @param height height of tilemap
 * @return NULL on error or bad params, pointer to level on success
 */
Level* level_create(const char* bg, const char* tileset, Uint32 tileWidth, Uint32 tileHeight, Uint32 tilesPerLine, Uint32 width, Uint32 height);

/**
 * @brief for a level, get index of tilemap for a tile's coords
 * @param level pointer to level
 * @param x x coord of tile
 * @param y y coord of tile
 * @return -1 if level is bad or coords are outside of map, index otherwise
 */
int level_get_tile_index(Level* level, Uint32 x, Uint32 y);

/**
 * @brief add a certain tile around the edges of a given level
 * @param level pointer to level
 * @param tile type of tile to put around the border
 */
void level_add_border(Level* level, Uint8 tile);

/**
 * @brief free a level
 * @param level pointer to level we want to free
 */
void level_free(Level* level);

/**
 * @brief draw a level
 * @param level pointer to level we want to draw
 */
void level_draw(Level* level);

#endif