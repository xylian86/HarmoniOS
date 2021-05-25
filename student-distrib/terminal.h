#ifndef TERMINAL_H
#define TERMINAL_H
#include "types.h"
#include "filesystem/filesys.h"
#include "devices/keyboard.h"

int32_t terminal_open();

int32_t terminal_read(int32_t fd, uint8_t *buf, int32_t nbytes);
int32_t terminal_read_intf(int32_t fd, uint32_t* offset, void* buf, int32_t nbytes);

int32_t terminal_write(int32_t fd, uint8_t *buf, int32_t nbytes);
int32_t terminal_write_intf(int32_t fd, const void* buf, int32_t nbytes);

int32_t terminal_close();

file_ops_table_t get_stdin_ops();

file_ops_table_t get_stdout_ops();

#define TERMINAL_NUM 3
typedef struct terminal_t
{
    char* keyboard_buf;
    volatile int enter_pressed_flag;
    int32_t vidmap;
    uint8_t * video_buffer;
    uint32_t buf_position;
    int32_t cursor_x;
    int32_t cursor_y;
    int32_t shell_opened;
}terminal_t;

void multi_terminal_init();

int32_t terminal_switch(int new_terminal);

extern terminal_t terminal_list[TERMINAL_NUM];

extern int32_t active_terminal;

#endif
