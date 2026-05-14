#include "simple_logger.h"
#include "simple_json.h"

#include "gf2d_draw.h"

#include "camera.h"
#include "level.h"

#include "pad.h"
#include "orb.h"
#include "portal.h"
#include "enemy.h"
#include "player.h"
#include "audio.h"
#include "coin.h"

static Level* theLevel = NULL;

static GFC_List* objects;
static GFC_List* enemies;

Level* level_new() {
    Level* level;

    level = gfc_allocate_array(sizeof(Level), 1);
    if (!level) return NULL;

    objects = gfc_list_new();
    enemies = gfc_list_new();

    level->coins = gfc_allocate_array(sizeof(Uint8), 3);

    return level;
}

void level_free(Level* level) {
    if (!level) return;

    Mix_HaltChannel(-1);

    free(level->coins);

    gfc_sound_free(level->song);

    gf2d_sprite_free(level->bg);
    gf2d_sprite_free(level->tileset);

    for (int i = 0; i < objects->count; i++) {
        entity_free(gfc_list_get_nth(objects, i));
    }

    for (int i = 0; i < enemies->count; i++) {
        entity_free(gfc_list_get_nth(enemies, i));
    }

    gfc_list_delete(level->beats);

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
    SJson* songJson = sj_object_get_value(levelConfig, "song");
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
    const char* songFilename = sj_get_string_value(songJson);
    const char* bgFilename = sj_get_string_value(bgJson);
    const char* tilesheetFilename = sj_get_string_value(tilesheetJson);
    int width, height, tileWidth, tileHeight, tilesPerRow;
    float speed;
    int numObjects = sj_array_get_count(objectsJson);
    int numEnemies = sj_array_get_count(enemiesJson);
    GFC_List* objectsJsonList = gfc_list_new_size(numObjects + 1);
    GFC_List* enemiesJsonList = gfc_list_new_size(numEnemies + 1);

    sj_get_integer_value(tileWidthJson, &tileWidth);
    sj_get_integer_value(tileHeightJson, &tileHeight);
    sj_get_integer_value(tilesPerRowJson, &tilesPerRow);
    sj_get_float_value(speedJson, &speed);

    width = sj_array_get_count(tilemapFirstRowJson);
    height = sj_array_get_count(tilemapJson);

    for (int i = 0; i < numObjects; i++) {
        gfc_list_append(objectsJsonList, sj_array_get_nth(objectsJson, i));
    }

    for (int i = 0; i < numEnemies; i++) {
        gfc_list_append(enemiesJsonList, sj_array_get_nth(enemiesJson, i));
    }

    // configure level with all the loaded info!
    gfc_line_cpy(level->filepath, filepath);
    level->song = gfc_sound_load(songFilename, 0.5f, 0);
    level->beats = get_beats(level->song);
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
    void* data = NULL; // optional; only used for coin

    for (int i = 0; i < numObjects; i++) {
        object = gfc_list_get_nth(objectsJsonList, i);
        strcpy(type, sj_get_string_value(sj_object_get_value(object, "type")));
        sj_get_float_value(sj_array_get_nth(sj_object_get_value(object, "pos"), 0), &posX);
        sj_get_float_value(sj_array_get_nth(sj_object_get_value(object, "pos"), 1), &posY);
        sj_get_float_value(sj_object_get_value(object, "rot"), &rot);

        if (gfc_stricmp(type, "coin") == 0) {
            int index;
            sj_get_integer_value(sj_object_get_value(object, "index"), &index);

            // store the fact that this coin exists
            level->coins[index] = 1;

            // but don't spawn it if collected
            Uint8* collectedCoins = gfc_hashmap_get(player_get_level_coins(), filepath);
            if (collectedCoins[index]) continue;

            data = (void*)index;
        }

        // fix up position (tile -> pixel, top-down Y -> bottom-up Y)
        posX = posX * 32 + 16;
        posY = (level->height - posY) * 32 - 16;

        // construct entity
        level_construct_object_from_name(type, posX, posY, rot, data);
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

        // construct entity
        level_construct_enemy_from_name(enemyType, enemyPosX, enemyPosY, 0);
    }

    camera_set_bounds(gfc_rect(0, 0, level->tileWidth * level->width, level->tileHeight * level->height));

    sj_free(levelConfigFile);

    return level;
}

void level_save(const char* filepath) {
    if (!theLevel) return;

    // level's base JSON
    SJson* levelConfigFile = sj_object_new();
    SJson* levelConfig = sj_object_new();

    // each property in level (SJson)
    sj_object_insert(levelConfig, "song", sj_new_str(theLevel->song->filepath));
    sj_object_insert(levelConfig, "background", sj_new_str(theLevel->bg->filepath));

    SJson* tilesetJson = sj_object_new();
    sj_object_insert(tilesetJson, "tilesheet", sj_new_str(theLevel->tileset->filepath));
    sj_object_insert(tilesetJson, "width", sj_new_int(theLevel->tileWidth));
    sj_object_insert(tilesetJson, "height", sj_new_int(theLevel->tileHeight));
    sj_object_insert(tilesetJson, "tilesPerRow", sj_new_int(theLevel->tileset->frames_per_line));
    sj_object_insert(levelConfig, "tileset", tilesetJson);

    sj_object_insert(levelConfig, "speed", sj_new_float(theLevel->speed));

    SJson* tilemapJson = sj_array_new();
    for (int j = 0; j < theLevel->height; j++) {
        SJson* rowJson = sj_array_new();
        for (int i = 0; i < theLevel->width; i++) {
            sj_array_append(rowJson, sj_new_int(theLevel->tilemap[level_get_tile_index(theLevel, i, j)]));
        }
        sj_array_append(tilemapJson, rowJson);
    }
    sj_object_insert(levelConfig, "tilemap", tilemapJson);

    SJson* objectsJson = sj_array_new();
    for (int i = 0; i < objects->count; i++) {
        Entity* object = gfc_list_nth(objects, i);

        SJson* objectJson = sj_object_new();

        sj_object_insert(objectJson, "type", sj_new_str(object->name));

        SJson* posJson = sj_array_new();

        // fix up position (tile -> pixel, top-down Y -> bottom-up Y)
        GFC_Vector2D objSavePos;
        objSavePos.x = (object->pos.x - 16) / theLevel->tileWidth;
        objSavePos.y = theLevel->height - ((object->pos.y + 16) / theLevel->tileHeight);

        sj_array_append(posJson, sj_new_float(objSavePos.x));
        sj_array_append(posJson, sj_new_float(objSavePos.y));
        sj_object_insert(objectJson, "pos", posJson);

        sj_object_insert(objectJson, "rot", sj_new_float(object->rotation));

        if (gfc_stricmp(object->name, "coin") == 0) {
            sj_object_insert(objectJson, "index", sj_new_int((Uint8)object->data));
        }

        sj_array_append(objectsJson, objectJson);
    }
    sj_object_insert(levelConfig, "objects", objectsJson);

    SJson* enemiesJson = sj_array_new();
    for (int i = 0; i < enemies->count; i++) {
        Entity* enemy = gfc_list_nth(enemies, i);

        SJson* enemyJson = sj_object_new();

        sj_object_insert(enemyJson, "type", sj_new_str(enemy->name));

        SJson* posJson = sj_array_new();

        // fix up position (tile -> pixel, top-down Y -> bottom-up Y)
        GFC_Vector2D enemySavePos;
        enemySavePos.x = (enemy->pos.x - 16) / theLevel->tileWidth;
        enemySavePos.y = theLevel->height - ((enemy->pos.y + 16) / theLevel->tileHeight);

        sj_array_append(posJson, sj_new_float(enemySavePos.x));
        sj_array_append(posJson, sj_new_float(enemySavePos.y));
        sj_object_insert(enemyJson, "pos", posJson);

        sj_array_append(enemiesJson, enemyJson);
    }
    sj_object_insert(levelConfig, "enemies", enemiesJson);

    sj_object_insert(levelConfigFile, "level", levelConfig);

    sj_save(levelConfigFile, filepath);
    sj_free(levelConfigFile);
}

GFC_List* level_objects_get() {
    return objects;
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

    if (player_editor_mode_get() && level->beats) {
        GFC_Vector2D scale = camera_get_zoom();
        GFC_Vector2D offset = camera_get_offset();
        offset = gfc_vector2d_multiply(offset, scale);

        for (int i = 0; i < level->beats->count; i++) {
            float scl = 360 / level->speed; // magic numbers hell yeah
            float off = 100;
            gf2d_draw_line(gfc_vector2d((((int)gfc_list_get_nth(level->beats, i)) / scl + off) * scale.x + offset.x, 0), gfc_vector2d((((int)gfc_list_get_nth(level->beats, i)) / scl + off) * scale.x + offset.x, 768), GFC_COLOR_DARKCYAN);
        }
    }

    if (level->tileset) {
        int index;
        Uint8 tile;
        GFC_Vector2D scale = camera_get_zoom();
        int player_tile = player_get()->pos.x / 32;

        for (int j = 0; j < level->height; j++) {
            for (int i = MAX(0, player_tile - 15); i < MIN(level->width, player_tile + 20); i++) {
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

void level_construct_object_from_name(GFC_TextLine type, float posX, float posY, float rot, void* data) {
    LevelObjectType enum_type = OBJECT_OBJECT_END;

    if (gfc_strlcmp(type, "normal_pad") == 0) {
        enum_type = OBJECT_OBJECT_PAD_NORMAL;
    }
    else if (gfc_strlcmp(type, "small_pad") == 0) {
        enum_type = OBJECT_OBJECT_PAD_SMALL;
    }
    else if (gfc_strlcmp(type, "gravity_pad") == 0) {
        enum_type = OBJECT_OBJECT_PAD_GRAVITY;
    }
    else if (gfc_strlcmp(type, "normal_orb") == 0) {
        enum_type = OBJECT_OBJECT_ORB_NORMAL;
    }
    else if (gfc_strlcmp(type, "small_orb") == 0) {
        enum_type = OBJECT_OBJECT_ORB_SMALL;
    }
    else if (gfc_strlcmp(type, "gravity_orb") == 0) {
        enum_type = OBJECT_OBJECT_ORB_GRAVITY;
    }
    else if (gfc_strlcmp(type, "coin") == 0) {
        enum_type = OBJECT_OBJECT_COIN;
    }
    else if (gfc_strlcmp(type, "cube_portal") == 0) {
        enum_type = OBJECT_OBJECT_PORTAL_CUBE;
    }
    else if (gfc_strlcmp(type, "ship_portal") == 0) {
        enum_type = OBJECT_OBJECT_PORTAL_SHIP;
    }
    else if (gfc_strlcmp(type, "ball_portal") == 0) {
        enum_type = OBJECT_OBJECT_PORTAL_BALL;
    }
    else if (gfc_strlcmp(type, "wave_portal") == 0) {
        enum_type = OBJECT_OBJECT_PORTAL_WAVE;
    }
    else if (gfc_strlcmp(type, "ufo_portal") == 0) {
        enum_type = OBJECT_OBJECT_PORTAL_UFO;
    }
    else if (gfc_strlcmp(type, "gravity_up_portal") == 0) {
        enum_type = OBJECT_OBJECT_PORTAL_GRAVITY_UP;
    }
    else if (gfc_strlcmp(type, "gravity_down_portal") == 0) {
        enum_type = OBJECT_OBJECT_PORTAL_GRAVITY_DOWN;
    }
    else if (gfc_strlcmp(type, "flip_flipped_portal") == 0) {
        enum_type = OBJECT_OBJECT_PORTAL_FLIP_FLIPPED;
    }
    else if (gfc_strlcmp(type, "flip_normal_portal") == 0) {
        enum_type = OBJECT_OBJECT_PORTAL_FLIP_NORMAL;
    }

    level_construct_object(enum_type, posX, posY, rot, data);
}

void level_construct_object(LevelObjectType type, float posX, float posY, float rot, void* data) {
    Entity* obj = NULL;

    switch (type)
    {
    case OBJECT_OBJECT_PAD_NORMAL:
        // slog("spawning normal pad at %f %f", posX, posY);
        obj = pad_entity_new(PAD_NORMAL, gfc_vector2d(posX, posY));
        break;
    case OBJECT_OBJECT_PAD_SMALL:
        // slog("spawning small pad at %f %f", posX, posY);
        obj = pad_entity_new(PAD_SMALL, gfc_vector2d(posX, posY));
        break;
    case OBJECT_OBJECT_PAD_GRAVITY:
        // slog("spawning gravity pad at %f %f", posX, posY);
        obj = pad_entity_new(PAD_GRAVITY, gfc_vector2d(posX, posY));
        break;
    case OBJECT_OBJECT_ORB_NORMAL:
        // slog("spawning normal orb at %f %f", posX, posY);
        obj = orb_entity_new(ORB_NORMAL, gfc_vector2d(posX, posY));
        break;
    case OBJECT_OBJECT_ORB_SMALL:
        // slog("spawning small orb at %f %f", posX, posY);
        obj = orb_entity_new(ORB_SMALL, gfc_vector2d(posX, posY));
        break;
    case OBJECT_OBJECT_ORB_GRAVITY:
        // slog("spawning gravity orb at %f %f", posX, posY);
        obj = orb_entity_new(ORB_GRAVITY, gfc_vector2d(posX, posY));
        break;
    case OBJECT_OBJECT_COIN:
        // slog("spawning coin at %f %f with index %i", posX, posY, (Uint8)data);
        obj = coin_entity_new(gfc_vector2d(posX, posY), (Uint8)data);
        break;
    case OBJECT_OBJECT_PORTAL_CUBE:
        // slog("spawning cube portal at %f %f", posX, posY);
        obj = portal_entity_new(PORTAL_CUBE, gfc_vector2d(posX, posY));
        break;
    case OBJECT_OBJECT_PORTAL_SHIP:
        // slog("spawning ship portal at %f %f", posX, posY);
        obj = portal_entity_new(PORTAL_SHIP, gfc_vector2d(posX, posY));
        break;
    case OBJECT_OBJECT_PORTAL_BALL:
        // slog("spawning ball portal at %f %f", posX, posY);
        obj = portal_entity_new(PORTAL_BALL, gfc_vector2d(posX, posY));
        break;
    case OBJECT_OBJECT_PORTAL_WAVE:
        // slog("spawning wave portal at %f %f", posX, posY);
        obj = portal_entity_new(PORTAL_WAVE, gfc_vector2d(posX, posY));
        break;
    case OBJECT_OBJECT_PORTAL_UFO:
        // slog("spawning ufo portal at %f %f", posX, posY);
        obj = portal_entity_new(PORTAL_UFO, gfc_vector2d(posX, posY));
        break;
    case OBJECT_OBJECT_PORTAL_GRAVITY_UP:
        // slog("spawning gravity up portal at %f %f", posX, posY);
        obj = portal_entity_new(PORTAL_GRAVITY_UP, gfc_vector2d(posX, posY));
        break;
    case OBJECT_OBJECT_PORTAL_GRAVITY_DOWN:
        // slog("spawning gravity down portal at %f %f", posX, posY);
        obj = portal_entity_new(PORTAL_GRAVITY_DOWN, gfc_vector2d(posX, posY));
        break;
    case OBJECT_OBJECT_PORTAL_FLIP_FLIPPED:
        // slog("spawning flip flipped portal at %f %f", posX, posY);
        obj = portal_entity_new(PORTAL_FLIP_FLIPPED, gfc_vector2d(posX, posY));
        break;
    case OBJECT_OBJECT_PORTAL_FLIP_NORMAL:
        // slog("spawning flip normal portal at %f %f", posX, posY);
        obj = portal_entity_new(PORTAL_FLIP_NORMAL, gfc_vector2d(posX, posY));
        break;
    }

    if (obj) gfc_list_append(objects, obj);
}

Entity* level_construct_enemy_from_name(GFC_TextLine type, float posX, float posY, float rot) {
    Entity* ent = NULL;
    EnemyType enum_type = ENEMY_END;

    if (gfc_strlcmp(type, "saw") == 0) {
        enum_type = ENEMY_SAW;
    }
    else if (gfc_strlcmp(type, "block") == 0) {
        enum_type = ENEMY_BLOCK;
    }

    ent = level_construct_enemy(enum_type, posX, posY, rot);
    return ent;
}

Entity* level_construct_enemy(EnemyType type, float posX, float posY, float rot) {
    Entity* ent = NULL;

    switch (type)
    {
    case ENEMY_SAW:
        // slog("spawning enemy saw at %f %f", posX, posY);
        ent = enemy_entity_new(ENEMY_SAW, gfc_vector2d(posX, posY));
        break;
    case ENEMY_BLOCK:
        // slog("spawning enemy block at %f %f", posX, posY);
        ent = enemy_entity_new(ENEMY_BLOCK, gfc_vector2d(posX, posY));
        break;
    }

    if (ent) gfc_list_append(enemies, ent);
    return ent;
}