#ifndef CURSOR_H
#define CURSOR_H

#include "../types.h"

#define VGA_WIDTH 80                // Lenght of VGA
#define HIGHER_MASK 0xFF            // Only use lower 8 bits
#define LEFT_SHIT_EIG 8             // Left shift 8 bits.


// enable the cursor
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end);

// disable the cursor
void disable_cursor();

// update the position of the cursor
void update_cursor(int x, int y);

void force_update_cursor(int x, int y);

// get the current position of the cursor
uint16_t get_cursor_position();

#endif
