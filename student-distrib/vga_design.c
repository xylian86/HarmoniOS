#include "vga_design.h"
#include "lib.h"
#include "process_crtl.h"
#include "terminal.h"
#include "data/vga_char.h"
#include "data/color.h"
#include "status_bar.h"
#include "devices/mouse.h"
#include "mouse_graphic.h"
#include "data/os.h"

// reference: https://wiki.osdev.org/VGA_Hardware
// reference: https://wiki.osdev.org/VBE
// reference: https://gitee.com/edahub/vgabios
// reference: https://github.com/xddxdd/uiuc-ece391-mp3

// Address of linear buffer, set by PCI scanner on startup
// INIT it as 0, which means do not start VGA. 
uint32_t qemu_vga_addr = 0;

// Current status of QEMU VGA
uint16_t qemu_vga_xres = 0;
uint16_t qemu_vga_yres = 0;
uint16_t qemu_vga_bpp = 0;
uint32_t qemu_vga_enabled = 0;
uint32_t qemu_vga_cursor_x = 0;
uint32_t qemu_vga_cursor_y = 0;

// show desktop picture or not
int32_t show_desktop_picture=0;


/* uint16_t qemu_vga_read(uint16_t index)
 * input: index - index of the register in QEMU VGA
 * output: ret val - data stored in that register
 * description: read from a QEMU VGA register.
 */
uint16_t qemu_vga_read(uint16_t index) {
    outw(index, QEMU_VGA_PORT_INDEX);
    return inw(QEMU_VGA_PORT_DATA);
}


/* void qemu_vga_write(uint16_t index, uint16_t data)
 * input: index - index of register in QEMU VGA
 *         data - data to be written into
 * output: data written into the register
 * description: write to a QEMU VGA register.
 */
void qemu_vga_write(uint16_t index, uint16_t data) {
    outw(index, QEMU_VGA_PORT_INDEX);
    outw(data, QEMU_VGA_PORT_DATA);
}

/* uint32_t qemu_vga_active_window_addr()
 * output: ret val - address of the active window on QEMU VGA linear buffer.
 * description: calculates and returns said address.
 */
uint32_t qemu_vga_active_window_addr() {

    return qemu_vga_addr + active_terminal * (qemu_vga_xres * qemu_vga_yres * qemu_vga_bpp / BITS_IN_BYTE);
}

/* uint32_t qemu_vga_cur_window_addr()
 * output: ret val - address of the current shown window on QEMU VGA linear buffer.
 * description: calculates and returns said address.
 */
uint32_t qemu_vga_cur_window_addr() {

    return qemu_vga_addr + cur_terminal_id * (qemu_vga_xres * qemu_vga_yres * qemu_vga_bpp / BITS_IN_BYTE);
}

/* uint32_t qemu_vga_cur_picture_addr()
 * output: ret val - address of the desktop picture shown window on QEMU VGA linear buffer.
 * description: calculates and returns said address.
 */
uint32_t qemu_vga_cur_picture_addr() {

    return qemu_vga_addr +  3 * (qemu_vga_xres * qemu_vga_yres * qemu_vga_bpp / BITS_IN_BYTE);
}

/* void qemu_vga_switch_terminal(int32_t tid)
 * input: tid - terminal id
 * output: display switches to the specified terminal
 * description: Terminals and desktop are stored continuously in linear buffer:
 * +------------+
 * | Terminal 1 |
 * +------------+
 * | Terminal 2 |
 * +------------+
 * | Terminal 3 |
 * +------------+
 * | Desktop  4 |
 * +------------+
 * so with a change of Y display offset, we can switch between these terminals.
 */
void qemu_vga_switch_terminal(int32_t tid) {
    if(!qemu_vga_enabled) return;
    if(tid >= TERMINAL_NUM){
        if (!(show_desktop_picture==1 &&
              tid==3))
            return;
    }
    qemu_vga_write(QEMU_VGA_IDX_Y_OFFSET, tid * qemu_vga_yres);
}

/* uint16_t qemu_vga_init(uint16_t xres, uint16_t yres, uint16_t bpp)
 * input: xres - X resolution
 *         yres - Y resolution
 *         bpp - bits per pixel, can only be 4, 8, 15, 16, 24, 32,
 *               where this OS only supports 16 and 32.
 * output: ret val - SUCCESS / FAIL
 *          QEMU VGA initialized to said state
 * description: initialized QEMU VGA, including set register and switch from text mode to vga 16/32h mode.
 */
uint16_t qemu_vga_init(uint16_t xres, uint16_t yres, uint16_t bpp) {
    if(0 == qemu_vga_addr) return FAIL;


    // Check if QEMU VGA device is present, and version correct
    qemu_vga_write(QEMU_VGA_IDX_ID, QEMU_VGA_MAX_VER);
    uint16_t ver = qemu_vga_read(QEMU_VGA_IDX_ID);

    if(ver < QEMU_VGA_MIN_VER || ver > QEMU_VGA_MAX_VER) {
        return FAIL;
    }

    // Currently only supports 16 bit and 32 bit BPP.
    // No plan to add more support as we don't use them.
    if(bpp != 16 && bpp != 32) return FAIL;

    // Store state information for later address calculation
    qemu_vga_xres = xres;
    qemu_vga_yres = yres;
    qemu_vga_bpp = bpp;

    // Write the setting into VGA
    qemu_vga_write(QEMU_VGA_IDX_ENABLE, QEMU_VGA_DISABLE);
    qemu_vga_write(QEMU_VGA_IDX_XRES, xres);
    qemu_vga_write(QEMU_VGA_IDX_YRES, yres);
    qemu_vga_write(QEMU_VGA_IDX_BPP, bpp);
    qemu_vga_write(QEMU_VGA_IDX_VIRT_WIDTH, xres);
    qemu_vga_write(QEMU_VGA_IDX_X_OFFSET, 0);
    qemu_vga_write(QEMU_VGA_IDX_Y_OFFSET, 0);
    qemu_vga_write(QEMU_VGA_IDX_ENABLE, QEMU_VGA_ENABLE_CLEAR);
    qemu_vga_enabled = 1;
    return SUCCESS;
}



/* void qemu_vga_pixel_set(uint16_t x, uint16_t y, vga_color_t color)
 * input: x, y - coordinate
 *         color: to be set for this pixel
 * output: pixel (x, y) set to color specified
 * description: sets a pixel on the running terminal.
 */
void qemu_vga_pixel_set(uint16_t x, uint16_t y, vga_color_t color) {
    if(!qemu_vga_enabled) return;
    if(x >= qemu_vga_xres || y >= qemu_vga_yres) return;
    uint32_t pos = qemu_vga_active_window_addr() + (y * qemu_vga_xres + x) * qemu_vga_bpp / BITS_IN_BYTE;

    // Currently only 32 bit and 16 bit color depth is supported.
    if(qemu_vga_bpp == 32) {
        // 32 bit encoding, 0x00RRGGBB
        *((uint32_t*) pos) = color.val & 0xffffff;
    } else if(qemu_vga_bpp == 16) {
        // 16 bit encoding, 5-6-5 as in MP2
        *((uint16_t*) pos) = color.val & 0xffff;
    }
}

/* void qemu_vga_pixel_set_force(uint16_t x, uint16_t y, vga_color_t color)
 * input: x, y - coordinate
 *         color: to be set for this pixel
 * output: pixel (x, y) set to color specified
 * description: sets a pixel on the current terminal.
 */
void qemu_vga_pixel_set_force(uint16_t x, uint16_t y, vga_color_t color) {
    if(!qemu_vga_enabled) return;
    if(x >= qemu_vga_xres || y >= qemu_vga_yres) return;
    uint32_t base = (show_desktop_picture==1) ? qemu_vga_cur_picture_addr(): qemu_vga_cur_window_addr();
    uint32_t pos = base + (y * qemu_vga_xres + x) * qemu_vga_bpp / BITS_IN_BYTE;

    // Currently only 32 bit and 16 bit color depth is supported.
    if(qemu_vga_bpp == 32) {
        // 32 bit encoding, 0x00RRGGBB
        *((uint32_t*) pos) = color.val & 0xffffff;
    } else if(qemu_vga_bpp == 16) {
        // 16 bit encoding, 5-6-5 as in MP2
        *((uint16_t*) pos) = color.val & 0xffff;
    }
}

/* void qemu_vga_putc(uint16_t x, uint16_t y, uint8_t ch, vga_color_t fg, vga_color_t bg)
 * input: x, y - left top corner coordinate for the character
 *         ch - character to be displayed
 *         fg, bg - foreground and background color
 * output: character written at specified position
 * description: writes a character onto the running terminal's screens.
 */

// only support asc ii so far.
void qemu_vga_putc(uint16_t x, uint16_t y, uint8_t ch, vga_color_t fg, vga_color_t bg) {
    if(!qemu_vga_enabled) return;
    int i, j;

    for(i = 0;i < FONT_ACTUAL_HEIGHT; i++)
    {
        for(j = 0; j < FONT_DATA_WIDTH; j++)
        {

            uint8_t mask = 1<<(7-j);
            if(font_data[ch][i]&mask)
            {
                qemu_vga_pixel_set(x + j, y + i, fg);
            }else{
                qemu_vga_pixel_set(x + j, y + i, bg);
            }


        }
        for(j=FONT_DATA_WIDTH;j<FONT_ACTUAL_WIDTH;j++)
        {
            qemu_vga_pixel_set(x + j, y + i, bg);
        }
        

    }
    
}

/* void qemu_vga_putc_force(uint16_t x, uint16_t y, uint8_t ch, vga_color_t fg, vga_color_t bg)
 * input: x, y - left top corner coordinate for the character
 *         ch - character to be displayed
 *         fg, bg - foreground and background color
 * output: character written at specified position
 * description: writes a character onto the current terminal's screens.
 */
void qemu_vga_putc_force(uint16_t x, uint16_t y, uint8_t ch, vga_color_t fg, vga_color_t bg) {
    if(!qemu_vga_enabled) return;
    int i, j;
    graphic_mouse_clear_force(mouse_x_pos, mouse_y_pos);
    for(i = 0;i < FONT_ACTUAL_HEIGHT; i++)
    {
        for(j = 0; j < FONT_DATA_WIDTH; j++)
        {
            if (x*FONT_ACTUAL_WIDTH + j >= mouse_x_pos && x*FONT_ACTUAL_WIDTH + j < mouse_x_pos + 12 && y*FONT_ACTUAL_HEIGHT + i >= mouse_y_pos && y*FONT_ACTUAL_HEIGHT + i < mouse_y_pos + 12) {
                continue;
            }
            uint8_t mask = 1<<(7-j);
            if(font_data[ch][i]&mask)
            {
                qemu_vga_pixel_set_force(x + j, y + i, fg);
            }else{
                qemu_vga_pixel_set_force(x + j, y + i, bg);
            }

        }
        for(j=FONT_DATA_WIDTH;j<FONT_ACTUAL_WIDTH;j++)
        {
            if (x*FONT_ACTUAL_WIDTH + j >= mouse_x_pos && x*FONT_ACTUAL_WIDTH + j < mouse_x_pos + 12 && y*FONT_ACTUAL_HEIGHT + i >= mouse_y_pos && y*FONT_ACTUAL_HEIGHT + i < mouse_y_pos + 12) {
                continue;
            }
            qemu_vga_pixel_set_force(x + j, y + i, bg);
        }
        
    }
    
    graphic_mouse_update_force(mouse_x_pos, mouse_y_pos);
}

/* void qemu_vga_putc_clock(uint16_t x, uint16_t y, uint8_t ch, vga_color_t fg, vga_color_t bg)
 * input: x, y - left top corner coordinate for the character
 *         ch - character to be displayed
 *         fg, bg - foreground and background color
 * output: character written at specified position
 * description: writes a character onto the current terminal's screens, only for updating clock and terminal id.
 */
void qemu_vga_putc_clock(uint16_t x, uint16_t y, uint8_t ch, vga_color_t fg, vga_color_t bg) {
    if(!qemu_vga_enabled) return;
    int i, j;

    for(i = 1;i < FONT_ACTUAL_HEIGHT; i++)
    {
        for(j = 0; j < FONT_DATA_WIDTH; j++)
        {

            uint8_t mask = 1<<(7-j);
            if(font_data[ch][i]&mask)
            {
                qemu_vga_pixel_set(x + j, y + i, fg);
            }else{
                qemu_vga_pixel_set(x + j, y + i, bg);
            }


        }
        for(j=FONT_DATA_WIDTH;j<FONT_ACTUAL_WIDTH;j++)
        {
            qemu_vga_pixel_set(x + j, y + i, bg);
        }
        

    }
    
}

/* void qemu_vga_putc_force_clock(uint16_t x, uint16_t y, uint8_t ch, vga_color_t fg, vga_color_t bg)
 * input: x, y - left top corner coordinate for the character
 *         ch - character to be displayed
 *         fg, bg - foreground and background color
 * output: character written at specified position
 * description: writes a character onto the current terminal's screens, only for updating clock and terminal id.
 */
void qemu_vga_putc_force_clock(uint16_t x, uint16_t y, uint8_t ch, vga_color_t fg, vga_color_t bg) {
    if(!qemu_vga_enabled) return;
    int i, j;
    graphic_mouse_clear_force(mouse_x_pos, mouse_y_pos);
    for(i = 1;i < FONT_ACTUAL_HEIGHT; i++)
    {
        for(j = 0; j < FONT_DATA_WIDTH; j++)
        {
            if (x*FONT_ACTUAL_WIDTH + j >= mouse_x_pos && x*FONT_ACTUAL_WIDTH + j < mouse_x_pos + 12 && y*FONT_ACTUAL_HEIGHT + i >= mouse_y_pos && y*FONT_ACTUAL_HEIGHT + i < mouse_y_pos + 12) {
                continue;
            }
            uint8_t mask = 1<<(7-j);
            if(font_data[ch][i]&mask)
            {
                qemu_vga_pixel_set_force(x + j, y + i, fg);
            }else{
                qemu_vga_pixel_set_force(x + j, y + i, bg);
            }

        }
        for(j=FONT_DATA_WIDTH;j<FONT_ACTUAL_WIDTH;j++)
        {
            if (x*FONT_ACTUAL_WIDTH + j >= mouse_x_pos && x*FONT_ACTUAL_WIDTH + j < mouse_x_pos + 12 && y*FONT_ACTUAL_HEIGHT + i >= mouse_y_pos && y*FONT_ACTUAL_HEIGHT + i < mouse_y_pos + 12) {
                continue;
            }
            qemu_vga_pixel_set_force(x + j, y + i, bg);
        }
        
    }
    
    graphic_mouse_update_force(mouse_x_pos, mouse_y_pos);
}

/* void qemu_vga_fill_color(uint16_t x, uint16_t y, uint8_t ch, vga_color_t fg, vga_color_t bg)
 * input: x, y - left top corner coordinate for the character
 *         ch - character to be displayed
 *         fg, bg - foreground and background color
 * output: character written at specified position
 * description: fill the color on (x,y).
 */
void qemu_vga_fill_color(uint16_t x, uint16_t y, uint8_t ch, vga_color_t fg, vga_color_t bg) {
    if(!qemu_vga_enabled) return;
    int i, j;
    for(i = 0;i < FONT_ACTUAL_HEIGHT; i++)
    {
        for(j = 0; j < FONT_DATA_WIDTH; j++)
        {
            qemu_vga_pixel_set_force(x + j, y + i, fg);
        }
        for(j=FONT_DATA_WIDTH;j<FONT_ACTUAL_WIDTH;j++)
        {
            qemu_vga_pixel_set_force(x + j, y + i, fg);
        }
        
    }
    
}


/* qemu_vga_putc_transparent(uint16_t x, uint16_t y, uint8_t ch, vga_color_t fg)
 * input: x, y - left top corner coordinate for the character
 *         ch - character to be displayed
 *         fg, bg - foreground and background color
 * output: character written at specified position
 * description: fill the color by transparent value.
 */
void qemu_vga_putc_transparent(uint16_t x, uint16_t y, uint8_t ch, vga_color_t fg) {
    if(!qemu_vga_enabled) return;
    int i, j;

    for(i=0;i<FONT_ACTUAL_HEIGHT;i++)
    {
        for(j=0;j<FONT_DATA_WIDTH;j++)
        {
            uint8_t mask = 1<<(7-j);
            if(font_data[ch][i]&mask)
            {
                qemu_vga_pixel_set(x+j, y+i, fg);
            }
        }
    }

}

/* void qemu_vga_clear()
 * input: None
 * output: current terminal's screen filled with black
 * description: clear the running virtual screen.
 */
void qemu_vga_clear() {
    if(!qemu_vga_enabled) return;
    int pos_start = FONT_ACTUAL_HEIGHT * qemu_vga_xres * qemu_vga_bpp / BITS_IN_BYTE;
    memset((char*) qemu_vga_active_window_addr()+pos_start, 0,
        FONT_ACTUAL_HEIGHT * (SCREEN_HEIGHT-2) * qemu_vga_xres * qemu_vga_bpp / BITS_IN_BYTE);
}

/* void qemu_vga_clear_force()
 * input: None
 * output: current terminal's screen filled with black
 * description: clear the running virtual screen.
 */
void qemu_vga_clear_force() {
    if(!qemu_vga_enabled) return;
    int pos_start = FONT_ACTUAL_HEIGHT * qemu_vga_xres * qemu_vga_bpp / BITS_IN_BYTE;
    memset((char*) qemu_vga_cur_window_addr()+pos_start, 0,
        FONT_ACTUAL_HEIGHT * (SCREEN_HEIGHT-2) * qemu_vga_xres * qemu_vga_bpp / BITS_IN_BYTE);
}

/* void qemu_vga_clear_row(uint8_t grid_y)
 * input: grid_y - Y on text mode grid, range 0-24.
 * output: that bar/row filled with black
 * description: clear the row on the running temrinal's screen.
 */
void qemu_vga_clear_row(uint8_t grid_y) {
    if(!qemu_vga_enabled) return;
    int pos_start = grid_y * FONT_ACTUAL_HEIGHT * qemu_vga_xres * qemu_vga_bpp / BITS_IN_BYTE;
    memset((char*) (pos_start + qemu_vga_active_window_addr()), 0,
        FONT_ACTUAL_HEIGHT * qemu_vga_xres * qemu_vga_bpp / BITS_IN_BYTE);
}

/* void qemu_vga_clear_row_force(uint8_t grid_y)
 * input: grid_y - Y on text mode grid, range 0-24.
 * output: that bar/row filled with black
 * description: clear the row on the current temrinal's screen.
 */
void qemu_vga_clear_row_force(uint8_t grid_y) {
    if(!qemu_vga_enabled) return;
    int pos_start = grid_y * FONT_ACTUAL_HEIGHT * qemu_vga_xres * qemu_vga_bpp / BITS_IN_BYTE;
    memset((char*) (pos_start + qemu_vga_cur_window_addr()), 0,
        FONT_ACTUAL_HEIGHT * qemu_vga_xres * qemu_vga_bpp / BITS_IN_BYTE);
}

/* void qemu_vga_roll_up()
 * output: running terminal's screen rolls up one row.
 * description: as above. Note that if there's extra space below the text area,
 *     they will not be touched. Useful for status bars.
 */
void qemu_vga_roll_up() {
    cli();
    if(!qemu_vga_enabled) return;
    int pos_offset = FONT_ACTUAL_HEIGHT * qemu_vga_xres * qemu_vga_bpp / BITS_IN_BYTE;
    int len_roll = (23 - 1) * pos_offset;
    if(cur_terminal_id==search_owner_terminal(cur_pid) && show_desktop_picture != 1)
    {
        graphic_mouse_clear(mouse_x_pos, mouse_y_pos);
    }

    memcpy((char*) qemu_vga_active_window_addr()+pos_offset,
        (char*) (qemu_vga_active_window_addr() + 2*pos_offset),
        len_roll);
    if(cur_terminal_id==search_owner_terminal(cur_pid) && show_desktop_picture != 1)
    {
        graphic_mouse_update(mouse_x_pos, mouse_y_pos);
    }
    if(qemu_vga_cursor_y > 0) qemu_vga_cursor_y -= 1;
    sti();
}

/* void qemu_vga_roll_up_force()
 * output: current terminal's screen rolls up one row.
 * description: as above. Note that if there's extra space below the text area,
 *     they will not be touched. Useful for status bars.
 */
void qemu_vga_roll_up_force() {
    if(!qemu_vga_enabled) return;
    int pos_offset = FONT_ACTUAL_HEIGHT * qemu_vga_xres * qemu_vga_bpp / BITS_IN_BYTE;
    int len_roll = (23 - 1) * pos_offset;
    graphic_mouse_clear_force(mouse_x_pos, mouse_y_pos);
    memcpy((char*) qemu_vga_cur_window_addr()+pos_offset,
        (char*) (qemu_vga_cur_window_addr() + 2*pos_offset),
        len_roll);
    graphic_mouse_update_force(mouse_x_pos, mouse_y_pos);
    if(qemu_vga_cursor_y > 0) qemu_vga_cursor_y -= 1;
}

/* void qemu_vga_set_cursor_pos(uint8_t x, uint8_t y)
 * input: x, y - cursor coordinate
 * output: cursor moved to that position (currently NOT WORKING)
 * description: simulates VGA cursor, NOT WORKING NOW
 */
void qemu_vga_set_cursor_pos(uint8_t x, uint8_t y) {
    if(!qemu_vga_enabled) return;
    if(cur_terminal_id != search_owner_terminal(cur_pid)) return;
    if(qemu_vga_cursor_x == x && qemu_vga_cursor_y == y) return;

    // // erase cursor on current location by redrawing the character
    // uint8_t ch = *(volatile uint8_t*) (TERMINAL_DIRECT_ADDR + (SCREEN_WIDTH * qemu_vga_cursor_y + qemu_vga_cursor_x) * 2);
    // uint8_t attrib = *(volatile uint8_t*) (TERMINAL_DIRECT_ADDR + (SCREEN_WIDTH * qemu_vga_cursor_y + qemu_vga_cursor_x) * 2 + 1);
    // qemu_vga_putc(qemu_vga_cursor_x * FONT_ACTUAL_WIDTH,
    //     qemu_vga_cursor_y * FONT_ACTUAL_HEIGHT,
    //     ch, qemu_vga_get_terminal_color(attrib), qemu_vga_get_terminal_color(attrib >> 4));

    // qemu_vga_putc_transparent(qemu_vga_cursor_x * FONT_ACTUAL_WIDTH,
    //     qemu_vga_cursor_y * FONT_ACTUAL_HEIGHT,
    //     '_', qemu_vga_get_terminal_color(ATTRIB >> 4));

    qemu_vga_cursor_x = x;
    qemu_vga_cursor_y = y;

    // // draw cursor on new location by drawing an underline
    // qemu_vga_putc_transparent(qemu_vga_cursor_x * FONT_ACTUAL_WIDTH,
    //     qemu_vga_cursor_y * FONT_ACTUAL_HEIGHT,
    //     '_', qemu_vga_get_terminal_color(ATTRIB));
}

/* vga_color_t qemu_vga_get_terminal_color(uint8_t color)
 * input: color - color code for console, only last 8 bit used
 * output: ret val - color in 16 bit or 32 bit, depending on init setting
 * description: translates terminal color to console color
 */
vga_color_t qemu_vga_get_terminal_color(uint8_t color) {
    return (qemu_vga_bpp == 16 ? terminal_color_16 : terminal_color_32)[(color & 0xff)];
}


/* vvga_color_t get_color_16(uint16_t color)
 * input: color - color code 16 bit
 * output: ret val - color in 16 bit or 32 bit, depending on init setting
 * description: renew the color
 */
vga_color_t get_color_16(uint16_t color){
    vga_color_t ret;
    ret.val = (uint32_t)color;
    return ret;
}

/* void disable_desktop_picture()
 * input: none
 * output: none
 * description: set the picture shown signal as 0.
 */
void disable_desktop_picture(){
    show_desktop_picture = 0;
}

/* int32_t showing_desktop()
 * input: none
 * output: none
 * description: get the picture shown signal.
 */
int32_t showing_desktop(){
    return show_desktop_picture;
}

/* void qemu_vga_show_picture(uint16_t width, uint16_t height, uint8_t bpp, uint8_t* data)
 * input: width, height - size of picture, cannot exceed screen resolution.
 *         bpp - color depth, MUST BE same as screen, or strange problem will occur
 *         data - data of image
 * output: picture drawn on left top corner of screen,
 *          cursor moved downwards if overlapping with picture
 * description: show a picture at left top corner of the screen.
 */
void qemu_vga_show_picture(uint16_t width, uint16_t height, uint8_t bpp, uint8_t* data) {
    if(!qemu_vga_enabled) return;
    if(width > qemu_vga_xres || height > qemu_vga_yres || bpp != qemu_vga_bpp) return;
    graphic_mouse_clear_force(mouse_x_pos, mouse_y_pos);
    // Copy over the image, row by row
    int row = qemu_vga_xres * qemu_vga_bpp / BITS_IN_BYTE;
    int i;
    show_desktop_picture = 1;
    for(i = 0; i < (height-16); i++) {
        memcpy((char*) (qemu_vga_cur_picture_addr()+ row),
            (char*) (data + i * width * bpp / BITS_IN_BYTE), width * bpp / BITS_IN_BYTE);
        row += qemu_vga_xres * qemu_vga_bpp / BITS_IN_BYTE;
    }
    draw_terminal_icon();
    char word[] = "DESKTOP";
    uint32_t len = strlen(word);
    message_update_for_sb(word, len, PARM_BLACK_ON_WHITE);
    qemu_vga_switch_terminal(3);
}

/* void qemu_vga_show_picture_by_xy(int32_t x, int32_t y, uint16_t width, uint16_t height, uint8_t bpp, uint8_t* data) 
 * input:  x,y : start position
 *         width, height - size of picture, cannot exceed screen resolution.
 *         bpp - color depth, MUST BE same as screen, or strange problem will occur
 *         data - data of image
 * output: picture drawn on left top corner of screen,
 *          cursor moved downwards if overlapping with picture
 * description: show a picture at left top corner of the screen.
 */

void qemu_vga_show_picture_by_xy(int32_t x, int32_t y, uint16_t width, uint16_t height, uint8_t bpp, uint8_t* data) {
    if(!qemu_vga_enabled) return;
    if(width > qemu_vga_xres || height > qemu_vga_yres || bpp != qemu_vga_bpp) return;

    // Copy over the image, row by row
    int row = 0;
    int i;
    for(i = 0; i < (height); i++) {
        memcpy((char*) (qemu_vga_cur_window_addr()+ ((y*ACTUAL_Y_HEIGHT*qemu_vga_yres+x*FONT_ACTUAL_WIDTH)*qemu_vga_bpp) / BITS_IN_BYTE+row),
            (char*) (data + i * width * bpp / BITS_IN_BYTE), width * bpp / BITS_IN_BYTE);
        row += qemu_vga_xres * qemu_vga_bpp / BITS_IN_BYTE;
    }

    // Move cursor downwards to avoid overlapping with picture
    if(terminal_list[cur_terminal_id].cursor_y < height / FONT_ACTUAL_HEIGHT) {
        terminal_list[cur_terminal_id].cursor_y = height / FONT_ACTUAL_HEIGHT + 1;
        terminal_list[cur_terminal_id].cursor_x = 0;
    }
}


/* uint32_t qemu_vga_pixel_get(uint16_t x, uint16_t y, vga_color_t color)
 * input: x, y - coordinate
 *         color: to be set for this pixel
 * output: pixel (x, y) set to color specified
 * description: sets a pixel.
 */
uint32_t qemu_vga_pixel_get(uint16_t x, uint16_t y) {
    if(!qemu_vga_enabled) return FAIL;
    if(x >= qemu_vga_xres || y >= qemu_vga_yres) return FAIL;
    uint32_t pos = qemu_vga_active_window_addr() + (y * qemu_vga_xres + x) * qemu_vga_bpp / BITS_IN_BYTE;

    // Currently only 32 bit and 16 bit color depth is supported.
    if(qemu_vga_bpp == 32) {
        // 32 bit encoding, 0x00RRGGBB
        return *((uint32_t*) pos);
    } else if(qemu_vga_bpp == 16) {
        // 16 bit encoding, 5-6-5 as in MP2
        return *((uint16_t*) pos);
    }
    return FAIL;
}

/* uint32_t qemu_vga_pixel_get_force(uint16_t x, uint16_t y, vga_color_t color)
 * input: x, y - coordinate
 *         color: to be set for this pixel
 * output: pixel (x, y) set to color specified
 * description: sets a pixel.
 */
uint32_t qemu_vga_pixel_get_force(uint16_t x, uint16_t y) {
    if(!qemu_vga_enabled) return FAIL;
    if(x >= qemu_vga_xres || y >= qemu_vga_yres) return FAIL;
    uint32_t base = (show_desktop_picture==1) ? qemu_vga_cur_picture_addr(): qemu_vga_cur_window_addr();
    uint32_t pos = base + (y * qemu_vga_xres + x) * qemu_vga_bpp / BITS_IN_BYTE;

    // Currently only 32 bit and 16 bit color depth is supported.
    if(qemu_vga_bpp == 32) {
        // 32 bit encoding, 0x00RRGGBB
        return *((uint32_t*) pos);
    } else if(qemu_vga_bpp == 16) {
        // 16 bit encoding, 5-6-5 as in MP2
        return *((uint16_t*) pos);
    }
    return FAIL;
}

void animation(void) {

    int32_t i, j;
    for (i=0; i<OS_IMAGE_HEIGHT; i++) {
        for (j=0; j<OS_IMAGE_WIDTH; j++) {
            qemu_vga_pixel_set_force(j+180, i+20, get_color_16(OS_IMAGE_DATA[i*OS_IMAGE_WIDTH+j]));
        }
    }

    for (i=0; i<START_IMAGE_HEIGHT; i++) {
        for (j=0; j<START_IMAGE_WIDTH; j++) {
            qemu_vga_pixel_set_force(j+240, i+140, get_color_16(START_IMAGE_DATA[i*START_IMAGE_WIDTH+j]));
        }
    }

    int32_t counter = 1;
    while(counter < 40000000) {

        if (counter % 5000000 == 0) {
            for (i=0; i<P_IMAGE_HEIGHT; i++) {
                for (j=0; j<P_IMAGE_WIDTH; j++) {
                    qemu_vga_pixel_set_force(j+200+(counter-1000000)/150000, i+240, qemu_vga_get_terminal_color((uint8_t) 0x0));
                }
            }

            for (i=0; i<P_IMAGE_HEIGHT; i++) {
                for (j=0; j<P_IMAGE_WIDTH; j++) {
                    qemu_vga_pixel_set_force(j+200+counter/150000, i+240, get_color_16(P1_IMAGE_DATA[i*P_IMAGE_WIDTH+j]));
                }
            }
        } else if (counter % 5000000 == 1000000) {
            for (i=0; i<P_IMAGE_HEIGHT; i++) {
                for (j=0; j<P_IMAGE_WIDTH; j++) {
                    qemu_vga_pixel_set_force(j+200+(counter-1000000)/150000, i+240, qemu_vga_get_terminal_color((uint8_t) 0x0));
                }
            }

            for (i=0; i<P_IMAGE_HEIGHT; i++) {
                for (j=0; j<P_IMAGE_WIDTH; j++) {
                    qemu_vga_pixel_set_force(j+200+counter/150000, i+240, get_color_16(P2_IMAGE_DATA[i*P_IMAGE_WIDTH+j]));
                }
            }
        } else if (counter % 5000000 == 2000000) {
            for (i=0; i<P_IMAGE_HEIGHT; i++) {
                for (j=0; j<P_IMAGE_WIDTH; j++) {
                    qemu_vga_pixel_set_force(j+200+(counter-1000000)/150000, i+240, qemu_vga_get_terminal_color((uint8_t) 0x0));
                }
            }

            for (i=0; i<P_IMAGE_HEIGHT; i++) {
                for (j=0; j<P_IMAGE_WIDTH; j++) {
                    qemu_vga_pixel_set_force(j+200+counter/150000, i+240, get_color_16(P3_IMAGE_DATA[i*P_IMAGE_WIDTH+j]));
                }
            }
        } else if (counter % 5000000 == 3000000) {
            for (i=0; i<P_IMAGE_HEIGHT; i++) {
                for (j=0; j<P_IMAGE_WIDTH; j++) {
                    qemu_vga_pixel_set_force(j+200+(counter-1000000)/150000, i+240, qemu_vga_get_terminal_color((uint8_t) 0x0));
                }
            }

            for (i=0; i<P_IMAGE_HEIGHT; i++) {
                for (j=0; j<P_IMAGE_WIDTH; j++) {
                    qemu_vga_pixel_set_force(j+200+counter/150000, i+240, get_color_16(P4_IMAGE_DATA[i*P_IMAGE_WIDTH+j]));
                }
            }
        } else if (counter % 5000000 == 4000000) {
            for (i=0; i<P_IMAGE_HEIGHT; i++) {
                for (j=0; j<P_IMAGE_WIDTH; j++) {
                    qemu_vga_pixel_set_force(j+200+(counter-1000000)/150000, i+240, qemu_vga_get_terminal_color((uint8_t) 0x0));
                }
            }

            for (i=0; i<P_IMAGE_HEIGHT; i++) {
                for (j=0; j<P_IMAGE_WIDTH; j++) {
                    qemu_vga_pixel_set_force(j+200+counter/150000, i+240, get_color_16(P5_IMAGE_DATA[i*P_IMAGE_WIDTH+j]));
                }
            }
        }

        counter++;
    }
    
    for (i=0; i<P_IMAGE_HEIGHT; i++) {
        for (j=0; j<P_IMAGE_WIDTH; j++) {
            qemu_vga_pixel_set_force(j+200+(counter-1000000)/150000, i+240, qemu_vga_get_terminal_color((uint8_t) 0x0));
        }
    }

    for (i=0; i<START_IMAGE_HEIGHT; i++) {
        for (j=0; j<START_IMAGE_WIDTH; j++) {
            qemu_vga_pixel_set_force(j+240, i+140, qemu_vga_get_terminal_color((uint8_t) 0x0));
        }
    }


    for (i=0; i<OS_IMAGE_HEIGHT; i++) {
        for (j=0; j<OS_IMAGE_WIDTH; j++) {
            qemu_vga_pixel_set_force(j+180, i+20, qemu_vga_get_terminal_color((uint8_t) 0x0));
        }
    }

}

