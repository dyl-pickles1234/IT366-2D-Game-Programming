#include "simple_logger.h"
#include "simple_json.h"

#include "camera.h"
#include "level.h"

static Level* theLevel = NULL;

Level* level_new() {
    Level* level;

    level = gfc_allocate_array(sizeof(Level), 1);
    if (!level) return NULL;

    return level;
}

void level_free(Level* level) {
    if (!level) return;

    gf2d_sprite_free(level->bg);
    gf2d_sprite_free(level->tileset);

    if (level->tilemap) free(level->tilemap);

    free(level);
}

// Level* level_create(const char* bg, const char* tileset, Uint32 tileWidth, Uint32 tileHeight, Uint32 tilesPerLine, Uint32 width, Uint32 height) {
//     Level* level = level_new();
//     if (!level) return NULL;
//     if (!width || !height) { slog("Can't make a level without tilemap dimensions"); return NULL; }

//     if (bg) {
//         level->bg = gf2d_sprite_load_image(bg);
//     }

//     if (tileset) {
//         level->tileset = gf2d_sprite_load_all(
//             tileset,
//             tileWidth,
//             tileHeight,
//             tilesPerLine,
//             true);
//     }

//     level->tilemap = gfc_allocate_array(sizeof(Uint8), width * height);
//     level->width = width;
//     level->height = height;
//     level->tileWidth = tileWidth;
//     level->tileHeight = tileHeight;

//     camera_set_bounds(gfc_rect(0, 0, level->tileWidth * level->width, level->tileHeight * level->height));

//     return level;
// }

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

    // pull out the actual values from JSON
    const char* bgFilename = sj_get_string_value(bgJson);
    const char* tilesheetFilename = sj_get_string_value(tilesheetJson);
    int width, height, tileWidth, tileHeight, tilesPerRow;
    float speed;

    sj_get_integer_value(tileWidthJson, &tileWidth);
    sj_get_integer_value(tileHeightJson, &tileHeight);
    sj_get_integer_value(tilesPerRowJson, &tilesPerRow);
    sj_get_float_value(speedJson, &speed);

    width = sj_array_get_count(tilemapFirstRowJson);
    height = sj_array_get_count(tilemapJson);

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

    camera_set_bounds(gfc_rect(0, 0, level->tileWidth * level->width, level->tileHeight * level->height));

    sj_free(levelConfigFile);

    return level;
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