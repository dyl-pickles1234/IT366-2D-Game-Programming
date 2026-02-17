#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "gfc_shape.h"

typedef struct {
    GFC_Rect bounds;
    GFC_Rect view;
    float zoom;
} Camera;

GFC_Vector2D camera_get_position();
GFC_Vector2D camera_get_offset();
GFC_Vector2D camera_get_zoom();
void camera_set_position(GFC_Vector2D position);
void camera_set_dimension(GFC_Vector2D dimension);
void camera_set_bounds(GFC_Rect bounds);
void camera_set_zoom(float zoom);
void camera_center_on(GFC_Vector2D pos);
void camera_snap_to_bounds();

#endif