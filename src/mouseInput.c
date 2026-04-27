#include "simple_logger.h"

#include "gfc_vector.h"

#include "player.h"
#include "mouseInput.h"

#define SCREEN_X 1200

static Uint32 prevMouseButtons;
static Uint32 mouseButtons;

static int mouseX, mouseY;

void mouse_input_update() {
    prevMouseButtons = mouseButtons;
    mouseButtons = SDL_GetMouseState(&mouseX, &mouseY);

    if (player_flipped_get()) {
        mouseX = SCREEN_X - mouseX;
    }
}

/**
 * @param button 1: Left; 2: Middle; 3: Right
 */
Uint8 mouse_down(int button) {
    return mouseButtons & SDL_BUTTON(button);
}

/**
 * @param button 1: Left; 2: Middle; 3: Right
 */
Uint8 mouse_clicked(int button) {
    return (mouseButtons & SDL_BUTTON(button)) && !(prevMouseButtons & SDL_BUTTON(button));
}

/**
 * @param button 1: Left; 2: Middle; 3: Right
 */
Uint8 mouse_unclicked(int button) {
    return !(mouseButtons & SDL_BUTTON(button)) && (prevMouseButtons & SDL_BUTTON(button));
}

int mouse_pos_x() {
    return mouseX;
}

int mouse_pos_y() {
    return mouseY;
}

GFC_Vector2D mouse_pos_get() {
    return gfc_vector2d(mouseX, mouseY);
}