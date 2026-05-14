#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "entity.h"
#include "gfc_hashmap.h"

typedef enum {
    PLAYER_CUBE = 0,
    PLAYER_SHIP,
    PLAYER_BALL,
    PLAYER_WAVE,
    PLAYER_UFO
} PlayerMode;

// for editor mode
typedef enum {
    OBJECT_TILE = 0,
    OBJECT_OBJECT,
    OBJECT_ENEMY,
    OBJECT_END
} ObjectType;

typedef enum {
    UPGRADE_1 = 0,
    UPGRADE_2,
    UPGRADE_3,
    UPGRADE_4,
    UPGRADE_5
} UpgradeType;

typedef struct {
    GFC_TextWord name;
    int cost;
    Uint8 purchased;
} Upgrade;

void player_entity_new(GFC_Vector2D pos);
void player_think();
void player_update();
void player_draw();

Entity* player_get();

int player_gravity_get();
void player_gravity_set(int newGravity);

PlayerMode player_mode_get();
void player_mode_set(PlayerMode mode);

Uint8 player_flipped_get();
void player_flipped_set(Uint8 flipped);

float player_charge_get();

Uint8 player_editor_mode_get();
void player_editor_mode_set(Uint8 editorMode);

void player_reset_no_sound();
void player_reset();

void player_add_coin(Uint8 index);
Uint32 player_get_coin_count();
GFC_HashMap* player_get_level_coins();

Uint8 player_owns_upgrade(UpgradeType upgrade);
void player_buy_upgrade(UpgradeType upgrade);
Uint8 player_get_upgrade_cost(UpgradeType upgrade);
void player_get_upgrade_name(UpgradeType upgrade, char* textOut);

Uint8 player_get_slowmo();
Uint8 player_get_shield();
void player_break_shield();

GFC_Vector4D player_get_customization();
void player_set_customization(Uint8 h, Uint8 f, Uint8 s, float hu);

Uint8 player_get_bullet();
void player_set_bullet(Uint8 b);

#endif