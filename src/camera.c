#include "simple_logger.h"

#include "camera.h"

static Camera camera = { 0 };

GFC_Vector2D camera_get_position() {
    return gfc_vector2d(camera.view.x, camera.view.y);
}

GFC_Vector2D camera_get_offset() {
    return gfc_vector2d(-camera.view.x, -camera.view.y);
}

GFC_Vector2D camera_get_zoom() {
    return gfc_vector2d(camera.zoom, camera.zoom);
}

void camera_set_position(GFC_Vector2D position) {
    camera.view.x = position.x;
    camera.view.y = position.y;
    camera_snap_to_bounds();
}

void camera_set_dimension(GFC_Vector2D dimension) {
    camera.view.w = dimension.x;
    camera.view.h = dimension.y;
    camera_snap_to_bounds();
}

void camera_set_bounds(GFC_Rect bounds) {
    camera.bounds = bounds;
}

void camera_set_zoom(float zoom) {
    camera.zoom = zoom;
}

void camera_center_on(GFC_Vector2D pos) {
    camera_set_position(gfc_vector2d(pos.x - (camera.view.w * 0.5 / camera.zoom), pos.y - (camera.view.h * 0.5 / camera.zoom)));
    camera_snap_to_bounds();
}

void camera_snap_to_bounds()
{
    if (camera.view.x + camera.view.w / camera.zoom > camera.bounds.w) camera.view.x = camera.bounds.w - camera.view.w / camera.zoom;
    if (camera.view.y + camera.view.h / camera.zoom > camera.bounds.h) camera.view.y = camera.bounds.h - camera.view.h / camera.zoom;
    if (camera.view.x < camera.bounds.x) camera.view.x = camera.bounds.x;
    if (camera.view.y < camera.bounds.y) camera.view.y = camera.bounds.y;
}