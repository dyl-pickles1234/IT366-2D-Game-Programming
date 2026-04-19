#ifndef __LEVEL_H__
#define __LEVEL_H__

#include "gf2d_sprite.h"
#include "entity.h"

#include "enemy.h"

typedef struct {
    Sprite* bg;
    Sprite* tileset; // sprite containing tiles
    Uint8* tilemap; // pointer to tilemap data
    Uint32 width, height; // size of tilemap
    Uint32 tileWidth, tileHeight; // size of tile
    float speed;
} Level;

typedef enum {
    OBJECT_OBJECT_ORB_NORMAL = 0,
    OBJECT_OBJECT_ORB_SMALL,
    OBJECT_OBJECT_ORB_GRAVITY,
    OBJECT_OBJECT_PAD_NORMAL,
    OBJECT_OBJECT_PAD_SMALL,
    OBJECT_OBJECT_PAD_GRAVITY,
    OBJECT_OBJECT_PORTAL_CUBE,
    OBJECT_OBJECT_PORTAL_SHIP,
    OBJECT_OBJECT_PORTAL_BALL,
    OBJECT_OBJECT_PORTAL_WAVE,
    OBJECT_OBJECT_PORTAL_UFO,
    OBJECT_OBJECT_PORTAL_GRAVITY_UP,
    OBJECT_OBJECT_PORTAL_GRAVITY_DOWN,
    OBJECT_OBJECT_PORTAL_FLIP_FLIPPED,
    OBJECT_OBJECT_PORTAL_FLIP_NORMAL,
    OBJECT_OBJECT_END
} LevelObjectType;

/**
 * @brief make a new level
 */
Level* level_new();

// /**
//  * @brief allocate and generate a level with given parameters
//  * @param bg filepath to background image
//  * @param tileset filepath to tileset image
//  * @param tileWidth width of each tile
//  * @param tileHeight height of each tile
//  * @param tilesPerLine num of tiles in each line of tileset
//  * @param width width of tilemap
//  * @param height height of tilemap
//  * @return NULL on error or bad params, pointer to level on success
//  */
// Level* level_create(const char* bg, const char* tileset, Uint32 tileWidth, Uint32 tileHeight, Uint32 tilesPerLine, Uint32 width, Uint32 height);

/**
 * @brief load a level from json config
 * @param filepath filepath of json-formatted config file that describes the level
 */
Level* level_load(const char* filepath);

void level_save(const char* filepath);

void level_set(Level* level);

GFC_List* level_enemies_get();

Level* level_get();

/**
 * @brief for a level, get index of tilemap for a tile's coords
 * @param level pointer to level
 * @param x x coord of tile
 * @param y y coord of tile
 * @return -1 if level is bad or coords are outside of map, index otherwise
 */
int level_get_tile_index(Level* level, Uint32 x, Uint32 y);

Uint8 level_test_rect(Level* level, GFC_Rect hitbox);

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

void level_construct_object_from_name(GFC_TextLine type, float posX, float posY, float rot);
void level_construct_object(LevelObjectType type, float posX, float posY, float rot);

Entity* level_construct_enemy_from_name(GFC_TextLine type, float posX, float posY, float rot);
Entity* level_construct_enemy(EnemyType type, float posX, float posY, float rot);

#endif