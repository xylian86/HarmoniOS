#ifndef MOUSE_GRAPHIC
#define MOUSE_GRAPHIC

#include "lib.h"

void graphic_mouse_update(int32_t x, int32_t y);
void graphic_mouse_init();
void graphic_mouse_update_force(int32_t x, int32_t y);
void graphic_mouse_clear_force(int32_t x, int32_t y);
void graphic_mouse_clear(int32_t x, int32_t y);

#endif

