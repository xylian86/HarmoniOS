#ifndef MOUSE_H_
#define MOUSE_H_

#include "../lib.h"
#include "i8259.h"
#include "cursor.h"

#define MOUSE_IRQ                   12
#define MOUSE_PORT                  0x64
#define KEYBOARD_PORT               0x60

#define TERMINAL_ICON_BLOCK_DIM     16

#define MOUSE_SEND_CMD              0xD4
#define MOUSE_ACK                   0xFA
#define MOUSE_AUXILIARY_EN          0xA8
#define MOUSE_GET_COMQAQ_STATUS     0x20
#define MOUSE_SET_DEFAULT           0xF6
#define MOUSE_ENABLE_STREAM         0xF4
#define MOUSE_SET_SAMPLE_RATE       0xF3

typedef union mouse_packet1_t {
    uint8_t val;
    struct {
        uint8_t left_btn    : 1;
        uint8_t right_btn   : 1;
        uint8_t mid_btn     : 1;
        uint8_t always_1    : 1;
        uint8_t x_sign      : 1;
        uint8_t y_sign      : 1;
        uint8_t x_overflow  : 1;
        uint8_t y_overflow  : 1;
    } __attribute__ ((packed));
} mouse_packet1_t;

extern int32_t mouse_x_pos;
extern int32_t mouse_y_pos;
extern int32_t mouse_x_pos_old;
extern int32_t mouse_y_pos_old;

void mouse_init(void);
void mouse_handler();
void wait_output_to_mouse();
void wait_input_from_mouse();
void write_to_mouse(uint8_t data);
uint8_t read_from_mouse();


#endif
