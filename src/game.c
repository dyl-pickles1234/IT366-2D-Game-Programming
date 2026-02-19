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

int main(int argc, char* argv[])
{
    /*variable declarations*/
    int done = 0;
    const Uint8* keys;

    int mx, my;
    float mf = 0;
    Sprite* mouse;
    GFC_Color mouseGFC_Color = gfc_color8(255, 100, 255, 200);

    /*program initializtion*/
    init_logger("gf2d.log", 0);
    slog("---==== BEGIN ====---");
    gf2d_graphics_initialize(
        "gf2d",
        1200,
        720,
        1200,
        720,
        gfc_vector4d(0, 0, 0, 255),
        0);
    gfc_input_init("config/input.cfg");
    gf2d_graphics_set_frame_delay(16);
    gf2d_sprite_init(1024);
    entity_manager_init(1024);
    SDL_ShowCursor(SDL_DISABLE);

    camera_set_dimension(gfc_vector2d(1200, 720));
    camera_set_zoom(1);

    /*demo setup*/
    mouse = gf2d_sprite_load_all("images/pointer.png", 32, 32, 16, 0);

    Level* level = level_create(
        "images/backgrounds/bg_flat.png",
        "images/tiles/tileset_flat.png",
        32,
        32,
        1,
        48,
        24
    );

    if (!level) { slog("bad level"); return 1; }
    level_add_border(level, 1);

    player_entity_new(gfc_vector2d(100, 100));

    monster_new(gfc_vector2d(200, 250));

    slog("press [escape] to quit");
    /*main game loop*/
    while (!done)
    {
        SDL_PumpEvents();   // update SDL's internal event structures
        keys = SDL_GetKeyboardState(NULL); // get the keyboard state for this frame

        /*update things here*/
        gfc_input_update();

        if (gfc_input_key_down("k")) {
            monster_new(gfc_vector2d(gfc_random() * 1200, gfc_random() * 720));
        }

        SDL_GetMouseState(&mx, &my);
        mf += 0.1;
        if (mf >= 16.0)mf = 0;

        entity_manager_think_all();
        entity_manager_update_all();

        gf2d_graphics_clear_screen();// clears drawing buffers
        // all drawing should happen betweem clear_screen and next_frame
            //backgrounds drawn first
        // gf2d_sprite_draw_image(sprite, gfc_vector2d(0, 0));

        level_draw(level);

        entity_manager_draw_all();

        //UI elements last
        gf2d_sprite_draw(
            mouse,
            gfc_vector2d(mx, my),
            NULL,
            NULL,
            NULL,
            NULL,
            &mouseGFC_Color,
            (int)mf);

        gf2d_graphics_next_frame();// render current draw frame and skip to the next frame

        if (keys[SDL_SCANCODE_UP]) camera_set_zoom(camera_get_zoom().x + 0.01);
        if (keys[SDL_SCANCODE_DOWN]) camera_set_zoom(camera_get_zoom().x - 0.01);

        if (keys[SDL_SCANCODE_LCTRL] && keys[SDL_SCANCODE_Q])done = 1; // exit condition (lctrl+q)
        //slog("Rendering at %f FPS",gf2d_graphics_get_frames_per_second());
    }
    slog("---==== END ====---");
    return 0;
}
/*eol@eof*/
