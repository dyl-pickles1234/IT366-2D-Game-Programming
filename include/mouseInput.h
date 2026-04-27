#include <SDL.h>

void mouse_input_update();

Uint8 mouse_down(int button);
Uint8 mouse_clicked(int button);
Uint8 mouse_unclicked(int button);

int mouse_pos_x();
int mouse_pos_y();
GFC_Vector2D mouse_pos_get();