#include "simple_logger.h"
#include "simple_json.h"

#include "camera.h"
#include "level.h"

#include "pad.h"
#include "orb.h"
#include "portal.h"
#include "enemy.h"

static Level* theLevel = NULL;

static GFC_List* enemies;

Level* level_new() {
    Level* level;

    level = gfc_allocate_array(sizeof(Level), 1);
    if (!level) return NULL;

    enemies = gfc_list_new();

    return level;
}

void level_free(Level* level) {
    if (!level) return;

    gf2d_sprite_free(level->bg);
    gf2d_sprite_free(level->tileset);

    if (level->tilemap) free(level->tilemap);

    free(level);
}

Level* level_load(const char* filepath) {
    Level* level = level_new();
    if (!level) return NULL;

    // level's base JSON
    SJson* levelConfigFile = sj_load(filepath);
    SJson* levelConfig = sj_object_get_value(levelConfigFile, "level");

    // each property in level (SJson)
    SJson* bgJson = sj_object_get_value(levelConfig, "background");
    SJson* tilesetJson = sj_object_get_value(levelConfig, "tileset");
    SJson* tilesheetJson = sj_object_get_value(tilesetJson, "tilesheet");
    SJson* tileWidthJson = sj_object_get_value(tilesetJson, "width");
    SJson* tileHeightJson = sj_object_get_value(tilesetJson, "height");
    SJson* tilesPerRowJson = sj_object_get_value(tilesetJson, "tilesPerRow");
    SJson* speedJson = sj_object_get_value(levelConfig, "speed");
    SJson* tilemapJson = sj_object_get_value(levelConfig, "tilemap");
    SJson* tilemapFirstRowJson = sj_array_get_nth(tilemapJson, 0);
    SJson* objectsJson = sj_object_get_value(levelConfig, "objects");
    SJson* enemiesJson = sj_object_get_value(levelConfig, "enemies");

    // pull out the actual values from JSON
    const char* bgFilename = sj_get_string_value(bgJson);
    const char* tilesheetFilename = sj_get_string_value(tilesheetJson);
    int width, height, tileWidth, tileHeight, tilesPerRow;
    float speed;
    int numObjects = sj_array_get_count(objectsJson);
    int numEnemies = sj_array_get_count(enemiesJson);
    GFC_List* objects = gfc_list_new_size(numObjects);
    GFC_List* enemiesJsonList = gfc_list_new_size(numEnemies);

    sj_get_integer_value(tileWidthJson, &tileWidth);
    sj_get_integer_value(tileHeightJson, &tileHeight);
    sj_get_integer_value(tilesPerRowJson, &tilesPerRow);
    sj_get_float_value(speedJson, &speed);

    width = sj_array_get_count(tilemapFirstRowJson);
    height = sj_array_get_count(tilemapJson);

    for (int i = 0; i < numObjects; i++) {
        gfc_list_append(objects, sj_array_get_nth(objectsJson, i));
    }

    for (int i = 0; i < numEnemies; i++) {
        gfc_list_append(enemiesJsonList, sj_array_get_nth(enemiesJson, i));
    }

    // configure level with all the loaded info!
    level->bg = gf2d_sprite_load_image(bgFilename);
    level->tileset = gf2d_sprite_load_all(
        tilesheetFilename,
        tileWidth,
        tileHeight,
        tilesPerRow,
        true);
    level->width = width;
    level->height = height;
    level->tileWidth = tileWidth;
    level->tileHeight = tileHeight;
    level->speed = speed;
    level->tilemap = gfc_allocate_array(sizeof(Uint8), width * height);

    // fill in the tilemap
    int index, tileValue;
    SJson* rowJson, * tileJson;
    for (int j = 0; j < height; j++) {
        rowJson = sj_array_get_nth(tilemapJson, j);
        for (int i = 0; i < width; i++) {
            tileJson = sj_array_get_nth(rowJson, i);
            index = level_get_tile_index(level, i, j);
            sj_get_integer_value(tileJson, &tileValue);
            level->tilemap[index] = tileValue;
        }
    }

    // create necessary objects
    SJson* object;

    GFC_TextLine type;
    float posX, posY;
    float rot;

    for (int i = 0; i < numObjects; i++) {
        object = gfc_list_get_nth(objects, i);
        strcpy(type, sj_get_string_value(sj_object_get_value(object, "type")));
        sj_get_float_value(sj_array_get_nth(sj_object_get_value(object, "pos"), 0), &posX);
        sj_get_float_value(sj_array_get_nth(sj_object_get_value(object, "pos"), 1), &posY);
        sj_get_float_value(sj_object_get_value(object, "rot"), &rot);

        // fix up position (tile -> pixel, top-down Y -> bottom-up Y)
        posX = posX * 32 + 16;
        posY = (level->height - posY) * 32 - 16;

        // construct entity
        if (gfc_strlcmp(type, "normal_pad") == 0) {
            posY += 16;
            slog("spawning normal pad at %f %f", posX, posY);
            pad_entity_new(PAD_NORMAL, gfc_vector2d(posX, posY));
        }
        else if (gfc_strlcmp(type, "gravity_pad") == 0) {
            posY += 16;
            slog("spawning gravity pad at %f %f", posX, posY);
            pad_entity_new(PAD_GRAVITY, gfc_vector2d(posX, posY));
        }
        else if (gfc_strlcmp(type, "normal_orb") == 0) {
            slog("spawning normal orb at %f %f", posX, posY);
            orb_entity_new(ORB_NORMAL, gfc_vector2d(posX, posY));
        }
        else if (gfc_strlcmp(type, "gravity_orb") == 0) {
            slog("spawning gravity orb at %f %f", posX, posY);
            orb_entity_new(ORB_GRAVITY, gfc_vector2d(posX, posY));
        }
        else if (gfc_strlcmp(type, "cube_portal") == 0) {
            slog("spawning cube portal at %f %f", posX, posY);
            portal_entity_new(PORTAL_CUBE, gfc_vector2d(posX, posY));
        }
        else if (gfc_strlcmp(type, "ship_portal") == 0) {
            slog("spawning ship portal at %f %f", posX, posY);
            portal_entity_new(PORTAL_SHIP, gfc_vector2d(posX, posY));
        }
        else if (gfc_strlcmp(type, "ball_portal") == 0) {
            slog("spawning ball portal at %f %f", posX, posY);
            portal_entity_new(PORTAL_BALL, gfc_vector2d(posX, posY));
        }
        else if (gfc_strlcmp(type, "wave_portal") == 0) {
            slog("spawning wave portal at %f %f", posX, posY);
            portal_entity_new(PORTAL_WAVE, gfc_vector2d(posX, posY));
        }
        else if (gfc_strlcmp(type, "ufo_portal") == 0) {
            slog("spawning ufo portal at %f %f", posX, posY);
            portal_entity_new(PORTAL_UFO, gfc_vector2d(posX, posY));
        }
        else if (gfc_strlcmp(type, "gravity_up_portal") == 0) {
            slog("spawning gravity up portal at %f %f", posX, posY);
            portal_entity_new(PORTAL_GRAVITY_UP, gfc_vector2d(posX, posY));
        }
        else if (gfc_strlcmp(type, "gravity_down_portal") == 0) {
            slog("spawning gravity down portal at %f %f", posX, posY);
            portal_entity_new(PORTAL_GRAVITY_DOWN, gfc_vector2d(posX, posY));
        }
        else if (gfc_strlcmp(type, "flip_flipped_portal") == 0) {
            slog("spawning flip flipped portal at %f %f", posX, posY);
            portal_entity_new(PORTAL_FLIP_FLIPPED, gfc_vector2d(posX, posY));
        }
        else if (gfc_strlcmp(type, "flip_normal_portal") == 0) {
            slog("spawning flip normal portal at %f %f", posX, posY);
            portal_entity_new(PORTAL_FLIP_NORMAL, gfc_vector2d(posX, posY));
        }
    }

    // create necessary enemies
    SJson* enemy;

    GFC_TextLine enemyType;
    float enemyPosX, enemyPosY;

    for (int i = 0; i < numEnemies; i++) {
        enemy = gfc_list_get_nth(enemiesJsonList, i);
        strcpy(enemyType, sj_get_string_value(sj_object_get_value(enemy, "type")));
        sj_get_float_value(sj_array_get_nth(sj_object_get_value(enemy, "pos"), 0), &enemyPosX);
        sj_get_float_value(sj_array_get_nth(sj_object_get_value(enemy, "pos"), 1), &enemyPosY);

        // fix up position (tile -> pixel, top-down Y -> bottom-up Y)
        enemyPosX = enemyPosX * 32 + 16;
        enemyPosY = (level->height - enemyPosY) * 32 - 16;

        Entity* ent = NULL;

        // construct entity
        if (gfc_strlcmp(enemyType, "saw") == 0) {
            slog("spawning enemy saw at %f %f", enemyPosX, enemyPosY);
            ent = enemy_entity_new(ENEMY_SAW, gfc_vector2d(enemyPosX, enemyPosY));
        }
        else if (gfc_strlcmp(enemyType, "block") == 0) {
            slog("spawning enemy block at %f %f", enemyPosX, enemyPosY);
            ent = enemy_entity_new(ENEMY_BLOCK, gfc_vector2d(enemyPosX, enemyPosY));
        }

        if (ent) gfc_list_append(enemies, ent);
    }

    camera_set_bounds(gfc_rect(0, 0, level->tileWidth * level->width, level->tileHeight * level->height));

    sj_free(levelConfigFile);

    return level;
}

GFC_List* level_enemies_get() {
    return enemies;
}

int level_get_tile_index(Level* level, Uint32 x, Uint32 y) {
    if (!level || !level->tilemap) return -1;
    if (x >= level->width) return -1;
    if (y >= level->height) return -1;

    return y * level->width + x;
}

void level_set(Level* level) {
    theLevel = level;
}

Level* level_get() {
    return theLevel;
}

Uint8 level_test_rect(Level* level, GFC_Rect playerRect) {
    int index;

    for (int j = 0; j < level->height; j++) {
        for (int i = 0; i < level->width; i++) {
            index = level_get_tile_index(level, i, j);
            if (index < 0) continue;
            if (level->tilemap[index] == 0) continue;

            GFC_Vector2D pos = gfc_vector2d(i * level->tileWidth, j * level->tileHeight);;
            GFC_Rect tileRect = { 0 };

            if (level->tilemap[index] == 3) { // spike
                tileRect = gfc_rect(pos.x + ((32 / 5) * 2), pos.y + (32 / 4), (32 / 5), (32 / 2));
            }
            else {
                tileRect = gfc_rect(pos.x, pos.y, level->tileWidth, level->tileHeight);
            }

            if (gfc_rect_overlap(playerRect, tileRect)) {
                // slog("contacted tile %i %i", i, j);
                return level->tilemap[index];
            }
        }
    }

    return 0;
}

void level_draw(Level* level) {
    if (!level) return;

    if (level->bg) {
        gf2d_sprite_draw_image(level->bg, gfc_vector2d(0, 0));
    }

    if (level->tileset) {
        int index;
        Uint8 tile;
        GFC_Vector2D scale = camera_get_zoom();

        for (int j = 0; j < level->height; j++) {
            for (int i = 0; i < level->width; i++) {
                index = level_get_tile_index(level, i, j);
                if (index < 0) continue;
                tile = level->tilemap[index];
                if (!tile) continue;
                GFC_Vector2D pos = gfc_vector2d(i * level->tileWidth * scale.x, j * level->tileHeight * scale.y);
                GFC_Vector2D offset = camera_get_offset();
                offset = gfc_vector2d_multiply(offset, scale);
                gfc_vector2d_add(pos, pos, offset);
                gf2d_sprite_draw(
                    level->tileset,
                    pos,
                    &scale,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    tile - 1);
            }
        }
    }
}