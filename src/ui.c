#include "simple_logger.h"
#include "simple_json.h"

#include "gfc_hashmap.h"
#include "gf2d_graphics.h"
#include "gf2d_draw.h"

#include "mouseInput.h"

#include "ui.h"

static TTF_Font* font;

static GFC_HashMap* windows;

static UIWindow* activeWindow;

void ui_init() {
    TTF_Init();
    font = TTF_OpenFont("resources/Asap-Regular.ttf", 64);
    if (!font) {
        printf("Failed to load font: %s\n", TTF_GetError());
    }

    windows = gfc_hashmap_new();
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

UIText* text_find(const char* name, GFC_List* elements) {
    UIText* text;
    for (int i = 0; i < elements->count; i++) {
        text = gfc_list_get_nth(elements, i);
        if (strcasecmp(text->name, name) == 0 && text->type == UI_TEXT) return text;
    }
    return NULL;
}

UIButton* button_new(const char* name, const char* iconPath, int x, int y, int w, int h, const char* label) {
    UIButton* uiButton = gfc_allocate_array(sizeof(UIButton), 1);
    if (!uiButton) return NULL;

    uiButton->type = UI_BUTTON;
    gfc_word_cpy(uiButton->name, name);
    uiButton->icon = gf2d_sprite_load_image(iconPath);
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
    if (button->icon) {
        GFC_Vector2D pos = { button->bounds.x, button->bounds.y };
        pos.x += button->bounds.w / 2;
        pos.y += button->bounds.h / 2;

        GFC_Vector2D center = { button->icon->frame_w / 2,button->icon->frame_h / 2 };
        GFC_Vector2D scale = { button->bounds.w / button->icon->frame_w, button->bounds.h / button->icon->frame_h };
        gf2d_sprite_draw(
            button->icon,
            pos,
            &scale,
            &center,
            NULL,
            NULL,
            NULL,
            0);
    }
    // draw label
    if (button->label && strlen(button->label->text) > 0) text_draw(button->label);

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

void window_load_all(const char* filepath) {
    // UI config JSON
    SJson* uiConfigFile = sj_load(filepath);
    SJson* windowsJson = sj_object_get_value(uiConfigFile, "windows");

    int numWindows = sj_array_get_count(windowsJson);
    GFC_List* windowsJsonList = gfc_list_new_size(numWindows + 1);

    // store each window JSON object
    for (int i = 0; i < numWindows; i++) {
        gfc_list_append(windowsJsonList, sj_array_get_nth(windowsJson, i));
    }

    // parse each window
    SJson* windowJson;

    for (int i = 0; i < numWindows; i++) {
        windowJson = gfc_list_get_nth(windowsJsonList, i);

        // window properties
        SJson* nameJson = sj_object_get_value(windowJson, "name");
        SJson* backgroundJson = sj_object_get_value(windowJson, "background");
        SJson* posJson = sj_object_get_value(windowJson, "pos");
        SJson* sizeJson = sj_object_get_value(windowJson, "size");
        SJson* elementsJson = sj_object_get_value(windowJson, "elements");

        // actual values
        const char* windowName = sj_get_string_value(nameJson);

        const char* backgroundFilename = NULL;
        if (!sj_is_null(backgroundJson)) {
            backgroundFilename = sj_get_string_value(backgroundJson);
        }

        int windowPosX, windowPosY;
        int windowWidth, windowHeight;

        sj_get_integer_value(sj_array_get_nth(posJson, 0), &windowPosX);
        sj_get_integer_value(sj_array_get_nth(posJson, 1), &windowPosY);

        sj_get_integer_value(sj_array_get_nth(sizeJson, 0), &windowWidth);
        sj_get_integer_value(sj_array_get_nth(sizeJson, 1), &windowHeight);

        int numElements = sj_array_get_count(elementsJson);
        GFC_List* elementsJsonList = gfc_list_new_size(numElements + 1);

        for (int j = 0; j < numElements; j++) {
            gfc_list_append(elementsJsonList, sj_array_get_nth(elementsJson, j));
        }

        // create window
        UIWindow* window = window_new(windowName, backgroundFilename, windowPosX, windowPosY, windowWidth, windowHeight);

        // parse elements
        SJson* elementJson;

        for (int j = 0; j < numElements; j++) {
            elementJson = gfc_list_get_nth(elementsJsonList, j);

            // element properties
            SJson* typeJson = sj_object_get_value(elementJson, "type");
            SJson* elementNameJson = sj_object_get_value(elementJson, "name");

            const char* elementType = sj_get_string_value(typeJson);
            const char* elementName = sj_get_string_value(elementNameJson);

            // TEXT ELEMENT
            if (gfc_stricmp(elementType, "text") == 0) {
                SJson* textJson = sj_object_get_value(elementJson, "text");
                SJson* textSizeJson = sj_object_get_value(elementJson, "size");
                SJson* posJson = sj_object_get_value(elementJson, "pos");
                SJson* centeredOnJson = sj_object_get_value(elementJson, "centeredOn");

                const char* text = sj_get_string_value(textJson);

                float textSize;
                sj_get_float_value(textSizeJson, &textSize);

                int posX, posY;
                if (posJson) {
                    sj_get_integer_value(sj_array_get_nth(posJson, 0), &posX);
                    sj_get_integer_value(sj_array_get_nth(posJson, 1), &posY);
                }
                else if (centeredOnJson) {
                    sj_get_integer_value(sj_array_get_nth(centeredOnJson, 0), &posX);
                    sj_get_integer_value(sj_array_get_nth(centeredOnJson, 1), &posY);
                    posX = text_center(text, textSize, posX, posX);
                    // posY -= textSize / 3;
                }

                // create text element
                UIText* textElement = text_new(elementName, text, textSize, posX, posY, GFC_COLOR_WHITE);

                gfc_list_append(window->UIElements, textElement);
            }

            // BUTTON ELEMENT
            else if (gfc_stricmp(elementType, "button") == 0) {
                SJson* iconJson = sj_object_get_value(elementJson, "icon");
                SJson* labelJson = sj_object_get_value(elementJson, "label");

                SJson* posJson = sj_object_get_value(elementJson, "pos");
                SJson* centeredOnJson = sj_object_get_value(elementJson, "centeredOn");
                SJson* sizeJson = sj_object_get_value(elementJson, "size");

                const char* iconFilename = NULL;
                const char* label = NULL;

                if (!sj_is_null(iconJson)) {
                    iconFilename = sj_get_string_value(iconJson);
                }

                if (!sj_is_null(labelJson)) {
                    label = sj_get_string_value(labelJson);
                }

                int width, height;
                sj_get_integer_value(sj_array_get_nth(sizeJson, 0), &width);
                sj_get_integer_value(sj_array_get_nth(sizeJson, 1), &height);

                // support either "pos" or "centeredOn"
                int posX, posY;
                if (posJson) {
                    sj_get_integer_value(sj_array_get_nth(posJson, 0), &posX);
                    sj_get_integer_value(sj_array_get_nth(posJson, 1), &posY);
                }
                else if (centeredOnJson) {
                    sj_get_integer_value(sj_array_get_nth(centeredOnJson, 0), &posX);
                    sj_get_integer_value(sj_array_get_nth(centeredOnJson, 1), &posY);
                    posX -= width / 2;
                    posY -= height / 2;
                }

                // create button
                UIButton* buttonElement = button_new(elementName, iconFilename, posX, posY, width, height, label);
                gfc_list_append(window->UIElements, buttonElement);
            }
        }

        // store window
        gfc_hashmap_insert(windows, window->name, window);
    }

    sj_free(uiConfigFile);
}

UIWindow* window_get(const char* name) {
    return gfc_hashmap_get(windows, name);
}

void window_free(UIWindow* window) {
    if (window) {
        if (window->bg) gf2d_sprite_free(window->bg);
        if (window->UIElements) gfc_list_delete(window->UIElements);
        free(window);
    }
}

float text_center(const char* text, int size, float min, float max) {
    int w;
    if (TTF_SizeUTF8(font, text, &w, NULL) == -1) slog("bad text size grab");
    w *= ((float)size / 64);
    return min + (max - min) / 2 - w / 2;
}