#include "simple_logger.h"

#include "gf2d_graphics.h"
#include "gf2d_draw.h"

#include "mouseInput.h"

#include "ui.h"

static TTF_Font* font;

static UIWindow* activeWindow;

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

    uiText->type = UI_TEXT;
    gfc_word_cpy(uiText->name, name);
    gfc_line_cpy(uiText->text, text);
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
    SDL_Surface* textSurface = TTF_RenderUTF8_Blended(font, text, color);
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

UIButton* button_new(const char* name, const char* iconPath, int x, int y, int w, int h, char* label) {
    UIButton* uiButton = gfc_allocate_array(sizeof(UIButton), 1);
    if (!uiButton) return NULL;

    uiButton->type = UI_BUTTON;
    gfc_word_cpy(uiButton->name, name);
    uiButton->icon = gf2d_sprite_load_all(iconPath, 32, 32, 1, false);
    uiButton->bounds = gfc_rect(x, y, w, h);
    if (label) uiButton->label = text_new(name, label, uiButton->bounds.h / 4, text_center(label, uiButton->bounds.h / 4, x, x + w), y + h / 3, GFC_COLOR_WHITE);

    return uiButton;
}

void button_free(UIButton* button) {
    if (button) {
        if (button->icon) gf2d_sprite_free(button->icon);
        if (button->label) text_free(button->label);
        free(button);
    }
}

Uint8 button_clicked(UIButton* button) {
    if (!button) return 0;
    return mouse_clicked(1) && gfc_point_in_rect(mouse_pos_get(), button->bounds);
}

void button_draw(UIButton* button) {
    // draw icon
    if (button->icon) gf2d_sprite_draw(button->icon, gfc_vector2d(button->bounds.x, button->bounds.y), NULL, NULL, NULL, NULL, NULL, 0);

    // draw label
    if (button->label) text_draw(button->label);

    // //debug draw bounds
    // gf2d_draw_rect(button->bounds, GFC_COLOR_MAGENTA);
}

UIButton* button_find(const char* name, GFC_List* elements) {
    UIButton* button;
    for (int i = 0; i < elements->count; i++) {
        button = gfc_list_get_nth(elements, i);
        if (strcasecmp(button->name, name) == 0) return button;
    }
    return NULL;
}

Uint8 button_clicked_by_name(const char* name) {
    return button_clicked(button_find(name, window_get_active()->UIElements));
}

UIWindow* window_new(const char* name, const char* bgPath, int x, int y, int w, int h) {
    UIWindow* uiWindow = gfc_allocate_array(sizeof(UIWindow), 1);
    if (!uiWindow) return NULL;

    gfc_word_cpy(uiWindow->name, name);
    uiWindow->bg = gf2d_sprite_load_image(bgPath);
    uiWindow->bounds = gfc_rect(x, y, w, h);
    uiWindow->UIElements = gfc_list_new();

    return uiWindow;
}

void window_draw(UIWindow* window) {
    if (!window) return;

    // draw bg
    if (window->bg) gf2d_sprite_draw_image(window->bg, gfc_vector2d(window->bounds.x, window->bounds.y));

    // draw each UI element
    for (int i = 0; i < window->UIElements->count; i++) {
        void* element = gfc_list_get_nth(window->UIElements, i);
        if (!element) continue;

        UIElementType type = *((UIElementType*)element);
        switch (type)
        {
        case UI_TEXT:
            text_draw((UIText*)element);
            break;
        case UI_BUTTON:
            button_draw((UIButton*)element);
            break;
        }
    }

    // //debug draw bounds
    // gf2d_draw_rect(window->bounds, GFC_COLOR_MAGENTA);
}

void window_set_active(UIWindow* window) {
    activeWindow = window;
}

UIWindow* window_get_active() {
    return activeWindow;
}

UIWindow* window_load(const char* filepath) {

}

void window_free(UIWindow* window) {
    if (window) {
        if (window->bg) gf2d_sprite_free(window->bg);
        if (window->UIElements) gfc_list_delete(window->UIElements);
        free(window);
    }
}

float text_center(char* text, int size, float min, float max) {
    int w;
    if (TTF_SizeUTF8(font, text, &w, NULL) == -1) slog("bad text size grab");
    w *= ((float)size / 64);
    return min + (max - min) / 2 - w / 2;
}