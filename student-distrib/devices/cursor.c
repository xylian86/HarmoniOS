#include "cursor.h"
#include "../lib.h"
#include "../process_crtl.h"

//Code reference: https://wiki.osdev.org/Text_Mode_Cursor

/*
 * enable_cursor
 *   DESCRIPTION: Enabling the cursor also allows you to set the start and end scanlines, 
                  the rows where the cursor starts and ends. 
                  The highest scanline is 0 and the lowest scanline is the maximum scanline (usually 15).
 *   INPUTS: cursor start and cursor end, these variables control the size of the cursor.
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Enable the cursor.
 */
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
{
    
    /*
    Ports 0x3D4 and 0x3D5 can be used to read and write the internal registers of the VGA memory.
    The method is to first write the register number to be accessed to port 0x3D4, 
    and then read and write the register data through port 0x3D5.
    */
    outb(0x0A, 0x3D4);      // 0x0A means the index of cursor begining register
    outb((inb(0x3D5) & 0xC0) | cursor_start, 0x3D5);    // set cursor_start

    outb(0x0B, 0x3D4);      //  0x0B means the index of cursor ending register
    outb((inb(0x3D5) & 0xE0) | cursor_end, 0x3D5);      // set cursor_end
}

/*
 * disable_cursor
 *   DESCRIPTION: Disable the cursor
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: disable the cursor.
 */

void disable_cursor()
{
    /*
    Ports 0x3D4 and 0x3D5 can be used to read and write the internal registers of the VGA memory.
    The method is to first write the register number to be accessed to port 0x3D4, 
    and then read and write the register data through port 0x3D5.
    */
    outb(0x0A, 0x3D4);          // 0x0A means low cursor shape
    outb(0x20, 0x3D5);          // bits 6-7 unused, bit 5 disables the cursor, bits 0-4 control the cursor shape
}

/*
 * update_cursor
 *   DESCRIPTION: Update the position of the cursor
 *   INPUTS: position of the screen, x and y. 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: update the position of the cursor
 */
void update_cursor(int x, int y)
{
    int32_t cur_pid_owner_terminal = search_owner_terminal(cur_pid);
    if (cur_pid_owner_terminal != cur_terminal_id)
        return;
    /*
    Ports 0x3D4 and 0x3D5 can be used to read and write the internal registers of the VGA memory.
    The method is to first write the register number to be accessed to port 0x3D4, 
    and then read and write the register data through port 0x3D5.
    */
    uint16_t pos = y*VGA_WIDTH + x; // calculate the position of the cursor.
    //update part
    outb(0x0F, 0x3D4);                                             // operate the 0x0F to get row
    outb((uint8_t)(pos & HIGHER_MASK), 0x3D5);                     // Low 8 bits mean row

    outb(0x0E, 0x3D4);                                             // operate the 0x0E to get column
    outb((uint8_t)((pos >> LEFT_SHIT_EIG) & HIGHER_MASK), 0x3D5);  // high 8 bits mean column
}

void force_update_cursor(int x, int y)
{
    uint16_t pos = y*VGA_WIDTH + x; // calculate the position of the cursor.
    //update part
    outb(0x0F, 0x3D4);                                             // operate the 0x0F to get row
    outb((uint8_t)(pos & HIGHER_MASK), 0x3D5);                     // Low 8 bits mean row

    outb(0x0E, 0x3D4);                                             // operate the 0x0E to get column
    outb((uint8_t)((pos >> LEFT_SHIT_EIG) & HIGHER_MASK), 0x3D5);  // high 8 bits mean column
}


/*
 * get_cursor_position
 *   DESCRIPTION: Get the position of our cursor
 *   INPUTS: none.
 *   OUTPUTS: none
 *   RETURN VALUE: position of the cursor.
 *   SIDE EFFECTS: update the position of the cursor
 */

uint16_t get_cursor_position()
{
    /*
    Ports 0x3D4 and 0x3D5 can be used to read and write the internal registers of the VGA memory.
    The method is to first write the register number to be accessed to port 0x3D4, 
    and then read and write the register data through port 0x3D5.
    */
    uint16_t pos = 0;
    outb(0x0F, 0x3D4);                                   // operate the 0x0F to get row
    pos |= inb(0x3D5);                                   // get row position
    outb(0x0E, 0x3D4);                                   // operate the 0x0E to get column
    pos |= ((uint16_t)inb(0x3D5)) << LEFT_SHIT_EIG;      // get column position

    return pos;
}
