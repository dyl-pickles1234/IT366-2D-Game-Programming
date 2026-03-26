#include <SDL.h>
#include "simple_logger.h"

#include "gfc_input.h"

#include "gf2d_graphics.h"
#include "gf2d_sprite.h"

#include "camera.h"
#include "entity.h"
#include "monster.h"
#include "player.h"

#include "level.h"

#define SCREEN_X 1200
#define SCREEN_Y 768

int main(int argc, char* argv[])
{
    /*variable declarations*/
    int done = 0;
    int paused = 0;
    const Uint8* keys;

    int mx, my;
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

    // Level* level = level_load("levels/showcase.json");
    Level* level = level_load("levels/CantLetGo.json");

    if (!level) { slog("bad level"); return 1; }
    level_set(level);

    player_entity_new(gfc_vector2d(100, 464));
    // player_mode_set(PLAYER_UFO);

    camera_center_on(player_get()->pos);

    // monster_new(gfc_vector2d(200, 250));

    slog("press [escape] to quit");

    /*main game loop*/
    while (!done)
    {
        /*update things here*/
        gfc_input_update();
        keys = SDL_GetKeyboardState(NULL); // get the keyboard state for this frame

        if (gfc_input_key_down("k")) {
            monster_new(gfc_vector2d(gfc_random() * SCREEN_X, gfc_random() * SCREEN_Y));
        }

        Uint32 clicks = SDL_GetMouseState(&mx, &my);

        if (player_flipped_get()) {
            mx = SCREEN_X - mx;
        }

        mf += 0.1;
        if (mf >= 16.0)mf = 0;

        // pause button :P
        if (mx <= 100 && my <= 100 && clicks & SDL_BUTTON(1) != 0 && !paused) {
            slog("paused");
            paused = 1;
        }

        if (mx >= 1100 && my <= 100 && clicks & SDL_BUTTON(1) != 0 && paused) {
            slog("unpaused");
            paused = 0;
        }

        if (!paused) {
            entity_manager_think_all();
            entity_manager_update_all();
        }

        // RENDERING
        SDL_SetRenderTarget(gf2d_graphics_get_renderer(), gf2d_graphics_get_screen_texture());
        gf2d_graphics_clear_screen();// clears drawing buffers

        // all drawing should happen betweem clear_screen and next_frame

        level_draw(level);

        entity_manager_draw_all();

        // UI elements last
        float charge = player_charge_get();
        GFC_Vector2D chargeScale = gfc_vector2d(5 * charge, 1);
        Sprite* chargeSprite = gf2d_sprite_load_all(
            "images/ui/charge.png",
            32,
            32,
            1,
            false);

        gf2d_sprite_draw(
            chargeSprite,
            gfc_vector2d(50, 100),
            &chargeScale,
            NULL,
            NULL,
            NULL,
            &chargeGFC_Color,
            0);

        if (!paused) {
            Sprite* pauseSprite = gf2d_sprite_load_all(
                "images/ui/pause.png",
                100,
                100,
                1,
                false);

            gf2d_sprite_draw(
                pauseSprite,
                gfc_vector2d(0, 0),
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                0);
        }
        else {
            Sprite* playSprite = gf2d_sprite_load_all(
                "images/ui/play.png",
                100,
                100,
                1,
                false);

            gf2d_sprite_draw(
                playSprite,
                gfc_vector2d(1100, 0),
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                0);
        }

        gf2d_sprite_draw(
            mouse,
            gfc_vector2d(mx, my),
            NULL,
            NULL,
            NULL,
            NULL,
            &mouseGFC_Color,
            (int)mf);

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

        if (keys[SDL_SCANCODE_LCTRL] && keys[SDL_SCANCODE_Q])done = 1; // exit condition (lctrl+q)
        // slog("Rendering at %f FPS", gf2d_graphics_get_frames_per_second());
    }
    slog("---==== END ====---");
    return 0;
}
/*eol@eof*/
