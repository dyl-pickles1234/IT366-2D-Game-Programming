#include <SDL.h>
#include <dirent.h>

#include "simple_logger.h"

#include "gfc_input.h"
#include "gfc_audio.h"

#include "gf2d_draw.h"
#include "gf2d_graphics.h"
#include "gf2d_sprite.h"

#include "camera.h"
#include "entity.h"
#include "monster.h"
#include "player.h"

#include "mouseInput.h"

#include "level.h"

#include "ui.h"

#include "audio.h"

#define GAME_TITLE "Game Game Game"
#define SCREEN_X 1200
#define SCREEN_Y 768

int main(int argc, char* argv[])
{
    /*variable declarations*/
    Uint8 beat_visualization = false;

    int done = 0;
    int paused = 1;
    const Uint8* keys;

    char FPS_string[8];

    float mf = 0;
    Sprite* mouse;
    GFC_Color mouseGFC_Color = gfc_color8(255, 100, 255, 200);
    GFC_Color chargeGFC_Color = gfc_color8(0, 255, 255, 200);

    /*program initializtion*/
    init_logger("gf2d.log", 0);
    slog("---==== BEGIN ====---");
    gf2d_graphics_initialize(
        "gf2d",
        SCREEN_X,
        SCREEN_Y,
        SCREEN_X,
        SCREEN_Y,
        gfc_vector4d(0, 0, 0, 255),
        0);
    gfc_input_init("config/input.cfg");
    gf2d_graphics_set_frame_delay(16);
    gf2d_sprite_init(1024);
    entity_manager_init(1024);
    SDL_ShowCursor(SDL_DISABLE);
    ui_init();

    // gotta do some wacky stuff to get audio right
    gfc_sound_init_config("config/audio.cfg");
    Mix_CloseAudio();
    if (Mix_OpenAudioDevice(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 64, NULL, 0) == -1)
    {
        slog("Failed to open audio: %s\n", SDL_GetError());
    }
    int freq;
    Uint16 fmt;
    int chann;
    Mix_QuerySpec(&freq, &fmt, &chann);
    slog("Mix Query: %i %X %i", freq, fmt, chann);
    // done wth wacky audio stuff

    GFC_List* beats = NULL;
    if (beat_visualization) {
        // GFC_Sound* song = gfc_sound_load("audio/music/blast.wav", 1.0, 0);
        GFC_Sound* song = gfc_sound_load("audio/music/miku.wav", 1.0, 0);
        // GFC_Sound* song = gfc_sound_load("audio/music/pig.wav", 1.0, 0);
        // slog("%i", SDL_GetTicks());
        beats = get_beats(song);
        // slog("%i", SDL_GetTicks());
        gfc_sound_play(song, 0, 0.1f, -1);
    }

    camera_set_dimension(gfc_vector2d(SCREEN_X, SCREEN_Y));
    camera_set_zoom(2);
    // camera_set_zoom(1);

    /*demo setup*/
    mouse = gf2d_sprite_load_all("images/pointer.png", 32, 32, 16, 0);

    // Level* level = level_create(
    //     "images/backgrounds/bg_flat.png",
    //     "images/tiles/tileset_flat.png",
    //     32,
    //     32,
    //     1,
    //     48,
    //     24
    // );

    // Level* level = level_load("levels/saved.json");
    // Level* level = level_load("levels/CantLetGo.json");

    // if (!level) { slog("bad level"); return 1; }
    // level_set(level);

    player_entity_new(gfc_vector2d(100, 464));
    // player_mode_set(PLAYER_UFO);
    // player_editor_mode_set(1);

    // monster_new(gfc_vector2d(200, 250));

    // sprite setup
    Sprite* chargeSprite = gf2d_sprite_load_all(
        "images/ui/charge.png",
        32,
        32,
        1,
        false);
    Sprite* pauseSprite = gf2d_sprite_load_all(
        "images/ui/pause.png",
        100,
        100,
        1,
        false);
    Sprite* playSprite = gf2d_sprite_load_all(
        "images/ui/play.png",
        100,
        100,
        1,
        false);

    // UI setup
    window_load_all("config/windows.json");
    GFC_TextWord upgradeButtonName;
    GFC_TextLine costLabelName;
    for (int i = 0; i < 5; i++) {
        snprintf(upgradeButtonName, GFCLINELEN, "upgrade_button%i", i + 1);
        player_get_upgrade_name(i, button_find(upgradeButtonName, window_get("shop")->UIElements)->label->text);

        snprintf(costLabelName, GFCLINELEN, "cost_text%i", i + 1);
        snprintf(text_find(costLabelName, window_get("shop")->UIElements)->text, GFCLINELEN, "Cost: %i", player_get_upgrade_cost(i));
    }

    if (beat_visualization) {
        window_set_active(window_get("editor_ui"));
    }
    else {
        window_set_active(window_get("main_menu"));
    }

    slog("press [ctrl+q] to quit");

    // level select logic (stolen from my 3D game :P)
    int levelID = 0;
    int selectedLevel = -1;

    GFC_List* levels = gfc_list_new();
    DIR* dir = opendir("levels");
    if (dir == NULL) {
        printf("Could not open levels dir");
        return 0;
    }

    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(".", entry->d_name) != 0 && strcmp("..", entry->d_name) != 0) {
            char* s = gfc_allocate_array(sizeof(char), 256);
            strncpy(s, entry->d_name, 256);
            s[strlen(s) - 5] = '\0';
            gfc_list_append(levels, s);
            // printf("%s\n", entry->d_name);
        }
    }
    closedir(dir);

    float x = 0.0f;

    /*main game loop*/
    while (!done)
    {
        /*update things here*/
        gfc_input_update();
        mouse_input_update();
        keys = SDL_GetKeyboardState(NULL); // get the keyboard state for this frame

        // if (gfc_input_key_down("k")) {
        //     monster_new(gfc_vector2d(gfc_random() * SCREEN_X, gfc_random() * SCREEN_Y));
        // }

        mf += 0.1;
        if (mf >= 16.0)mf = 0;

        // // pause button :P
        // if (button_clicked_by_name("pause_button")) {
        //     slog("pause toggle");
        //     paused = !paused;
        // }

        if (!paused && selectedLevel != -1) {
            entity_manager_think_all();
            entity_manager_update_all();
        }

        if (!level_get() && !window_get_active()) {
            selectedLevel = -1;
            window_set_active(window_get("level_select"));
        }

        if (window_get_active() == window_get("main_menu")) {
            if (button_clicked_by_name("start_button")) {
                window_set_active(window_get("level_select"));
            }
        }

        if (window_get_active() == window_get("shop")) {
            snprintf(text_find("coin_text", window_get_active()->UIElements)->text, GFCLINELEN, "Coins: %i", player_get_coin_count());
            for (int i = 0; i < 5; i++) {
                GFC_TextLine buttonName;
                GFC_TextLine costLabelName;
                snprintf(buttonName, GFCLINELEN, "upgrade_button%i", i + 1);
                snprintf(costLabelName, GFCLINELEN, "cost_text%i", i + 1);

                if (player_owns_upgrade(i)) {
                    gfc_line_cpy(button_find(buttonName, window_get("shop")->UIElements)->label->text, "Purchased");
                    gfc_line_cpy(text_find(costLabelName, window_get("shop")->UIElements)->text, " ");
                }
                else if (button_clicked_by_name(buttonName) && player_get_coin_count() >= player_get_upgrade_cost(i)) {
                    player_buy_upgrade(i);
                }
            }
        }

        if (window_get_active() == window_get("level_select") && selectedLevel == -1) {
            // cycle levels
            if (button_clicked_by_name("right_button")) {
                levelID++;
            }
            if (button_clicked_by_name("left_button")) {
                levelID--;
            }

            if (levelID < 0) levelID = gfc_list_count(levels) - 1;
            if (levelID >= gfc_list_count(levels)) levelID = 0;

            // display level name
            char* level_name = gfc_list_get_nth(levels, levelID);
            UIButton* levelButton = button_find("level_button", window_get_active()->UIElements);
            strcpy(levelButton->label->text, level_name);
            levelButton->label->pos.x = text_center(level_name, levelButton->label->fontSize, 0, SCREEN_X);

            // gf2d_font_draw_line_tag(levelDisplay, FT_Normal, GFC_COLOR_WHITE, gfc_vector2d(1280 / 2 - 150 / 2, 400));

            // shop button
            if (button_clicked_by_name("shop_button")) {
                window_set_active(window_get("shop"));
            }

            if (button_clicked_by_name("level_button")) {
                selectedLevel = levelID;

                char level_path[256] = { 0 };
                sprintf(level_path, "levels/%s.json", level_name);

                // set coins for level
                GFC_HashMap* levelCoins = player_get_level_coins();
                if (!gfc_hashmap_get(levelCoins, level_path)) {
                    gfc_hashmap_insert(levelCoins, level_path, gfc_allocate_array(sizeof(Uint8), 3));
                }

                level_set(level_load(level_path));

                player_editor_mode_get() ? window_set_active(window_get("editor_ui")) : window_set_active(NULL);
                player_reset_no_sound();
                camera_center_on(player_get()->pos);
                paused = 0;
            }
        }

        // pauseButton->icon = paused ? playSprite : pauseSprite;

        // RENDERING
        SDL_SetRenderTarget(gf2d_graphics_get_renderer(), gf2d_graphics_get_screen_texture());
        gf2d_graphics_clear_screen();// clears drawing buffers

        // all drawing should happen betweem clear_screen and next_frame

        level_draw(level_get());

        entity_manager_draw_all();

        // UI elements last
        float charge = player_charge_get();
        GFC_Vector2D chargeScale = gfc_vector2d(5 * charge, 1);
        gf2d_sprite_draw(
            chargeSprite,
            gfc_vector2d(50, 100),
            &chargeScale,
            NULL,
            NULL,
            NULL,
            &chargeGFC_Color,
            0);

        window_draw(window_get_active());

        snprintf(FPS_string, 8, "%.1f", gf2d_graphics_get_frames_per_second());
        text_draw_raw(FPS_string, 32, 32, 32, GFC_COLOR_WHITE);

        // mouse should always be on top
        gf2d_sprite_draw(
            mouse,
            gfc_vector2d(mouse_pos_x(), mouse_pos_y()),
            NULL,
            NULL,
            NULL,
            NULL,
            &mouseGFC_Color,
            (int)mf);

        if (beat_visualization) {
            for (int i = 0; i < beats->count; i++) {
                float scale = 750;
                gf2d_draw_line(gfc_vector2d(((int)gfc_list_get_nth(beats, i)) / scale, SCREEN_Y / 3), gfc_vector2d(((int)gfc_list_get_nth(beats, i)) / scale, SCREEN_Y / 3 * 2), GFC_COLOR_DARKCYAN);
            }
            gf2d_draw_circle(gfc_vector2d(x, SCREEN_Y / 2), 5, GFC_COLOR_CYAN);
            // x += 0.565; // PC
            x += 0.485; // laptop
        }

        // render current draw frame and skip to the next frame
        SDL_SetRenderTarget(gf2d_graphics_get_renderer(), NULL);

        if (player_flipped_get()) {
            gf2d_graphics_next_frame_flipped();
        }
        else {
            gf2d_graphics_next_frame();
        }

        if (keys[SDL_SCANCODE_UP]) camera_set_zoom(camera_get_zoom().x + 0.01);
        if (keys[SDL_SCANCODE_DOWN]) camera_set_zoom(camera_get_zoom().x - 0.01);

        if (gfc_input_key_pressed("t") && level_get()) {
            Mix_HaltChannel(-1);
            player_editor_mode_set(player_editor_mode_get() == 1 ? 0 : 1);

            if (player_editor_mode_get()) {
                window_set_active(window_get("editor_ui"));
            }
            else {
                // player_reset_no_sound();
                window_set_active(NULL);
            }
        }

        if (keys[SDL_SCANCODE_LCTRL] && keys[SDL_SCANCODE_Q])done = 1; // exit condition (lctrl+q)
        if (keys[SDL_SCANCODE_ESCAPE]) {
            level_free(level_get());
            level_set(NULL);
            selectedLevel = -1;
            window_set_active(window_get("level_select"));
        }
        // slog("Rendering at %f FPS", gf2d_graphics_get_frames_per_second());
    }
    slog("---==== END ====---");
    return 0;
}
/*eol@eof*/
