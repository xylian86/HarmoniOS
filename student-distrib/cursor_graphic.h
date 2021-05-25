#ifndef CURSOR_GRAPHIC
#define CURSOR_GRAPHIC

#include "lib.h"
void graphic_cursor_update(int32_t x, int32_t y);
void graphic_cursor_init();
void graphic_cursor_update_force(int32_t x, int32_t y);

void graphic_cursor_clear_force(int32_t x, int32_t y);
void graphic_cursor_clear(int32_t x, int32_t y);


#endif

