#include "simple_logger.h"

#include "camera.h"

static Camera camera = { 0 };

GFC_Vector2D camera_get_position() {
    return gfc_vector2d(camera.bounds.x, camera.bounds.y);
}

GFC_Vector2D camera_get_offset() {
    return gfc_vector2d(-camera.bounds.x, -camera.bounds.y);
}

void camera_set_position(GFC_Vector2D position);

void camera_set_dimension(GFC_Vector2D dimension) {
    camera.bounds.w = dimension.x;
    camera.bounds.h = dimension.y;
}

void camera_center_on(GFC_Vector2D pos) {
    camera.bounds.x = pos.x - (camera.bounds.w * 0.5);
    camera.bounds.y = pos.y - (camera.bounds.h * 0.5);
}