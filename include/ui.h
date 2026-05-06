#include <SDL.h>
#include <SDL_ttf.h>

#include "gf2d_sprite.h"
#include "gfc_list.h"
#include "gfc_color.h"
#include "gfc_shape.h"

typedef struct {
    GFC_TextWord name;
    Sprite* bg;
    GFC_Rect bounds;
    GFC_List* UIElements;
} UIWindow;

typedef struct {
    GFC_TextWord name;
    GFC_TextLine text;
    GFC_Vector2D pos;
    Uint32 fontSize;
    GFC_Color color;
} UIText;

typedef struct {
    GFC_TextWord name;
    Sprite* icon;
    GFC_Rect bounds;
    UIText* label;
} UIButton;

void text_init();

UIText* text_new(const char* name, const char* text, float size, int x, int y, GFC_Color col);
void text_free(UIText* text);
void text_draw(UIText* text);
void text_draw_raw(const char* text, float size, int x, int y, GFC_Color col);
float text_estimate_width(char* text, int size);
float text_estimate_centered(char* text, int size, float min, float max);

UIButton* button_new(const char* name, const char* iconPath, int x, int y, int w, int h, char* label);
void button_free(UIButton* button);
Uint8 button_clicked(UIButton* button);
void button_draw(UIButton* button);
UIButton* button_find(const char* name, GFC_List* elements);

void window_draw(UIWindow* window);
void window_set_active(UIWindow* window);
UIWindow* window_get_active();
UIWindow* window_load(const char* filepath);
void window_free(UIWindow* window);