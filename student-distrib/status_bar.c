#include "lib.h"
#include "status_bar.h"
#include "data/vga_char.h"
#include "vga_design.h"
#include "process_crtl.h"
#include "terminal.h"
#include "timer.h"
#include "data/terminal_icon.h"
#include "data/minimize.h"

char previous_time_list[TIMER_BUF_LEN] = {0};
// for terminal switch
void swtich_terminal_for_sb()
{
    uint32_t len = 0;
    if(!qemu_vga_enabled)   return;
    if(cur_terminal_id>TERMINAL_NUM) return;

    char word[] = "Current Terminal ID : #0";
    len = strlen(word);
    // update terminal id 
    word[len-1] = '0' + cur_terminal_id;
    
    // update status bar
    message_update_for_sb(word, len, PARM_BLACK_ON_WHITE);
    clock_update_for_sb();
}

// update message in status bar
void message_update_for_sb(char* message, uint32_t len, uint8_t param)
{
    if(!qemu_vga_enabled)   return;
    if(message == NULL) return;
    int i = 0;
    for(i=0;(i<len&&i<STATUS_BAR_MESSAGE_END);i++)
    {
        qemu_vga_putc_force_clock((i+STATUS_BAR_MESSAGE_START)*FONT_ACTUAL_WIDTH, 
        (STATUS_BAR_HEIGHT-1)*FONT_ACTUAL_HEIGHT, 
        message[i], 
        qemu_vga_get_terminal_color(param), 
        qemu_vga_get_terminal_color(param>>FOUR_OFFSET));

    }


    // fulfill the rest of things.
    for(i=len;i<STATUS_BAR_MESSAGE_END;i++)
    {
        qemu_vga_putc_force_clock((i+STATUS_BAR_MESSAGE_START)*FONT_ACTUAL_WIDTH, 
        (STATUS_BAR_HEIGHT-1)*FONT_ACTUAL_HEIGHT, 
        ' ', 
        qemu_vga_get_terminal_color(0xf), 
        qemu_vga_get_terminal_color(0xf));      
    }
}

void clock_update_for_sb()
{
    uint32_t i = 0;
    if(!qemu_vga_enabled) return;
    char time_list[TIMER_BUF_LEN] = "0000/00/00 00:00:00 ";

    cmos_read(0, &i, time_list, TIMER_BUF_LEN);
    time_list[TIMER_BUF_LEN - 1] = ' ';
    for(i=0;i<TIMER_BUF_LEN;i++)
    {
        // if(time_list[i] == previous_time_list[i])
        // {
        //     continue;
        // }
        qemu_vga_putc_clock((STATUS_BAR_TIMER_START + i) * FONT_ACTUAL_WIDTH,
                (STATUS_BAR_HEIGHT - 1) * FONT_ACTUAL_HEIGHT,
                time_list[i], qemu_vga_get_terminal_color(PARM_BLACK_ON_WHITE),
                qemu_vga_get_terminal_color(PARM_BLACK_ON_WHITE >> FOUR_OFFSET)); 
        if (showing_desktop()){
            qemu_vga_putc_force_clock((STATUS_BAR_TIMER_START + i) * FONT_ACTUAL_WIDTH,
                (STATUS_BAR_HEIGHT - 1) * FONT_ACTUAL_HEIGHT,
                time_list[i], qemu_vga_get_terminal_color(PARM_BLACK_ON_WHITE),
                qemu_vga_get_terminal_color(PARM_BLACK_ON_WHITE >> FOUR_OFFSET)); 
        }
         
    }
    memcpy(previous_time_list, time_list, TIMER_BUF_LEN);
}


void draw_terminal_icon(void){
    static int32_t first_entry = 1;
    static uint16_t icon_active[TERMINAL_ICON_BLOCK_DIM*TERMINAL_ICON_BLOCK_DIM];
    static uint16_t icon_inactive[TERMINAL_ICON_BLOCK_DIM*TERMINAL_ICON_BLOCK_DIM];
    int32_t i,j;
    if (first_entry){
        // fill in two picture buffer
        uint16_t focus = 0x632c;
        uint16_t not_focus = 0xFFFF;
        int32_t i_icon,j_icon,idx, idx_icon;
        int32_t pad_row = (TERMINAL_ICON_BLOCK_DIM - TERMINAL_ICON_HEIGHT)/2;
        int32_t pad_col = (TERMINAL_ICON_BLOCK_DIM - TERMINAL_ICON_WIDTH)/2;
        int32_t start_row = pad_row;
        int32_t start_col = pad_col;
        int32_t end_row = TERMINAL_ICON_BLOCK_DIM-1-pad_row;
        int32_t end_col = TERMINAL_ICON_BLOCK_DIM-1-pad_col;
        for (i=0; i<TERMINAL_ICON_BLOCK_DIM; i++){
            for (j=0; j<TERMINAL_ICON_BLOCK_DIM; j++){
                idx = i*TERMINAL_ICON_BLOCK_DIM+j;
                if (i>=start_row && j>=start_col && i<=end_row && j<=end_col){
                    i_icon = i-pad_row;
                    j_icon = j-pad_col;
                    idx_icon = i_icon*TERMINAL_ICON_WIDTH+j_icon;
                    icon_active[idx] = TERM_ICON_DATA[idx_icon];
                    icon_inactive[idx] = TERM_ICON_DATA[idx_icon];
                }
                else{
                    icon_active[idx] = focus;
                    icon_inactive[idx] = not_focus;
                }
            }
        }
        first_entry = 0;
    }
    int32_t cur_icon;
    int32_t minimize_icon_start_col = qemu_vga_xres-1-TERMINAL_ICON_BLOCK_DIM;
    int32_t draw_row, draw_col, start_draw_col, end_draw_col, start_draw_row, icon_row, icon_col, icon_idx;
    for (cur_icon=0; cur_icon<4; cur_icon++){
        start_draw_col = cur_icon*TERMINAL_ICON_BLOCK_DIM;
        end_draw_col = (cur_icon+1)*TERMINAL_ICON_BLOCK_DIM;
        draw_row = 0;
        icon_row = 0;
        icon_col = 0;
        for (draw_row = 0;draw_row<TERMINAL_ICON_BLOCK_DIM; draw_row++){
            for (draw_col = start_draw_col; draw_col<end_draw_col; draw_col++){
                icon_idx = icon_row*TERMINAL_ICON_BLOCK_DIM+icon_col;
                if (cur_icon != 3){
                    if (cur_terminal_id == cur_icon && (!showing_desktop())){
                        qemu_vga_pixel_set_force(draw_col, draw_row, get_color_16(icon_active[icon_idx]));
                    }
                    else{
                        qemu_vga_pixel_set_force(draw_col, draw_row, get_color_16(icon_inactive[icon_idx]));
                    }
                }
                else{
                    qemu_vga_pixel_set_force(minimize_icon_start_col+icon_col, draw_row, get_color_16(MINIMIZE_ICON_DATA[icon_idx]));
                }
                icon_col ++;
            }
            icon_row++;
            icon_col = 0;
        }
    }
    start_draw_col = 3*TERMINAL_ICON_BLOCK_DIM;
    start_draw_row = 0;
    for (i=start_draw_row; i<TERMINAL_ICON_BLOCK_DIM; i++){
        for (j=start_draw_col; j<minimize_icon_start_col; j++){
            qemu_vga_pixel_set_force(j,i,get_color_16(0xce79));
        }
    }
}
