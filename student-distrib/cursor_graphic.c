#include "lib.h"
#include "vga_design.h"



void graphic_cursor_update_force(int32_t x, int32_t y)
{
    if(!qemu_vga_enabled) return;
    int i = 0;
    for(;i<FONT_ACTUAL_WIDTH;i++)
    {
        qemu_vga_pixel_set_force(FONT_ACTUAL_WIDTH*x+i,ACTUAL_Y_HEIGHT*(y+1),qemu_vga_get_terminal_color((uint8_t) 0x0F));
    }
}


void graphic_cursor_update(int32_t x, int32_t y)
{
    if(!qemu_vga_enabled) return;
    int i = 0;
    for(;i<FONT_ACTUAL_WIDTH;i++)
    {
        qemu_vga_pixel_set(FONT_ACTUAL_WIDTH*x+i,ACTUAL_Y_HEIGHT*(y+1),qemu_vga_get_terminal_color((uint8_t) 0x0F));
    }
}

void graphic_cursor_clear_force(int32_t x, int32_t y)
{
    if(!qemu_vga_enabled) return;
    int i = 0;
    for(;i<FONT_ACTUAL_WIDTH;i++)
    {
        qemu_vga_pixel_set_force(FONT_ACTUAL_WIDTH*x+i,ACTUAL_Y_HEIGHT*(y+1),qemu_vga_get_terminal_color((uint8_t) 0));
    }
}


void graphic_cursor_clear(int32_t x, int32_t y)
{
    if(!qemu_vga_enabled) return;
    int i = 0;
    for(;i<FONT_ACTUAL_WIDTH;i++)
    {
        qemu_vga_pixel_set(FONT_ACTUAL_WIDTH*x+i,ACTUAL_Y_HEIGHT*(y+1),qemu_vga_get_terminal_color((uint8_t) 0));
    }
}

void graphic_cursor_init()
{
    graphic_cursor_update(0, 32);
}

