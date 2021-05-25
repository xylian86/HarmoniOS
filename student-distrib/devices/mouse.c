#include "mouse.h"
#include "../mouse_graphic.h"
#include "../process_crtl.h"
#include "../terminal.h"
#include "../vga_design.h"
#include "../data/desktop.h"

#define SCREEN_WIDTH_P    720
#define SCREEN_HEIGHT_P   400
#define SIGN_MASK       0xFFFFFF00

int32_t mouse_x_pos = 0;
int32_t mouse_y_pos = 0;

int32_t mouse_x_pos_old = 0;
int32_t mouse_y_pos_old = 0;

void mouse_init(void) {
    uint8_t status;

    /* Aux Input Enable Command */
    // Send the Enable Auxiliary Device command (0xA8) 
    // to port 0x64. This will generate an ACK response from the keyboard, 
    // which you must wait to receive.
    wait_output_to_mouse();
    outb(MOUSE_AUXILIARY_EN, MOUSE_PORT);

    /* Set Compaq Status/Enable IRQ12 */
    // You need to send the command byte 0x20 
    // ("Get Compaq Status Byte") to the PS2 controller 
    // on port 0x64.
    wait_output_to_mouse();
    outb(MOUSE_GET_COMQAQ_STATUS, MOUSE_PORT);

    wait_input_from_mouse();
    status = inb(KEYBOARD_PORT);
    
    // set bit number 1 (value=2, Enable IRQ12), 
    status |= 2;
    // and clear bit number 5 (value=0x20, Disable Mouse Clock)
    status &= 0xDF;

    // Then send command byte 0x60 ("Set Compaq Status") to port 0x64, 
    // followed by the modified Status byte to port 0x60.
    wait_output_to_mouse();
    outb(KEYBOARD_PORT, MOUSE_PORT);
    wait_output_to_mouse();
    outb(status, KEYBOARD_PORT);

    /* Disables streaming, 
    sets the packet rate to 100 per second, 
    and resolution to 4 pixels per mm. */
    write_to_mouse(MOUSE_SET_DEFAULT);
    read_from_mouse();

    /* The mouse starts sending automatic packets 
    when the mouse moves or is clicked. */
    write_to_mouse(MOUSE_ENABLE_STREAM);
    read_from_mouse();

    /* modify the Samplerate and Resolution, 
    and then send a 0xF4 command to the mouse to 
    make the mouse automatically generate movement packets. */
    write_to_mouse(MOUSE_SET_SAMPLE_RATE);
    // read_from_mouse();

    /* set sample rate to 80 */
    wait_output_to_mouse();
    outb(40, KEYBOARD_PORT);

    enable_irq(MOUSE_IRQ);
}

void mouse_handler() {
    mouse_packet1_t mouse_packet_1;
    mouse_packet_1.val = read_from_mouse();
    int32_t mouse_x = read_from_mouse();
    int32_t mouse_y = read_from_mouse();

    send_eoi(MOUSE_IRQ);

    if (mouse_packet_1.x_overflow || mouse_packet_1.y_overflow || mouse_packet_1.always_1 != 1) return;

    /* save old mouse position */
    mouse_x_pos_old = mouse_x_pos;
    mouse_y_pos_old = mouse_y_pos;
    /* should OR 0xFFFFFF00 onto the value of delta Y, as a sign extension (if using 32 bits). */
    if (mouse_packet_1.x_sign) mouse_x_pos += mouse_x | SIGN_MASK;
    else mouse_x_pos += mouse_x;
    /* boundary check */
    if (mouse_x_pos < 0) mouse_x_pos = 0;
    if (mouse_x_pos >= SCREEN_WIDTH_P) mouse_x_pos = SCREEN_WIDTH_P - 1;

    /* should OR 0xFFFFFF00 onto the value of delta Y, as a sign extension (if using 32 bits). */
    if (mouse_packet_1.y_sign) mouse_y_pos -= mouse_y | SIGN_MASK;
    else mouse_y_pos -= mouse_y;
    /* boundary check */
    if (mouse_y_pos < 0) mouse_y_pos = 0;
    if (mouse_y_pos >= SCREEN_HEIGHT_P) mouse_y_pos = SCREEN_HEIGHT_P - 1;
    
    // if(cur_terminal_id != search_owner_terminal(cur_pid) || showing_desktop() == 1)
    // {
    graphic_mouse_clear_force(mouse_x_pos_old, mouse_y_pos_old);
    graphic_mouse_update_force(mouse_x_pos, mouse_y_pos);
    // }else{
    //     graphic_mouse_clear(mouse_x_pos_old, mouse_y_pos_old);
    //     graphic_mouse_update(mouse_x_pos, mouse_y_pos);        
    // }

    if (mouse_packet_1.left_btn == 1) {
        // terminal icon 1
        if (mouse_x_pos >= 0 && mouse_x_pos < TERMINAL_ICON_BLOCK_DIM && mouse_y_pos < TERMINAL_ICON_BLOCK_DIM && mouse_y_pos >= 0) terminal_switch(0);
        // terminal icon 2
        if (mouse_x_pos >= TERMINAL_ICON_BLOCK_DIM && mouse_x_pos < 2*TERMINAL_ICON_BLOCK_DIM && mouse_y_pos < TERMINAL_ICON_BLOCK_DIM && mouse_y_pos >= 0) terminal_switch(1);
        // terminal icon 3
        if (mouse_x_pos >= 2*TERMINAL_ICON_BLOCK_DIM && mouse_x_pos < 3*TERMINAL_ICON_BLOCK_DIM && mouse_y_pos < TERMINAL_ICON_BLOCK_DIM && mouse_y_pos >= 0) terminal_switch(2);
        // minimize
        if (mouse_x_pos >= qemu_vga_xres - TERMINAL_ICON_BLOCK_DIM && mouse_x_pos < qemu_vga_xres && mouse_y_pos < TERMINAL_ICON_BLOCK_DIM && mouse_y_pos >= 0) qemu_vga_show_picture(DESKTOP_IMAGE_WIDTH, DESKTOP_IMAGE_HEIGHT, QEMU_VGA_DEFAULT_BPP, (uint8_t*)DESKTOP_IMAGE_DATA);
    }


    // printf("x:%d y:%d \n", mouse_x_pos, mouse_y_pos);
    
}

/*
 * wait_output_to_mouse
 *   DESCRIPTION: Wait until mouse can receive next packet from CPU.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void wait_output_to_mouse() {
    int timeout = 10000;
    /* All output to port 0x60 or 0x64 must 
    be preceded by waiting for bit 1 (value=2) 
    of port 0x64 to become clear. */
    while(timeout-- && (inb(MOUSE_PORT) & 2) != 0) {
    }
}

/*
 * wait_input_from_mouse
 *   DESCRIPTION: Wait until mouse can send next packet to CPU.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void wait_input_from_mouse() {
    int timeout = 10000;
    /* In that byte from port 0x64, 
    bit number 0 (value=1) indicates that 
    a byte is available to be read on port 0x60. */
    while(timeout-- && (~inb(MOUSE_PORT) & 1) != 0) {
    }
}

/*
 * write_to_mouse
 *   DESCRIPTION: cpu writes data to mouse at port 0x60 for mouse command set.
 *   INPUTS: data - data to be sent to mouse
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: cpu write data to mouse.
 */
void write_to_mouse(uint8_t data) {
    /* wait until mouse is ready to receive data, send "send" command to port 0x64 */
    wait_output_to_mouse();
    outb(MOUSE_SEND_CMD, MOUSE_PORT);
    /* wait until mouse is ready to receive data, send data to port 0x60 */
    wait_output_to_mouse();
    outb(data, KEYBOARD_PORT);
}

/*
 * read_from_mouse
 *   DESCRIPTION: cpu reads data from mouse at port 0x60 for mouse command set.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: data - data in byte from mouse
 *   SIDE EFFECTS: cpu read data sent from mouse
 */
uint8_t read_from_mouse() {
    /* wait until data from mouse is ready, read it from port 0x60 */
    wait_input_from_mouse();
    return inb(KEYBOARD_PORT);
}


