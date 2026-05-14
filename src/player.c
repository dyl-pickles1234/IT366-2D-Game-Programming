#include "simple_logger.h"

#include "gfc_input.h"
#include "gfc_audio.h"

#include "camera.h"
#include "level.h"
#include "mouseInput.h"
#include "ui.h"

#include "bullet.h"
#include "enemy.h"

#include "player.h"

#define PLAYER_SPEED 1

#define CUBE_JUMP_SPEED 5.05
#define SHIP_BOOST_SPEED 0.275
#define UFO_JUMP_SPEED 3.15

#define CUBE_GRAVITY 0.19
#define SHIP_GRAVITY 0.13
#define UFO_GRAVITY 0.1

#define MAX_CHARGE 150
#define PRACTICE_TIMER 150

static Entity* player = NULL;
static PlayerMode playerMode = PLAYER_CUBE;

static Uint8 editorMode = 0;
static ObjectType objectType = OBJECT_TILE;
static int selectedTile = 1;
static LevelObjectType selectedObject = 0;
static EnemyType selectedEnemy = 0;

static int gravityMult = 1;
static Uint8 flipped = 0;
static int charge = 0;
static int shootTimer = 0;

static int practiceMode = 0;
static int practiceTimer = PRACTICE_TIMER;
static GFC_Vector2D practiceCheckpointPos = { 0 };
static int practiceCheckpointGravity = 1;
static Uint8 practiceCheckpointFlipped = 0;
static PlayerMode practiceCheckpointMode = PLAYER_CUBE;

GFC_Sound* die_sfx;
GFC_Sound* win_sfx;
GFC_Sound* coin_sfx;

GFC_HashMap* levelCoins;
Uint8* thisLevelCoins;
int coinsSpent = 0;

GFC_List* upgrades;

Uint8 slowmo = false;

Uint8 hasShield = false;
int iFrames = -1;

Uint8 hat = 0;
Uint8 face = 0;
Uint8 shape = 0;
float hue = 0;
Uint8 bullet = 0;
GFC_TextLine bullet_path = "images/objects/bullet0.png";

void upgrades_load(const char* filepath);

void player_entity_new(GFC_Vector2D pos) {
    Entity* self;
    self = entity_new();

    if (!self) { slog("failed to create entity for player"); return; }

    self->sprite = gf2d_sprite_load_all(
        "images/player/shape0.png",
        64,
        64,
        1,
        false);

    self->pos = pos;
    self->center = gfc_vector2d(32, 32);
    self->scale = gfc_vector2d(0.5, 0.5);
    self->think = player_think;
    self->update = player_update;
    self->draw = player_draw;

    self->speed = PLAYER_SPEED;
    self->hitbox = gfc_rect(pos.x - 16, pos.y - 16, 31, 31);

    player = self;

    die_sfx = gfc_sound_load("audio/sfx/die.wav", 1.0f, 0);
    win_sfx = gfc_sound_load("audio/sfx/victory.wav", 1.0f, 0);
    coin_sfx = gfc_sound_load("audio/sfx/coin.wav", 1.0f, 0);

    levelCoins = gfc_hashmap_new();
    thisLevelCoins = gfc_allocate_array(sizeof(Uint8), 3);

    upgrades = gfc_list_new_size(5);

    upgrades_load("config/upgrades.json");
}

void upgrades_load(const char* filepath) {
    SJson* upgradesConfigFile = sj_load(filepath);
    SJson* upgradesJson = sj_object_get_value(upgradesConfigFile, "upgrades");

    SJson* upgradeJson;
    for (int i = 0; i < sj_array_get_count(upgradesJson); i++) {
        upgradeJson = sj_array_get_nth(upgradesJson, i);

        // upgrade properties
        SJson* nameJson = sj_object_get_value(upgradeJson, "name");
        SJson* costJson = sj_object_get_value(upgradeJson, "cost");

        // actual values
        const char* upgradeName = sj_get_string_value(nameJson);

        int cost;
        sj_get_integer_value(costJson, &cost);

        // create upgrade
        Upgrade* upgrade = gfc_allocate_array(sizeof(Upgrade), 1);
        gfc_word_cpy(upgrade->name, upgradeName);
        upgrade->cost = cost;
        upgrade->purchased = false;

        gfc_list_append(upgrades, upgrade);
    }

    sj_free(upgradesConfigFile);
}

void player_editor_think() {
    if (!player) return;

    Level* level = level_get();

    GFC_Vector2D move = { 0 };

    move.x += player->speed * gfc_input_key_down("d");
    move.x -= player->speed * gfc_input_key_down("a");
    move.y += player->speed * gfc_input_key_down("s");
    move.y -= player->speed * gfc_input_key_down("w");

    player->vel = move;

    // scroll through selected item to be placed
    if (gfc_input_key_pressed("e")) {
        if (objectType == OBJECT_TILE) {
            selectedTile++;
            if (selectedTile > 3) selectedTile = 1;
        }
        else if (objectType == OBJECT_OBJECT) {
            selectedObject++;
            if (selectedObject == OBJECT_OBJECT_END) selectedObject = 0;
        }
        else if (objectType == OBJECT_ENEMY) {
            selectedEnemy++;
            if (selectedEnemy == ENEMY_END) selectedEnemy = 0;
        }
    }

    if (gfc_input_key_pressed("q")) {
        if (objectType == OBJECT_TILE) {
            selectedTile--;
            if (selectedTile < 1) selectedTile = 3;
        }
        else if (objectType == OBJECT_OBJECT) {
            selectedObject--;
            if (selectedObject == -1) selectedObject = OBJECT_OBJECT_END - 1;
        }
        else if (objectType == OBJECT_ENEMY) {
            selectedEnemy--;
            if (selectedEnemy == -1) selectedEnemy = ENEMY_END - 1;
        }
    }

    // handle placing things
    GFC_Vector2D mouseInLevel = mouse_pos_get();
    mouseInLevel.x /= camera_get_zoom().x;
    mouseInLevel.y /= camera_get_zoom().y;
    gfc_vector2d_add(mouseInLevel, mouseInLevel, camera_get_position());

    if (mouse_down(1)) {
        switch (objectType)
        {
        case OBJECT_TILE:
            level->tilemap[level_get_tile_index(level, mouseInLevel.x / 32, mouseInLevel.y / 32)] = selectedTile;
            break;
        case OBJECT_OBJECT:
            // level->tilemap[level_get_tile_index(level, mouseInLevel.x / 32, mouseInLevel.y / 32)] = 2;
            void* data = NULL;
            if (mouse_clicked(1)) {
                if (selectedObject == OBJECT_OBJECT_COIN) {
                    int freeCoin = -1;
                    for (int i = 0; i < 3; i++) {
                        if (!level->coins[i]) {
                            freeCoin = i;
                            break;
                        }
                    }
                    // if all coins are used, cant spawn new ones
                    if (freeCoin == -1) break;
                    data = (void*)freeCoin;
                    level->coins[freeCoin] = 1;
                }
                level_construct_object(selectedObject, mouseInLevel.x, mouseInLevel.y, 0, data);
            }
            break;
        case OBJECT_ENEMY:
            // level->tilemap[level_get_tile_index(level, mouseInLevel.x / 32, mouseInLevel.y / 32)] = 2;
            if (mouse_clicked(1)) level_construct_enemy(selectedEnemy, mouseInLevel.x, mouseInLevel.y, 0);
            break;
        }
    }

    if (mouse_down(3)) {
        switch (objectType)
        {
        case OBJECT_TILE:
            level->tilemap[level_get_tile_index(level, mouseInLevel.x / 32, mouseInLevel.y / 32)] = 0;
            break;
        case OBJECT_OBJECT:
            if (mouse_clicked(3)) {
                GFC_List* objects = level_objects_get();
                Entity* object;
                int entityTest;

                for (int i = 0; i < gfc_list_get_count(objects); i++) {
                    object = gfc_list_get_nth(objects, i);
                    entityTest = gfc_point_in_rect(mouseInLevel, object->hitbox);

                    if (entityTest) {
                        slog("deleted entity %s", object->name);
                        if (gfc_stricmp(object->name, "coin") == 0) {
                            Uint8 index = (Uint8)object->data;
                            level->coins[index] = 0;
                        }
                        gfc_list_delete_nth(objects, i);
                        entity_free(object);
                        break;
                    }
                }
            }
            break;
        case OBJECT_ENEMY:
            if (mouse_clicked(3)) {
                GFC_List* enemies = level_enemies_get();
                Entity* entity;
                int entityTest;

                for (int i = 0; i < gfc_list_get_count(enemies); i++) {
                    entity = gfc_list_get_nth(enemies, i);
                    entityTest = gfc_point_in_rect(mouseInLevel, entity->hitbox);

                    if (entityTest) {
                        slog("deleted entity %s", entity->name);
                        gfc_list_delete_nth(enemies, i);
                        entity_free(entity);
                        break;
                    }
                }
            }
            break;
        }
    }


    if (gfc_input_key_down("LSHIFT")) {
        player->speed = PLAYER_SPEED * 5;
    }
    else {
        player->speed = PLAYER_SPEED;
    }

    if (gfc_input_key_pressed("TAB")) {
        objectType++;
        if (objectType == OBJECT_END) objectType = OBJECT_TILE;
    }

    if (gfc_input_key_down("LCTRL") && gfc_input_key_pressed("s")) {
        level_save(level->filepath);
        slog("saved level");
    }
}

void player_editor_update() {
    if (!player) return;

    gfc_vector2d_add(player->pos, player->pos, player->vel);

    GFC_Vector2D cameraFocus = player->pos;
    // cameraFocus.x += 125;
    // cameraFocus.y += 100 * gravityMult;
    camera_center_on(cameraFocus);
}

void delay(void* udata, Uint8* stream, int len);
void player_think() {
    if (!player) return;

    GFC_Vector2D move = { 0 };

    // movement can only happen with upgrade
    if (((Upgrade*)gfc_list_get_nth(upgrades, UPGRADE_1))->purchased) {
        move.x += player->speed * gfc_input_key_down("d");
        move.x -= player->speed * gfc_input_key_down("a");
    }

    if (flipped) move.x *= -1;

    player->vel.x = level_get()->speed + move.x;

    GFC_Rect groundCheck = player->hitbox;
    groundCheck.y += 1 * gravityMult;
    int testGround = level_test_rect(level_get(), groundCheck);
    if (testGround && testGround != 3) {
        player->onGround = 1;
    }
    else {
        player->onGround = 0;
    }

    if (gfc_input_key_pressed("p")) {
        practiceMode = practiceMode ? 0 : 1;
    }

    if (gfc_input_key_pressed("TAB") && player_owns_upgrade(UPGRADE_4) && charge > 0) {
        slowmo = true;
        Mix_SetPostMix(delay, NULL);
    }

    if (practiceMode && player->onGround && practiceTimer <= 0) {
        practiceCheckpointPos = player->pos;
        practiceCheckpointGravity = gravityMult;
        practiceCheckpointFlipped = flipped;
        practiceCheckpointMode = playerMode;
        practiceTimer = PRACTICE_TIMER;
    }

    GFC_Vector2D mouse = mouse_pos_get();
    GFC_Vector2D bulletPos = gfc_vector2d(player->pos.x, player->pos.y);
    GFC_Vector2D playerScreenPos;
    GFC_Vector2D playerToMouse;

    gfc_vector2d_add(playerScreenPos, player->pos, camera_get_offset());
    playerScreenPos = gfc_vector2d_multiply(playerScreenPos, camera_get_zoom());

    gfc_vector2d_sub(playerToMouse, mouse, playerScreenPos);
    gfc_vector2d_normalize(&playerToMouse);

    if (shootTimer > 0) shootTimer--;

    switch (playerMode)
    {
    case PLAYER_CUBE:
        player->sprite = gf2d_sprite_load_all(
            "images/player/cube.png",
            32,
            32,
            1,
            false);

        if (gfc_input_key_down(" ") && player->onGround) {
            player->vel.y = -CUBE_JUMP_SPEED * gravityMult;
            // slog("jumped at tile %i %i", (int)(player->pos.x / 32), (int)(player->pos.y / 32));
        }

        // left click shoot
        if (mouse_clicked(1)) {
            GFC_Vector2D bulletVel = playerToMouse;

            // set speed
            gfc_vector2d_scale(bulletVel, bulletVel, 8);
            // gfc_vector2d_add(bulletVel, bulletVel, player->vel);

            bullet_entity_new(bullet_path, bulletPos, 16, bulletVel, 0, -1);
        }

        // right click shoot
        if (charge >= MAX_CHARGE && mouse_clicked(3)) {
            GFC_Vector2D bulletVel = playerToMouse;

            for (int i = 0; i < 5; i++) {
                GFC_Vector2D bulletVelRand;

                // set speed
                gfc_vector2d_scale(bulletVelRand, bulletVel, gfc_random() + 5);

                bulletVelRand = gfc_vector2d_rotate(bulletVelRand, gfc_random() * 0.5 - 0.25);
                bullet_entity_new(bullet_path, bulletPos, 8, bulletVelRand, 0, -1);
            }

            charge = -1;
        }

        break;
    case PLAYER_SHIP:
        player->sprite = gf2d_sprite_load_all(
            "images/player/ship.png",
            32,
            32,
            1,
            false);

        if (gfc_input_key_down(" ")) {
            player->vel.y += -SHIP_BOOST_SPEED * gravityMult;
        }

        // left click shoot
        if (mouse_down(1) && shootTimer <= 0) {
            GFC_Vector2D bulletVel = playerToMouse;

            // set speed
            gfc_vector2d_scale(bulletVel, bulletVel, 3);
            gfc_vector2d_add(bulletVel, bulletVel, player->vel);

            bullet_entity_new(bullet_path, bulletPos, 8, bulletVel, 0, -1);
            shootTimer = 15;
        }

        // right click shoot
        if (charge >= 0 && mouse_down(3)) {
            if (shootTimer <= 0) {
                GFC_Vector2D bulletVel = playerToMouse;

                for (int i = 0; i < 5; i++) {
                    GFC_Vector2D bulletVelRand;

                    // set speed
                    gfc_vector2d_scale(bulletVelRand, bulletVel, gfc_random() + 2);

                    bulletVelRand = gfc_vector2d_rotate(bulletVelRand, gfc_random() * 0.5 - 0.25);
                    bullet_entity_new(bullet_path, bulletPos, 8, bulletVelRand, 0, -1);
                }
                shootTimer = 15;
            }
            charge -= 2;
        }

        if (charge == -1 && mouse_down(3) && shootTimer <= 0) charge = -100;

        break;
    case PLAYER_BALL:
        player->sprite = gf2d_sprite_load_all(
            "images/player/ball.png",
            32,
            32,
            1,
            false);

        if (gfc_input_key_pressed(" ") && player->onGround) {
            // player->vel.y = -CUBE_JUMP_SPEED * gravityMult;
            player->vel.y = 0;
            gravityMult *= -1;
        }

        // left click shoot
        if (mouse_clicked(1) && shootTimer <= 0) {
            GFC_Vector2D bulletVel = playerToMouse;

            // set speed
            gfc_vector2d_scale(bulletVel, bulletVel, 1);
            gfc_vector2d_add(bulletVel, bulletVel, player->vel);

            bullet_entity_new(bullet_path, bulletPos, 64, bulletVel, 0, -1);
            shootTimer = 120;
        }

        // right click shoot
        if (charge >= MAX_CHARGE && mouse_clicked(3)) {
            bullet_entity_new(bullet_path, bulletPos, 256, gfc_vector2d(0, 0), 0, 2);
            charge = -1;
        }

        break;
    case PLAYER_WAVE:
        player->sprite = gf2d_sprite_load_all(
            "images/player/wave.png",
            32,
            32,
            1,
            false);

        if (gfc_input_key_down(" ")) {
            // player->vel.y = -CUBE_JUMP_SPEED * gravityMult;
            player->vel.y = -level_get()->speed * gravityMult;
        }
        else {
            player->vel.y = level_get()->speed * gravityMult;
        }

        // left click shoot
        if (mouse_clicked(1)) {
            GFC_List* enemies = level_enemies_get();
            Entity* enemy;
            GFC_Vector2D enemyScreenPos;
            GFC_Rect hitboxScreen;
            int enemyTest;

            for (int i = 0; i < gfc_list_get_count(enemies); i++) {
                enemy = gfc_list_get_nth(enemies, i);

                gfc_vector2d_add(enemyScreenPos, enemy->pos, camera_get_offset());
                enemyScreenPos = gfc_vector2d_multiply(enemyScreenPos, camera_get_zoom());

                hitboxScreen.w = enemy->hitbox.w * camera_get_zoom().x;
                hitboxScreen.h = enemy->hitbox.h * camera_get_zoom().y;
                hitboxScreen.x = enemyScreenPos.x - hitboxScreen.w / 2;
                hitboxScreen.y = enemyScreenPos.y - hitboxScreen.h / 2;

                enemyTest = gfc_point_in_rect(mouse, hitboxScreen);

                if (enemyTest) {
                    slog("zapped his ass");
                    gfc_list_delete_nth(enemies, i);
                    entity_free(enemy);
                    break;
                }
            }
        }

        // right click shoot
        if (charge >= MAX_CHARGE && mouse_clicked(3)) {
            GFC_List* enemies = level_enemies_get();
            Entity* enemy;
            GFC_Vector2D enemyScreenPos;

            for (int i = 0; i < gfc_list_get_count(enemies); i++) {
                enemy = gfc_list_get_nth(enemies, i);

                gfc_vector2d_add(enemyScreenPos, enemy->pos, camera_get_offset());
                enemyScreenPos = gfc_vector2d_multiply(enemyScreenPos, camera_get_zoom());

                if (gfc_point_in_rect(enemyScreenPos, gfc_rect(0, 0, 1200, 768))) {
                    slog("zapped his ass");
                    gfc_list_delete_nth(enemies, i);
                    entity_free(enemy);
                    break;
                }
            }
            charge = -1;
        }

        break;
    case PLAYER_UFO:
        player->sprite = gf2d_sprite_load_all(
            "images/player/ufo.png",
            32,
            32,
            1,
            false);

        if (gfc_input_key_pressed(" ")) {
            player->vel.y = -UFO_JUMP_SPEED * gravityMult;
        }

        // left click shoot
        if (mouse_clicked(1) && shootTimer <= 0) {
            GFC_Vector2D bulletVel = playerToMouse;

            // set speed
            gfc_vector2d_scale(bulletVel, bulletVel, 3);
            gfc_vector2d_add(bulletVel, bulletVel, player->vel);

            bullet_entity_new(bullet_path, bulletPos, 32, bulletVel, 1, -1);
            shootTimer = 120;
        }

        // right click shoot
        if (charge >= MAX_CHARGE && mouse_clicked(3)) {
            GFC_Vector2D bulletVel = playerToMouse;

            // set speed
            gfc_vector2d_scale(bulletVel, bulletVel, 2);

            for (int i = 0; i < 360; i += 10) {
                bulletVel = gfc_vector2d_rotate(bulletVel, 10 * GFC_DEGTORAD);
                GFC_Vector2D thisVel = bulletVel;
                gfc_vector2d_add(thisVel, thisVel, player->vel);
                bullet_entity_new(bullet_path, bulletPos, 8, thisVel, 0, -1);
            }

            charge = -1;
        }

        break;
    }

    if (slowmo) {
        if (charge > 0) charge--;
        else {
            slowmo = false;
            Mix_SetPostMix(NULL, NULL);
        }
    }
    else if (charge < MAX_CHARGE) charge += player_owns_upgrade(UPGRADE_2) ? 2 : 1;

    if (iFrames > 0) iFrames--;

    if (iFrames == 0 && hasShield) hasShield = false;

    if (practiceTimer > 0) practiceTimer--;
}

void player_update() {
    if (!player) return;

    // slog("player - pos: %f %f   vel: %f %f", player->pos.x, player->pos.y, player->vel.x, player->vel.y);

    // do collision checks for permission
    GFC_Rect levelVCheck = player->hitbox;
    levelVCheck.y += player->vel.y;

    int testV = level_test_rect(level_get(), levelVCheck);

    if (testV) {
        if (testV == 3 || (playerMode == PLAYER_CUBE && player->vel.y * gravityMult < 0) || playerMode == PLAYER_WAVE) { // hit spike, cube jumped into ceiling, or wave
            player_reset();
        }
        player->vel.y = 0;
        player->pos.y = (int)(player->pos.y / 32) * 32 + 16.001;

        if (gravityMult < 0) player->pos.y += 0.001;
    }

    // update player hitbox
    player->hitbox.x = player->pos.x - 16;
    player->hitbox.y = player->pos.y - 16;

    GFC_Rect levelHCheck = player->hitbox;
    levelHCheck.x += player->vel.x;

    int testH = level_test_rect(level_get(), levelHCheck);

    if (testH) {
        player_reset();
    }

    gfc_vector2d_add(player->pos, player->pos, player->vel);

    // update player hitbox
    player->hitbox.x = player->pos.x - 16;
    player->hitbox.y = player->pos.y - 16;

    if (player->vel.x || player->vel.y) player->rotation = gfc_vector2d_angle(player->vel) * GFC_RADTODEG - 90;

    // apply gravity
    float gravity;
    switch (playerMode)
    {
    case PLAYER_SHIP:
        gravity = SHIP_GRAVITY;
        break;
    case PLAYER_UFO:
        gravity = UFO_GRAVITY;
        break;
    default:
        gravity = CUBE_GRAVITY;
    }

    if (!player->onGround) player->vel.y += gravity * gravityMult;

    // cap speed if necessary
    if (playerMode == PLAYER_SHIP) {
        // cap going down
        if (player->vel.y * gravityMult >= level_get()->speed) {
            player->vel.y = gravityMult * level_get()->speed;
        }
        // cap going up
        else if (player->vel.y * gravityMult <= -level_get()->speed * 1.5) {
            player->vel.y = gravityMult * -level_get()->speed * 1.5;
        }
    }
    else if (playerMode == PLAYER_UFO && player->vel.y * gravityMult >= level_get()->speed) {
        player->vel.y = gravityMult * level_get()->speed;
    }


    GFC_Vector2D cameraFocus = player->pos;
    // cameraFocus.x += 125;
    // cameraFocus.y += 100 * gravityMult;
    if (playerMode != PLAYER_CUBE) {
        // cameraFocus.y = camera_get_center().y;
    }

    camera_center_on(cameraFocus);

    if (player->pos.x >= (level_get()->width - 3) * 32) {
        slog("You Win!");
        gfc_sound_play(win_sfx, 0, 0.25f, -1);
        SDL_Delay(1000);
        Uint8* savedCoins = gfc_hashmap_get(levelCoins, level_get()->filepath);
        savedCoins[0] = thisLevelCoins[0];
        savedCoins[1] = thisLevelCoins[1];
        savedCoins[2] = thisLevelCoins[2];
        level_free(level_get());
        level_set(NULL);
    }
    if (playerMode == PLAYER_CUBE) {
        GFC_TextLine shape_path;
        snprintf(shape_path, GFCLINELEN, "images/player/shape%i.png", shape);
        Sprite* shape_sprite;
        player->sprite = gf2d_sprite_load_all(
            shape_path,
            64,
            64,
            1,
            false);
        player->center = gfc_vector2d(32, 32);
        player->scale = gfc_vector2d(0.5, 0.5);
    }
    else {
        player->center = gfc_vector2d(16, 16);
        player->scale = gfc_vector2d(1, 1);
    }
}

Entity* player_get() {
    return player;
}

int player_gravity_get() {
    return gravityMult;
}

void player_gravity_set(int newGravity) {
    gravityMult = newGravity;
}

void player_reset_no_sound() {
    player->vel.x = 0;
    player->vel.y = 0;
    if (practiceMode) {
        player->pos = practiceCheckpointPos;
        gravityMult = practiceCheckpointGravity;
        flipped = practiceCheckpointFlipped;
        playerMode = practiceCheckpointMode;
    }
    else {
        player->pos.x = 100;
        player->pos.y = level_get()->height * 32 - 32.1f;
        player->onGround = true;
        gravityMult = 1;
        flipped = 0;
        playerMode = PLAYER_CUBE;
    }
    player->hitbox.x = player->pos.x - 16;
    player->hitbox.y = player->pos.y - 16;

    charge = 0;
    slowmo = false;
    Mix_SetPostMix(NULL, NULL);
    if (player_owns_upgrade(UPGRADE_3)) hasShield = true; else hasShield = false;
    iFrames = -1;

    player_editor_mode_set(0);

    Uint8* saved_coins = gfc_hashmap_get(levelCoins, level_get()->filepath);
    thisLevelCoins[0] = saved_coins[0];
    thisLevelCoins[1] = saved_coins[1];
    thisLevelCoins[2] = saved_coins[2];

    GFC_TextLine path;
    gfc_line_cpy(path, level_get()->filepath);
    level_free(level_get());
    level_set(level_load(path));

    // slog("coins: %i", player_get_coin_count());

    // slog("player reset");
    gfc_sound_play(level_get()->song, 0, 0.1f, -1);
}

void player_reset() {
    Mix_SetPostMix(NULL, NULL);
    Mix_HaltChannel(-1);
    gfc_sound_play(die_sfx, 0, 0.25f, -1);
    SDL_Delay(500);
    player_reset_no_sound();
}

void player_mode_set(PlayerMode mode) {
    playerMode = mode;
}

PlayerMode player_mode_get() {
    return playerMode;
}

Uint8 player_flipped_get() {
    return flipped;
}

void player_flipped_set(Uint8 flip) {
    flipped = flip;
}

float player_charge_get() {
    return (float)charge / MAX_CHARGE;
}

void player_editor_draw(Entity* player) {
    if (!player) return;

    player_draw(player);

    int size = 32;

    if (objectType == OBJECT_OBJECT && selectedObject >= OBJECT_OBJECT_PORTAL_CUBE) size = 64;

    GFC_Color alpha = gfc_color(1, 1, 1, 0.5);

    GFC_Vector2D pos = mouse_pos_get();
    GFC_Vector2D scale = camera_get_zoom();

    // project to tiles
    pos.x /= scale.x;
    pos.y /= scale.y;

    gfc_vector2d_add(pos, pos, camera_get_position());

    if (objectType == OBJECT_TILE) {
        pos.x = (int)(pos.x / 32);
        pos.y = (int)(pos.y / 32);
    }
    else {
        pos.x = (pos.x / size) - 0.5;
        pos.y = (pos.y / size) - 0.5;
    }

    // project back to screen
    pos.x *= size;
    pos.y *= size;

    gfc_vector2d_sub(pos, pos, camera_get_position());

    pos.x *= scale.x;
    pos.y *= scale.y;

    Sprite* sprite;

    switch (objectType)
    {
    case OBJECT_TILE:
        sprite = gf2d_sprite_load_all(
            "images/tiles/geometry_dash.png",
            32,
            32,
            1,
            false);

        text_draw_raw("Palette:", 32, 10, 700, GFC_COLOR_WHITE);
        break;
    case OBJECT_OBJECT:
        GFC_TextLine filename;

        if (selectedObject == OBJECT_OBJECT_ORB_NORMAL) {
            strcpy(filename, "images/objects/orb_normal.png");
        }
        else if (selectedObject == OBJECT_OBJECT_ORB_SMALL) {
            strcpy(filename, "images/objects/orb_small.png");
        }
        else if (selectedObject == OBJECT_OBJECT_ORB_GRAVITY) {
            strcpy(filename, "images/objects/orb_gravity.png");
        }
        else if (selectedObject == OBJECT_OBJECT_PAD_NORMAL) {
            strcpy(filename, "images/objects/pad_normal.png");
        }
        else if (selectedObject == OBJECT_OBJECT_PAD_SMALL) {
            strcpy(filename, "images/objects/pad_small.png");
        }
        else if (selectedObject == OBJECT_OBJECT_PAD_GRAVITY) {
            strcpy(filename, "images/objects/pad_gravity.png");
        }
        else if (selectedObject == OBJECT_OBJECT_COIN) {
            strcpy(filename, "images/objects/coin.png");
        }
        else if (selectedObject == OBJECT_OBJECT_PORTAL_CUBE) {
            strcpy(filename, "images/objects/portal_cube.png");
        }
        else if (selectedObject == OBJECT_OBJECT_PORTAL_SHIP) {
            strcpy(filename, "images/objects/portal_ship.png");
        }
        else if (selectedObject == OBJECT_OBJECT_PORTAL_BALL) {
            strcpy(filename, "images/objects/portal_ball.png");
        }
        else if (selectedObject == OBJECT_OBJECT_PORTAL_WAVE) {
            strcpy(filename, "images/objects/portal_wave.png");
        }
        else if (selectedObject == OBJECT_OBJECT_PORTAL_UFO) {
            strcpy(filename, "images/objects/portal_ufo.png");
        }
        else if (selectedObject == OBJECT_OBJECT_PORTAL_GRAVITY_UP) {
            strcpy(filename, "images/objects/portal_gravity_up.png");
        }
        else if (selectedObject == OBJECT_OBJECT_PORTAL_GRAVITY_DOWN) {
            strcpy(filename, "images/objects/portal_gravity_down.png");
        }
        else if (selectedObject == OBJECT_OBJECT_PORTAL_FLIP_FLIPPED) {
            strcpy(filename, "images/objects/portal_flip_flipped.png");
        }
        else if (selectedObject == OBJECT_OBJECT_PORTAL_FLIP_NORMAL) {
            strcpy(filename, "images/objects/portal_flip_normal.png");
        }

        sprite = gf2d_sprite_load_all(
            filename,
            selectedObject >= OBJECT_OBJECT_PORTAL_CUBE ? 64 : 32,
            selectedObject >= OBJECT_OBJECT_PORTAL_CUBE ? 64 : 32,
            1,
            false);

        text_draw_raw("Selected object:", 32, 10, 700, GFC_COLOR_WHITE);
        break;
    case OBJECT_ENEMY:
        GFC_TextLine e_filename;

        if (selectedEnemy == ENEMY_SAW) {
            strcpy(e_filename, "images/enemies/saw.png");
        }
        else if (selectedEnemy == ENEMY_BLOCK) {
            strcpy(e_filename, "images/enemies/block.png");
            scale.x *= 0.5;
            scale.y *= 0.5;
            pos.x += 16;
            pos.y += 16;
        }

        sprite = gf2d_sprite_load_all(
            e_filename,
            32,
            32,
            1,
            false);

        text_draw_raw("Selected enemy:", 32, 10, 700, GFC_COLOR_WHITE);
        break;
    }

    gf2d_sprite_draw(
        sprite,
        pos,
        &scale,
        NULL,
        NULL,
        NULL,
        &alpha,
        objectType == OBJECT_TILE ? selectedTile - 1 : 0);

    // display name of whatever is selected (this is a small memory leak. But i dont care)
    char* tok = strtok(SDL_strdup(sprite->filepath), "/");
    char* saved = tok;
    while (tok != NULL) {
        saved = tok;
        tok = strtok(NULL, "/");
    }

    text_draw_raw(strtok(SDL_strdup(saved), "."), 24, 10, 732, GFC_COLOR_WHITE);
}

void player_draw(Entity* player) {
    if (!player) return;

    if (practiceMode) {
        GFC_Vector2D pos;
        GFC_Vector2D scale = camera_get_zoom();

        gfc_vector2d_add(pos, practiceCheckpointPos, camera_get_offset());
        pos = gfc_vector2d_multiply(pos, scale);

        scale.x *= 0.5;
        scale.y *= 0.5;

        Sprite* sprite;

        sprite = gf2d_sprite_load_all(
            "images/player/wave2.png",
            32,
            32,
            1,
            false);

        GFC_Vector2D center = { 16, 16 };

        gf2d_sprite_draw(
            sprite,
            pos,
            &scale,
            &center,
            NULL,
            NULL,
            NULL,
            0);
    }

    GFC_Vector2D flip = gfc_vector2d(0, gravityMult < 0);

    // draw hat on player
    GFC_Vector2D hat_pos;
    GFC_Vector2D hat_scale = camera_get_zoom();

    gfc_vector2d_add(hat_pos, player->pos, camera_get_offset());
    hat_pos.y -= 16 * (gravityMult < 0 ? -1 : 1);
    hat_pos.y -= abs(player->rotation) / 6 * (gravityMult < 0 ? -1 : 1);
    hat_pos = gfc_vector2d_multiply(hat_pos, hat_scale);

    GFC_TextLine hat_path;
    snprintf(hat_path, GFCLINELEN, "images/player/hat%i.png", hat);
    Sprite* hat_sprite;
    hat_sprite = gf2d_sprite_load_all(
        hat_path,
        64,
        64,
        1,
        false);

    GFC_Vector2D hat_center = { 32, gravityMult < 0 ? 0 : 64 };
    float hat_rotation = player->rotation / 3;

    hat_scale.x *= 0.35;
    hat_scale.y *= 0.35;

    gf2d_sprite_draw(
        hat_sprite,
        hat_pos,
        &hat_scale,
        &hat_center,
        &hat_rotation,
        &flip,
        NULL,
        0);

    if (playerMode == PLAYER_CUBE || playerMode == PLAYER_BALL) {
        // draw face on player
        GFC_Vector2D face_pos;
        GFC_Vector2D face_scale = camera_get_zoom();

        gfc_vector2d_add(face_pos, player->pos, camera_get_offset());
        face_pos = gfc_vector2d_multiply(face_pos, face_scale);

        GFC_TextLine face_path;
        snprintf(face_path, GFCLINELEN, "images/player/face%i.png", face);
        Sprite* face_sprite;
        face_sprite = gf2d_sprite_load_all(
            face_path,
            64,
            64,
            1,
            false);

        GFC_Vector2D face_center = { 32, 24 };
        float face_rotation = player->rotation;

        face_scale.x *= 0.35;
        face_scale.y *= 0.35;

        gf2d_sprite_draw(
            face_sprite,
            face_pos,
            &face_scale,
            &face_center,
            &face_rotation,
            &flip,
            NULL,
            0);
    }
}

Uint8 player_editor_mode_get() {
    return editorMode;
}

void player_editor_mode_set(Uint8 editor) {
    // slog("switching editor mode - %i", editor);
    editorMode = editor;
    if (editor) {
        slowmo = false;
        player->think = player_editor_think;
        player->update = player_editor_update;
        player->draw = player_editor_draw;
    }
    else {
        player->speed = PLAYER_SPEED;
        player->think = player_think;
        player->update = player_update;
        player->draw = player_draw;
    }
}

void player_add_coin(Uint8 index) {
    gfc_sound_play(coin_sfx, 0, 0.25f, 1);
    thisLevelCoins[index] = 1;
    // slog("collected coins in this level: [%i %i %i]", thisLevelCoins[0], thisLevelCoins[1], thisLevelCoins[2]);
}

void player_add_debug_coin() {
    coinsSpent--;
}

Uint32 player_get_coin_count() {
    int coinCount = 0;

    GFC_HashElement* item;
    GFC_List* items = gfc_hashmap_get_all_values(levelCoins);
    for (int i = 0; i < gfc_list_get_count(items); i++) {
        item = gfc_list_get_nth(items, i);
        if (((Uint8*)(item->data))[0]) coinCount++;
        if (((Uint8*)(item->data))[1]) coinCount++;
        if (((Uint8*)(item->data))[2]) coinCount++;
    }
    gfc_list_delete(items);

    return coinCount - coinsSpent;
}

GFC_HashMap* player_get_level_coins() {
    return levelCoins;
}

Uint8 player_owns_upgrade(UpgradeType upgrade) {
    return ((Upgrade*)gfc_list_get_nth(upgrades, upgrade))->purchased;
}

void player_buy_upgrade(UpgradeType upgrade) {
    Upgrade* upgradeObj = gfc_list_get_nth(upgrades, upgrade);
    upgradeObj->purchased = 1;
    coinsSpent += upgradeObj->cost;
    slog("Coins remaining: %i", player_get_coin_count());
}

Uint8 player_get_upgrade_cost(UpgradeType upgrade) {
    return ((Upgrade*)gfc_list_get_nth(upgrades, upgrade))->cost;
}

void player_get_upgrade_name(UpgradeType upgrade, char* textOut) {
    gfc_word_cpy(textOut, ((Upgrade*)gfc_list_get_nth(upgrades, upgrade))->name);
}

Uint8 player_get_slowmo() {
    return slowmo;
}

Uint8 player_get_shield() {
    return hasShield;
}

void player_break_shield() {
    if (iFrames < 0) iFrames = 100;
}

GFC_Vector4D player_get_customization() {
    return gfc_vector4d(hat, face, shape, hue);
}

void player_set_customization(Uint8 h, Uint8 f, Uint8 s, float hu) {
    hat = h;
    face = f;
    shape = s;
    hue = hu;
    player_get()->hue = hu;
}

Uint8 player_get_bullet() {
    return bullet;
}

void player_set_bullet(Uint8 b) {
    bullet = b;
    snprintf(bullet_path, GFCLINELEN, "images/objects/bullet%i.png", b);
}