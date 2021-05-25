#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../types.h"

/*The scancode for key release (`break') is obtained from it by setting the high order bit (adding 0x80 = 128). Thus, Esc press produces scancode 01, Esc release scancode 81 (hex). 
*/

#define HIGH_EIGHT_MASK    0xFF
#define KEYBOARD_NUMBER    1
#define KEYBOARD_DATA_PORT  0x60
#define KEYBOARD_CMD_PORT  0x64

#define BACKSPACE_EVAL     8


// special function keys
// RELEASE=PRESS+0X80
#define PRESS_ALT           0x38     //For ALT, left and right are same.
#define RELEASE_ALT         0xB8

#define PRESS_LEFT_SHIFT    0x2A
#define RELEASE_LEFT_SHIFT  0xAA

#define PRESS_RIGHT_SHIFT   0x36
#define RELEASE_RIGHT_SHIFT 0xB6

#define PRESS_CAPSLOCK      0x3A
#define RELEASE_CAPSLOCK    0xBA

#define PRESS_CTRL          0x1D
#define RELEASE_CTRL        0x9D


#define PRESS_ESC           0x01
#define RELEASE_ESC         0X81

#define PRESS_BACKSPACE     0x0E
#define RELEASE_BACKSPACE   0x8E

#define PRESS_TAB           0x0F
#define RELEASE_TAB         0x8F

#define PRESS_ENTER         0x1C
#define RELEASE_ENTER       0x9C

#define PRESS_F1            0x3B
#define PRESS_F2            0x3C
#define PRESS_F3            0x3D

#define PRESS_F4            0x3E
#define PRESS_F10           0x44

#define UP_ARROW            0x48
#define DOWN_ARROW          0x50


#define MAX_BUFFER_SIZE 128

#define MAX_HISTORY_BUFFER 135
#define MEMORY_HISTORY_LEAK 7

extern char* keyboard_buffer;
extern int  keyboard_position;
extern volatile int* enter_press;

extern char keyboard_buffer_1[MAX_BUFFER_SIZE];
extern char keyboard_buffer_2[MAX_BUFFER_SIZE];
extern char keyboard_buffer_3[MAX_BUFFER_SIZE];


typedef struct backtrace_buffer{
    int8_t bt_buffer[MAX_BUFFER_SIZE];
}backtrace_buffer;

typedef struct history_element{
    int32_t cur_history;       // mark as the max content in the history list.
    int32_t buffer_index;      // mark as the current pointer to the history buffer I intend to display.
}history_element;

extern backtrace_buffer history_buffer_list[MAX_HISTORY_BUFFER];
extern history_element history_key;

// init keyboard function
extern void init_keyboard();

// interrupt handler for keyboard function
extern void keyboard_handler();

// deal with special key press
extern int special_key_process(uint8_t scancode_special);

// clear the enter_press
void clear_enter_press();

// get status of enter
int get_enter_press();

void force_putc(unsigned char keyboard_value);

void tab_auto_complete();

void print_auto_complete(char* keyboard_buffer, int8_t* result_buffer, int32_t fill_pos, int32_t cmd_pos, int32_t cmd_len);

void init_history_list();


void print_content(int index);

#endif


