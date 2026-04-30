#include "simple_logger.h"

#include "gf2d_graphics.h"
#include "gf2d_draw.h"

#include "mouseInput.h"

#include "ui.h"

static TTF_Font* font;

void text_init() {
    TTF_Init();
    font = TTF_OpenFont("resources/Asap-Regular.ttf", 64);
    if (!font) {
        printf("Failed to load font: %s\n", TTF_GetError());
    }
}

UIText* text_new(const char* name, const char* text, float size, int x, int y, GFC_Color col) {
    UIText* uiText = gfc_allocate_array(sizeof(UIText), 1);
    if (!uiText) return NULL;

    gfc_word_cpy(uiText->name, name);
    gfc_line_cpy(uiText->text, name);
    uiText->fontSize = size;
    uiText->pos = gfc_vector2d(x, y);
    uiText->color = col;

    return uiText;
}

void text_free(UIText* text) {
    if (text) free(text);
}

void text_draw_raw(const char* text, float size, int x, int y, GFC_Color col) {
    SDL_Color color = { col.r, col.g, col.b, col.a };
    SDL_Surface* textSurface = TTF_RenderText_Blended(font, text, color);
    SDL_Surface* converted = gf2d_graphics_screen_convert(&textSurface);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(gf2d_graphics_get_renderer(), converted);
    SDL_Rect rect = { x, y, converted->w * (size / 64), converted->h * (size / 64) };
    SDL_RenderCopy(gf2d_graphics_get_renderer(), textTexture, NULL, &rect);

    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(converted);
    SDL_FreeSurface(textSurface);
}

void text_draw(UIText* text) {
    text_draw_raw(text->text, text->fontSize, text->pos.x, text->pos.y, text->color);
}

Uint8 button_clicked(UIButton* button) {
    return mouse_clicked(1) && gfc_point_in_rect(mouse_pos_get(), button->bounds);
}

void button_draw(UIButton* button) {
    // draw icon
    // draw label
    // text_draw(button->label)
    gf2d_draw_rect(button->bounds, GFC_COLOR_MAGENTA);
}

UIButton* button_find(const char* name, GFC_List* elements) {
    UIButton* button;
    for (int i = 0; i < elements->count; i++) {
        button = gfc_list_get_nth(elements, i);
        if (strcasecmp(button->name, name) == 0) return button;
    }
    return NULL;
}

void window_draw(UIWindow* window) {

}

void window_set_active(UIWindow* window);
UIWindow* window_get_active();
UIWindow* window_load(const char* filepath);
void window_free(UIWindow* window);