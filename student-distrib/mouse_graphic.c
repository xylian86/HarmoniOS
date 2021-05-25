#include "lib.h"
#include "vga_design.h"
#include "data/mouse_icon.h"

uint32_t background_buf[MOUSE_WIDTH*MOUSE_WIDTH];

void graphic_mouse_update_force(int32_t x, int32_t y)
{
    if(!qemu_vga_enabled) return;
    int32_t i,j;
    for (i = 0; i < MOUSE_WIDTH; i++) {
        for (j = 0; j < MOUSE_WIDTH; j++) {
            background_buf[i*MOUSE_WIDTH+j] = qemu_vga_pixel_get_force(x+i,y+j);
            qemu_vga_pixel_set_force(x+i,y+j,get_color_16(MOUSE_ICON_DATA[i*MOUSE_WIDTH+j]));
        }
    }
}


void graphic_mouse_update(int32_t x, int32_t y)
{
    if(!qemu_vga_enabled) return;
    int32_t i,j;
    for (i = 0; i < MOUSE_WIDTH; i++) {
        for (j = 0; j < MOUSE_WIDTH; j++) {
            background_buf[i*MOUSE_WIDTH+j] = qemu_vga_pixel_get(x+i,y+j);
            qemu_vga_pixel_set(x+i,y+j,get_color_16(MOUSE_ICON_DATA[i*MOUSE_WIDTH+j]));
        }
    }
}

void graphic_mouse_clear_force(int32_t x, int32_t y)
{
    if(!qemu_vga_enabled) return;
    int32_t i,j;
    for (i = 0; i < MOUSE_WIDTH; i++) {
        for (j = 0; j < MOUSE_WIDTH; j++) {
            qemu_vga_pixel_set_force(x+i,y+j,get_color_16(background_buf[i*MOUSE_WIDTH+j]));
        }
    }
}


void graphic_mouse_clear(int32_t x, int32_t y)
{
    if(!qemu_vga_enabled) return;
    int32_t i,j;
    for (i = 0; i < MOUSE_WIDTH; i++) {
        for (j = 0; j < MOUSE_WIDTH; j++) {
            qemu_vga_pixel_set(x+i,y+j,get_color_16(background_buf[i*MOUSE_WIDTH+j]));
        }
    }
}


void graphic_mouse_init()
{
    graphic_mouse_update_force(0, 0);
}
