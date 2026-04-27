#include "simple_logger.h"

#include "gf2d_graphics.h"

#include "text.h"

static TTF_Font* font;

void text_init() {
    TTF_Init();
    font = TTF_OpenFont("resources/Asap-Regular.ttf", 64);
    if (!font) {
        printf("Failed to load font: %s\n", TTF_GetError());
    }
}

void text_draw(const char* text, float size, int x, int y, GFC_Color col) {
    SDL_Color color = { col.r, col.g, col.b, col.a };
    SDL_Surface* textSurface = TTF_RenderText_Blended(font, text, color);
    SDL_Surface* converted = gf2d_graphics_screen_convert(&textSurface);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(gf2d_graphics_get_renderer(), converted);
    SDL_Rect rect = { x, y, converted->w * (size / 64), converted->h * (size / 64) };
    SDL_RenderCopy(gf2d_graphics_get_renderer(), textTexture, NULL, &rect);
}