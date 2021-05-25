#include "../lib.h"
#include "keyboard.h"
#include "i8259.h"
#include "scancode.h"
#include "../terminal.h"
#include "../page.h"
#include "../data/desktop.h"
#include "../process_crtl.h"
#include "cursor.h"
#include "../vga_design.h"
#include "../signal.h"
// static helper
// static void force_putc(unsigned char keyboard_value);

//backtrace_buffer history_buffer_list[MAX_HISTORY_BUFFER];
// keyboard speical key signal list
static int ctrl_press = 0;
static int caps_lock_press = 0;
static int caps_count =0;
static int shift_press = 0;
static int alt_press = 0;
static int tab_press = 0;
static int enter_esc = 0;

// press enter?
volatile int * enter_press;

//keyboard char buffer and current position of the cursor.
char* keyboard_buffer;

int keyboard_position;

char keyboard_buffer_1[MAX_BUFFER_SIZE];
char keyboard_buffer_2[MAX_BUFFER_SIZE];
char keyboard_buffer_3[MAX_BUFFER_SIZE];

backtrace_buffer history_buffer_list[MAX_HISTORY_BUFFER];
history_element history_key;

/*
 * init_keyboard
 *   DESCRIPTION: Initialize the 8259 keyboard interrupt. The entry is 1.
 *   INPUTS: none (Keyboard number for PIC)
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Enable the interrupt request from keyboard.
 */
void init_keyboard()
{
    enable_irq(KEYBOARD_NUMBER);
}
 
/*
 * keyboard_handler (for cp2)
 *   DESCRIPTION: Keyboard handler, it will not print the value of functional keys.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Display all the useful chars on the screen, without left/right/up/down arrow and keypad.
 */
void keyboard_handler()
{
    uint8_t keyboard_input;             // index for keyboard input in scancode array
    unsigned char keyboard_value;       // fill the corresponding value in keyboard scancode array

    send_eoi(KEYBOARD_NUMBER);          //send end of interrupt message to PIC

    keyboard_input = inb(KEYBOARD_DATA_PORT) & HIGH_EIGHT_MASK; //receive the index from keyboard input

    // Release key is out of index, so we need to handle it at first.
    if(special_key_process(keyboard_input) == 0)
        return;

    //test if input index is valid.
    if(keyboard_input >= SCANCODE_SIZE)
        return;

    // get the raw keyboard_value, init it as the 0 col in scancode (this is caps_count = 0 and shift_press = 0 condition).
    keyboard_value = scancode_array[keyboard_input][0];             // only read lowercase char and number

    //There are four conditions for caps_lock key and shft key
    if((caps_count == 1) && (shift_press == 0)){
        keyboard_value = scancode_array[keyboard_input][0];
        if(((keyboard_value>='a')&&(keyboard_value<='z')))
        {
            keyboard_value = scancode_array[keyboard_input][1]; 
        }
    }

    if((caps_count == 1) && (shift_press == 1)){
        keyboard_value = scancode_array[keyboard_input][0]; 
        if(!((keyboard_value>='a')&&(keyboard_value<='z')))
        {
            keyboard_value = scancode_array[keyboard_input][1]; 
        }
    }

    if((caps_count == 0) && (shift_press == 1)){
        keyboard_value = scancode_array[keyboard_input][1]; 
    }

    // check_number=((keyboard_value >= '0') && (keyboard_value <= '9'));    // check number  0-9
    // check_lower_char=((keyboard_value >= 'a') && (keyboard_value <= 'z'));// check lowercase char. a-z
    // check_higher_char=((keyboard_value >= 'A') && (keyboard_value <= 'Z'));// check highercase char. A-Z
    // check_space = (keyboard_value==' ');

    //if we press the function key with other keys, the other keys should not display.
    if(ctrl_press)
    {
        if((keyboard_value=='l') || (keyboard_value=='L'))
        {
            clear();
        }
        if((keyboard_value=='c') || (keyboard_value=='C'))
        {
            signal_raise(2); // signal part
        }
        return;
    }

    // if the alt key is pressed, we do nothing but return.
    if(alt_press){
        return;
    }

    // cannot allow to print if we meet the end of the buffer
    if((keyboard_position>=MAX_BUFFER_SIZE-1) && (keyboard_value!='\n')){
        return;
    }
    if (showing_desktop()){
        return;
    }
    // Deal with the condition that input enter.
    if(keyboard_value=='\n')
    {
        *enter_press = 1;
        //read_buffer
        force_putc(keyboard_value);
        strncpy(history_buffer_list[history_key.cur_history].bt_buffer, keyboard_buffer, keyboard_position);
        history_key.cur_history+=1;
        keyboard_buffer[keyboard_position] = keyboard_value;
        keyboard_position = 0;      // init the start position
    }else{
        if(keyboard_value!=0)
        {
            force_putc(keyboard_value);
            keyboard_buffer[keyboard_position] = keyboard_value;
            keyboard_position += 1;
        }
    }

        history_key.buffer_index = history_key.cur_history;
        if(history_key.cur_history == MAX_HISTORY_BUFFER)
        {
            init_history_list();
        }
        return;


}


/*
 * speical_key_process
 *   DESCRIPTION: Handle with special keys, it works like a MUX. We need to deal with key press signal and key release signal.
 *   INPUTS: scancode_index of the spcial key. 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: set the signal we need to use.
 */
int special_key_process(uint8_t scancode_special)
{
    //int tab_index = 0;  // used for counting the space printed by tab

    // We always set the signal as 0 when we release the key, and set the signal as 1 when we press the key. 
    switch (scancode_special)
    {

    case PRESS_ESC:
        enter_esc = 1;
        return 0;
        break;
    
    case RELEASE_ESC:
        enter_esc = 0;
        return 0;
        break;
        
    case PRESS_CTRL:
        ctrl_press = 1;
        return 0;
        break;

    case RELEASE_CTRL:
        ctrl_press = 0;
        return 0;
        break;

    case PRESS_CAPSLOCK:
        if(caps_count == 1)
        {
            caps_count = 0;
            return 0;
        }
        caps_lock_press = 1;
        caps_count += 1;
        return 0;
        break;
    case RELEASE_CAPSLOCK:
        caps_lock_press = 0;
        return 0;
        break;

    case PRESS_LEFT_SHIFT:
        shift_press = 1;
        return 0;
        break;

    case RELEASE_LEFT_SHIFT:
        shift_press = 0;
        return 0;
        break;

    case PRESS_RIGHT_SHIFT:
        shift_press = 1;
        return 0;
        break;

    case RELEASE_RIGHT_SHIFT:
        shift_press = 0;
        return 0;
        break;

    // For the function of tab, we assume it is same as space now.
    case PRESS_TAB:
        tab_press = 1;

        // auto complete current terminal
        tab_auto_complete();
        return 0;
        break;

    case RELEASE_TAB:
        tab_press = 0;
        return 0;
        break;
    
    // for the function of backspace, it will not go to the buffer, but change the position of the end of the meaningful buffer.
    case PRESS_BACKSPACE:
        if(keyboard_position!=0)
        {
            force_putc(BACKSPACE_EVAL);
            keyboard_position -= 1;
        }
        return 0;
        break;   

    case PRESS_ALT:
        alt_press = 1;
        return 0;
        break;

    case RELEASE_ALT:
        alt_press = 0;
        return 0;
        break;  

    // press enter is not viewed by special key.
    // case RELEASE_ENTER:
    //     *enter_press = 0;
    //     return 0;
    //     break;

    case PRESS_F1:
        if(alt_press)
        {
            terminal_switch(0);
        }
        return 0;
        break;

    case PRESS_F2:
        if(alt_press)
        {
            terminal_switch(1);
        }
        return 0;
        break;

    case PRESS_F3:
        if(alt_press)
        {
            
            terminal_switch(2);
        }
        return 0;
        break;

    case PRESS_F4:
        if(alt_press)
        {
            qemu_vga_show_picture(DESKTOP_IMAGE_WIDTH, DESKTOP_IMAGE_HEIGHT, QEMU_VGA_DEFAULT_BPP, (uint8_t*)DESKTOP_IMAGE_DATA);
        }
        return 0;
        break;

    // extra credit, for backtrace.
    case UP_ARROW:
        // adjust up arrow key
        if(history_key.buffer_index<=0)
        {
            return 0;
        }

        history_key.buffer_index -= 1;
        print_content(history_key.buffer_index);


        return 0;
        break;

    case DOWN_ARROW:
        // adjust down arrow key
        if(history_key.buffer_index<0)
        {
            return 0;
        }
        // update buffer index
        if(history_key.buffer_index>history_key.cur_history)
        {
            history_key.buffer_index = history_key.cur_history;
            return 0;
        }
 
        // update buffer index, increase the value
        history_key.buffer_index += 1; 
        print_content(history_key.buffer_index); 

        // detect whether maximum pointer = current pointer
        if(history_key.buffer_index == (history_key.cur_history))
        {
            //screen_x = BEGIN_OS_PLACE;
            // update_cursor(screen_x,screen_y);
            return 0;
        }
    
        return 0;
        break;
       
    default:
        return -1;
        break;
    }
}

/*
 * clear_enter_press
 *   DESCRIPTION: A helper function which can set the enter_press signal to 0.
 *   INPUTS: scancode_index of the spcial key. 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: set the signal we need to use.
 */
void clear_enter_press(){
    *enter_press = 0;
}

/*
 * get_enter_press
 *   DESCRIPTION: A helper function which can get the enter_press signal
 *   INPUTS: scancode_index of the spcial key. 
 *   OUTPUTS: none
 *   RETURN VALUE: the value of enter press.
 *   SIDE EFFECTS: get the signal we need to use.
 */
int get_enter_press(){
    return *enter_press;
}

void force_putc(unsigned char keyboard_value){
    int32_t cur_pid_owner_terminal = search_owner_terminal(cur_pid);
    uint32_t old_vidmem_base_addr;
    int32_t screen_x_backup, screen_y_backup, new_screen_x, new_screen_y;
    set_multi_process_vidmem(VIDMEM_SET_PHYSICAL, &old_vidmem_base_addr);
    if(cur_pid_owner_terminal != cur_terminal_id){
        get_screen_pos(&screen_x_backup, &screen_y_backup);
        set_screen_pos(terminal_list[cur_terminal_id].cursor_x, terminal_list[cur_terminal_id].cursor_y);
        force_update_cursor(terminal_list[cur_terminal_id].cursor_x, terminal_list[cur_terminal_id].cursor_y);
        putc_force(keyboard_value);
        get_screen_pos(&new_screen_x, &new_screen_y);
        terminal_list[cur_terminal_id].cursor_x = new_screen_x;
        terminal_list[cur_terminal_id].cursor_y = new_screen_y;
        force_update_cursor(new_screen_x, new_screen_y);
        set_screen_pos(screen_x_backup, screen_y_backup);
    }else{
        putc_force(keyboard_value);
    }
    set_multi_process_vidmem(VIDMEM_FORCE_MAPPING, &old_vidmem_base_addr);
}

// fill the buffer content auto.
void tab_auto_complete()
{
    int32_t count_read_file = 0;        // store the length of our information buffer
    int32_t read_keyboard_pos = 0;      // auto complete keyboard buffer pointer

    int32_t cmd_len = 0;                // command length
    int32_t place_flag = 0;             // mark as completed    

    uint32_t number_file_count = 0;     // mark to count the file index offset
    int32_t filename_len = 0;           // count the file name length

    int32_t cmd_pos = 0;                // position for command

    int32_t fill_pos = 0;               // position for the fill buffer

    int8_t current_inforamtion[MAX_FILENAME_LEN + 1]; //plus one for '\0'

    int8_t source_buffer[MAX_FILENAME_LEN+1];       // source buffer
    int8_t result_buffer[MAX_FILENAME_LEN+1];       // target buffer

    // false condition, it must be wrong input.
    if(keyboard_position >= MAX_FILENAME_LEN)
    {
        return;
    }



    // if not equal, continue to update it until equal
    // same as phrase function
    while(keyboard_position!=read_keyboard_pos)
    {
        // test the space split
        if(keyboard_buffer[read_keyboard_pos] == ' ')
        {
            read_keyboard_pos += 1;
            count_read_file = 0;
        }else{
            current_inforamtion[count_read_file] = keyboard_buffer[read_keyboard_pos];
            read_keyboard_pos += 1;
            count_read_file += 1;
        }
    }

    // check edge condition
    if(count_read_file > MAX_FILENAME_LEN)
    {
        return;
    }

    current_inforamtion[count_read_file] = '\0'; // fill the end char as 0.


    // read the file name
    
    // get source buffer content
    // search the file system using dir entry.
    while(1)
    {
        filename_len = dir_read(0, &number_file_count, source_buffer, MAX_FILENAME_LEN+1);
        if(filename_len !=0)
        {
            if(strncmp(source_buffer, current_inforamtion, count_read_file) == 0)
            {
                // if place flag is 1, refuse to run the auto complete
                if(place_flag)
                {
                    return;
                }
                strncpy(result_buffer, source_buffer, MAX_FILENAME_LEN+1);

                //open the flag
                place_flag = 1;

                //update command length
                cmd_len = filename_len;
            }
        }else{
            break;
        }
    }

    while(result_buffer[cmd_pos] == current_inforamtion[cmd_pos])
    {
        cmd_pos += 1;
    }

    // update fill position
    fill_pos = cmd_pos;

    // helper function, load the content into the keyboard buffer
    print_auto_complete(keyboard_buffer, result_buffer, fill_pos, cmd_pos, cmd_len);
    return;

}

// helper function for print the complete information then complete the keyboard buffer.
void print_auto_complete(char* keyboard_buffer, int8_t* result_buffer, int32_t fill_pos, int32_t cmd_pos, int32_t cmd_len)
{
    while(fill_pos < cmd_len)
    {
        force_putc((uint8_t)result_buffer[fill_pos]);

        keyboard_buffer[keyboard_position] = result_buffer[fill_pos];

        keyboard_position += 1;
        fill_pos += 1;

        if(keyboard_position == (MAX_BUFFER_SIZE-1))
        {
            return;
        }
    }
    return;
}

// initial for history buffer list
void init_history_list()
{
    // int j=0;
    // int k=0;
    //     //used for extra, history buffer
    // for(j=0;j<MAX_HISTORY_BUFFER;j++)
    // {
    //     for(k=0;k<MAX_BUFFER_SIZE;k++)
    //     {
    //         history_buffer_list[j].bt_buffer[k] = '\0';
    //     }
    // }

    history_key.buffer_index = 0;
    history_key.cur_history = 0;
    return;
}

// print history content
void print_content(int index)
{
    int32_t len = 0;
    int32_t i = 0;

    while(keyboard_position!=0)
    {
        force_putc(BACKSPACE_EVAL);
        keyboard_position-=1;
    }

    //copy the history components into current buffer
    keyboard_buffer = strncpy(keyboard_buffer, history_buffer_list[index].bt_buffer, MAX_BUFFER_SIZE);
    keyboard_position = strlen(keyboard_buffer);

    //refresh the cursor, jump to 391OS> 
    // screen_x = BEGIN_OS_PLACE;
    //update_cursor(screen_x,screen_y);

    //screen_x = BEGIN_OS_PLACE; //jump 3910S> 

    // print the history information
    len = strlen(history_buffer_list[index].bt_buffer);
    for(i=0;i<keyboard_position;i++)
    {
        force_putc(history_buffer_list[index].bt_buffer[i]);
    }
    return;

}

// force print value

