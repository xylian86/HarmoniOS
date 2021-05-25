#ifndef VGA_DESIGN_H
#define VGA_DESIGN_H

#include "lib.h"
#include "x86_desc.h"

// reference: https://github.com/qemu/vgabios/blob/master/vbe_display_api.txt
#define QEMU_VGA_PORT_INDEX 0x01ce
#define QEMU_VGA_PORT_DATA 0x01cf

#define FAIL   -1
#define SUCCESS   0
#define QEMU_VGA_IDX_ID 0x0
#define QEMU_VGA_IDX_XRES 0x1
#define QEMU_VGA_IDX_YRES 0x2
#define QEMU_VGA_IDX_BPP 0x3
#define QEMU_VGA_IDX_ENABLE 0x4
#define QEMU_VGA_IDX_BANK 0x5
#define QEMU_VGA_IDX_VIRT_WIDTH 0x6
#define QEMU_VGA_IDX_VIRT_HEIGHT 0x7
#define QEMU_VGA_IDX_X_OFFSET 0x8
#define QEMU_VGA_IDX_Y_OFFSET 0x9

#define QEMU_VGA_BANK_SIZE 0x1000000


#define QEMU_VGA_MIN_VER 0xb0c0
#define QEMU_VGA_MAX_VER 0xb0c5


#define QEMU_VGA_DISABLE 0xe0
#define QEMU_VGA_ENABLE 0xe1

#define QEMU_VGA_ENABLE_CLEAR 0x61

#define BITS_IN_BYTE 8

#define FONT_ACTUAL_WIDTH 9
#define FONT_ACTUAL_HEIGHT 16

#define QEMU_VGA_DEFAULT_WIDTH 720
#define QEMU_VGA_DEFAULT_HEIGHT 400
#define QEMU_VGA_DEFAULT_BPP 16

#define UTF8_3BYTE_MASK 0xe0
#define UTF8_2BYTE_MASK 0xc0
#define UTF8_MASK 0x80

extern uint16_t qemu_vga_xres;
extern uint16_t qemu_vga_yres;
extern uint16_t qemu_vga_bpp;
extern uint32_t qemu_vga_addr;
extern uint32_t qemu_vga_enabled;
extern uint32_t qemu_vga_cursor_x;
extern uint32_t qemu_vga_cursor_y;

typedef union {
    uint32_t val;
    struct __attribute__ ((packed)) {
        uint8_t r16         : 5;
        uint8_t g16         : 6;
        uint8_t b16         : 5;
        uint16_t dummy16    : 16;
    };
    struct __attribute__ ((packed)) {
        uint8_t r32;
        uint8_t g32;
        uint8_t b32;
        uint8_t dummy32;
    };
} vga_color_t;

typedef struct {
    // For QEMU VGA
    uint8_t len;    // Length of this UTF-8 code
    uint8_t have;   // Length of what we got
    uint8_t buf[3]; // What we got
    // For putc in lib.c
    uint8_t got;    // How many letters left for UTF-8 code
} utf8_state_t;

uint16_t qemu_vga_read(uint16_t index);
void qemu_vga_write(uint16_t index, uint16_t data);

uint32_t qemu_vga_active_window_addr();
void qemu_vga_switch_terminal(int32_t tid);

uint16_t qemu_vga_init(uint16_t xres, uint16_t yres, uint16_t bpp);
void qemu_vga_pixel_set(uint16_t x, uint16_t y, vga_color_t color);
void qemu_vga_putc(uint16_t x, uint16_t y, uint8_t ch, vga_color_t fg, vga_color_t bg);
void qemu_vga_putc_transparent(uint16_t x, uint16_t y, uint8_t ch, vga_color_t fg);
void qemu_vga_clear();
void qemu_vga_clear_row(uint8_t grid_y);
void qemu_vga_roll_up();
void qemu_vga_set_cursor_pos(uint8_t x, uint8_t y);
vga_color_t qemu_vga_get_terminal_color(uint8_t color);
vga_color_t get_color_16(uint16_t color);
void qemu_vga_show_picture(uint16_t width, uint16_t height, uint8_t bpp, uint8_t* data);
void disable_desktop_picture();
int32_t showing_desktop();
uint32_t qemu_vga_cur_window_addr();
void qemu_vga_pixel_set_force(uint16_t x, uint16_t y, vga_color_t color);
void qemu_vga_putc_force(uint16_t x, uint16_t y, uint8_t ch, vga_color_t fg, vga_color_t bg);
void qemu_vga_roll_up_force();
void qemu_vga_clear_force();
void qemu_vga_clear_row_force(uint8_t grid_y);
void qemu_vga_fill_color(uint16_t x, uint16_t y, uint8_t ch, vga_color_t fg, vga_color_t bg);
uint32_t qemu_vga_cur_picture_addr();
uint32_t qemu_vga_pixel_get(uint16_t x, uint16_t y);
uint32_t qemu_vga_pixel_get_force(uint16_t x, uint16_t y);
void animation(void);

void qemu_vga_putc_clock(uint16_t x, uint16_t y, uint8_t ch, vga_color_t fg, vga_color_t bg);
void qemu_vga_putc_force_clock(uint16_t x, uint16_t y, uint8_t ch, vga_color_t fg, vga_color_t bg);

void qemu_vga_show_picture_by_xy(int32_t x, int32_t y, uint16_t width, uint16_t height, uint8_t bpp, uint8_t* data);
#endif
