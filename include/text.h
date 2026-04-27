#include <SDL.h>
#include <SDL_ttf.h>

#include "gfc_color.h"

void text_init();

void text_draw(const char* text, float size, int x, int y, GFC_Color col);