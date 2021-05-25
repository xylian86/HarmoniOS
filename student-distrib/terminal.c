#include "terminal.h"
#include "devices/keyboard.h"
#include "devices/cursor.h"
#include "lib.h"
#include "process_crtl.h"
#include "page.h"
#include "do_syscall.h"
#include "vga_design.h"
#include "status_bar.h"
#include "data/desktop.h"
#include "mouse_graphic.h"
#include "devices/mouse.h"

terminal_t terminal_list[TERMINAL_NUM];
int32_t just_switch = 0;
int32_t active_terminal = 0;


/*
 * terminal_open
 *   DESCRIPTION: do nothing
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: 0 for successful
 */
int32_t terminal_open(){
    return 0;
}

/*
 * terminal_read
 *   DESCRIPTION: read from the keyboard buffer after pressing enter
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: number of bytes copied
 */
int32_t terminal_read(int32_t fd, uint8_t *buf, int32_t nbytes){
    if (buf==NULL)
        return -1;
    int i=0;

    while (!get_enter_press() || 
           (cur_terminal_id!=search_owner_terminal(cur_pid))){}
    clear_enter_press();
    // enter pressed
    while (keyboard_buffer[i] != '\n' && i<nbytes){
        buf[i] = keyboard_buffer[i];
        i++;
    }

    

    // work for history
    // if(buf[0] != 0)
    // {
    //     strncpy(history_buffer_list[history_key.cur_history].bt_buffer, (int8_t*)buf, nbytes);
    //     history_key.cur_history+=1;       
    // }

    // history_key.buffer_index = history_key.cur_history;

    // if(history_key.cur_history==(MAX_HISTORY_BUFFER - MEMORY_HISTORY_LEAK))
    // {
    //     init_history_list();
    //     history_key.cur_history = 0;
    //     history_key.buffer_index = 0;
    // }

    //end for history
    if (i<nbytes){
        buf[i]='\n';
        i++;
    }
    return i;
}

/*
 * terminal_read_intf
 *   DESCRIPTION: function wrapper for uniform interface
 *   INPUTS: only buf and nbytes are used
 *   OUTPUTS: None
 *   RETURN VALUE: follow the core function inside wrapper
 */
int32_t terminal_read_intf(int32_t fd, uint32_t* offset, void* buf, int32_t nbytes){
    return terminal_read(fd, (uint8_t*)buf, nbytes);
}

/*
 * terminal_write
 *   DESCRIPTION: write input buffer to screen by putc
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: 0 for successful
 */
int32_t terminal_write(int32_t fd, uint8_t *buf, int32_t nbytes){
    if (buf==NULL)
        return -1;
    int i=0;
    while (i<nbytes){
        if(buf[i]!='\0')
        {
            putc(buf[i]);  
        }
        i++; 
    }
    return i;
}

/*
 * terminal_write_intf
 *   DESCRIPTION: function wrapper for uniform interface
 *   INPUTS: only buf and nbytes are used
 *   OUTPUTS: None
 *   RETURN VALUE: follow the function inside wrapper
 */
int32_t terminal_write_intf(int32_t fd, const void* buf, int32_t nbytes){
    return terminal_write(fd, (uint8_t*)buf, nbytes);
}

/*
 * terminal_close
 *   DESCRIPTION: do nothing
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: 0 for successful
 */
int32_t terminal_close(){
    return -1;
}

/*
 * check_executable
 *   DESCRIPTION: get operations supported by stdin
 *   INPUTS: None
 *   OUTPUTS: None
 *   RETURN VALUE: file op table
 */
file_ops_table_t get_stdin_ops(){
    static file_ops_table_t op = {NULL, NULL, NULL, terminal_read_intf};
    return op;
}

/*
 * check_executable
 *   DESCRIPTION: get operations supported by stdout
 *   INPUTS: None
 *   OUTPUTS: None
 *   RETURN VALUE: file op table
 */
file_ops_table_t get_stdout_ops(){
    static file_ops_table_t op = {NULL, NULL, terminal_write_intf, NULL};
    return op;
}

void multi_terminal_init()
{
    int i=0;
    for(i=0;i<TERMINAL_NUM;i++)
    {
        terminal_list[i].cursor_x = 0;
        terminal_list[i].cursor_y = 1;
        // the index of background buffer in kernel page table 
        terminal_list[i].video_buffer = (uint8_t*)(VIDEO_MEM_BEGIN+(i+1)*PAGE_SIZE_4K); //assign four KB page for each terminal
        clear_video_buffer(terminal_list[i].video_buffer);
        terminal_list[i].buf_position = 0;
        terminal_list[i].shell_opened = 0;
        terminal_list[i].enter_pressed_flag = 0;
        terminal_list[i].vidmap = 0;
    }
    terminal_list[0].keyboard_buf = keyboard_buffer_1;
    terminal_list[1].keyboard_buf = keyboard_buffer_2;
    terminal_list[2].keyboard_buf = keyboard_buffer_3;

    // set current terminal as 0 terminal
    keyboard_buffer = terminal_list[0].keyboard_buf;
    keyboard_position = terminal_list[0].buf_position;
    set_screen_pos(terminal_list[0].cursor_x, terminal_list[0].cursor_y);
    enter_press = &(terminal_list[0].enter_pressed_flag);

}

int32_t terminal_switch(int new_terminal)
{
    // current = new, we do not switch
    //cli();
    if((new_terminal == cur_terminal_id) && (!showing_desktop()))
    {
        return 0;
    }
    graphic_mouse_clear_force(mouse_x_pos, mouse_y_pos);
    // don't show picture
    disable_desktop_picture();
    // use pointer to modify directly
    terminal_t* cur_t_pointer = &terminal_list[cur_terminal_id];
    terminal_t* new_t_pointer = &terminal_list[new_terminal];

    //store current data
    cur_t_pointer->buf_position = keyboard_position;
    cur_t_pointer->keyboard_buf = keyboard_buffer;

    // load new data
    keyboard_buffer = new_t_pointer->keyboard_buf;
    keyboard_position = new_t_pointer->buf_position;
    *enter_press = 0;
    enter_press = &(new_t_pointer->enter_pressed_flag);

    int32_t new_screen_x = new_t_pointer->cursor_x;
    int32_t new_screen_y = new_t_pointer->cursor_y;
    force_update_cursor(new_screen_x,new_screen_y);

    // video memoey change

    // do not need old video memory
    //uint32_t old_vidmem_base_addr;

    //set_multi_process_vidmem(VIDMEM_SET_PHYSICAL, &old_vidmem_base_addr);
    update_multi_process_vidmem(cur_terminal_id);
    
    memcpy((void*)(cur_t_pointer->video_buffer), (void*)VIDEO_MEM_BEGIN, PAGE_SIZE_4K);
    memcpy((void*)VIDEO_MEM_BEGIN, (void*)(new_t_pointer->video_buffer), PAGE_SIZE_4K);

    // set_multi_process_vidmem(VIDMEM_FORCE_MAPPING, &old_vidmem_base_addr);
    cur_terminal_id = new_terminal;
    
    qemu_vga_switch_terminal(cur_terminal_id);
    swtich_terminal_for_sb();

    //graphic_mouse_update_force(mouse_x_pos, mouse_y_pos);
    // control_block_update(PARM_1_ON_BLACK, PARM_2_ON_BLACK, PARM_3_ON_BLACK);
    draw_terminal_icon();

    // should check wheter running == visable, cannot directly change video memory pointer.
    //update_multi_process_vidmem(search_owner_terminal(cur_pid));

    graphic_mouse_clear_force(mouse_x_pos, mouse_y_pos);
    if (terminal_list[new_terminal].shell_opened == 0){
        set_screen_pos(new_screen_x, new_screen_y);
        execute("shell");
    }
    //sti();
    return 0 ;
}
