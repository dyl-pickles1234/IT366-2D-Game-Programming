#include "simple_logger.h"

#include "camera.h"
#include "level.h"

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

Level* level_create(const char* bg, const char* tileset, Uint32 tileWidth, Uint32 tileHeight, Uint32 tilesPerLine, Uint32 width, Uint32 height) {
    Level* level = level_new();
    if (!level) return NULL;
    if (!width || !height) { slog("Can't make a level without tilemap dimensions"); return NULL; }

    if (bg) {
        level->bg = gf2d_sprite_load_image(bg);
    }

    if (tileset) {
        level->tileset = gf2d_sprite_load_all(
            tileset,
            tileWidth,
            tileHeight,
            tilesPerLine,
            true);
    }

    level->tilemap = gfc_allocate_array(sizeof(Uint8), width * height);
    level->width = width;
    level->height = height;
    level->tileWidth = tileWidth;
    level->tileHeight = tileHeight;
    return level;
}

int level_get_tile_index(Level* level, Uint32 x, Uint32 y) {
    if (!level || !level->tilemap) return -1;
    if (x >= level->width) return -1;
    if (y >= level->height) return -1;

    return y * level->width + x;
}

void level_add_border(Level* level, Uint8 tile) {
    if (!level || !level->tilemap) return;

    int index;

    for (int j = 0; j < level->height; j++) {
        index = level_get_tile_index(level, 0, j);
        if (index >= 0) level->tilemap[index] = tile;
        index = level_get_tile_index(level, level->width - 1, j);
        if (index >= 0) level->tilemap[index] = tile;
    }

    for (int i = 0; i < level->height; i++) {
        index = level_get_tile_index(level, 0, i);
        if (index >= 0) level->tilemap[index] = tile;
        index = level_get_tile_index(level, level->width - 1, i);
        if (index >= 0) level->tilemap[index] = tile;
    }
}

void level_draw(Level* level) {
    if (!level) return;

    if (level->bg) {
        gf2d_sprite_draw_image(level->bg, gfc_vector2d(0, 0));
    }

    if (level->tileset) {
        int index;
        Uint8 tile;
        for (int j = 0; j < level->height; j++) {
            for (int i = 0; i < level->width; i++) {
                index = level_get_tile_index(level, i, j);
                if (index < 0) continue;
                tile = level->tilemap[index];
                if (!tile) continue;
                GFC_Vector2D pos = gfc_vector2d(i * level->tileWidth, j * level->tileHeight);
                gfc_vector2d_add(pos, pos, camera_get_offset());
                gf2d_sprite_draw(
                    level->tileset,
                    pos,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    tile - 1);
            }
        }
    }
}