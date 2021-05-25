#include "rtc.h"
#include "i8259.h"

#include "../tests.h"
#include "../lib.h"
#include "../process_crtl.h"
#include "../status_bar.h"
#include "../page.h"
#include "../cursor_graphic.h"

volatile int32_t rtc_count[3] = {1, 1, 1};   // initialize the rtc_count
volatile int32_t read_flag[3] = {0, 0, 0};
volatile int32_t max_count = 1;
volatile int32_t rtc_count_date = 1;
volatile int32_t read_flag_data = 0;
// volatile int32_t rtc_use[3] = {0, 0, 0};
int32_t rtc_beep = 1;

int32_t cursor_update = 1024;

void own_test_interrupts(); // define the our own rtc_test function
int32_t freq_ref(int32_t freq);

/*
 * freq_ref
 *   DESCRIPTION: get target freq
 *   INPUTS: freq
 *   OUTPUTS: None
 *   RETURN VALUE: freq value for rtc chip
 */
int32_t freq_ref(int32_t freq){
    switch (freq){
        case 2:     return 512;
        case 4:     return 256;
        case 8:     return 128;
        case 16:    return 64;
        case 32:    return 32;
        case 64:    return 16;
        case 128:   return 8;
        case 256:   return 4;
        case 512:   return 2;
        case 1024:  return 1;
        default:    return -1;
    }
}
/*
 * rtc_init
 *   DESCRIPTION: Initialize the RTC interrupt. The entry is 8.
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: 0 for successful
 *   SIDE EFFECTS: Enable the interrupt request from RTC
 */
int32_t rtc_init(){
    // refer to https://wiki.osdev.org/RTC
    char prev;
    outb(RTC_SR_B, RTC_IO);		    // select register B, and disable NMI
    prev = inb(CMOS_IO);	        // read the current value of register B
    outb(RTC_SR_B, RTC_IO);		    // set the index again (a read will reset the index to register D)
    outb(prev | SIXTH_BIT, CMOS_IO);	    // write the previous value ORed with 0x40. This turns on bit 6 of register B


    outb(RTC_SR_A, RTC_IO);                 // set index to register A, disable NMI
    prev = inb(CMOS_IO);                        // get initial value of register A
    outb(RTC_SR_A, RTC_IO);                 // reset index to A
    outb((prev & LOWER_MASK) | RATE_SET, CMOS_IO);    //write only our rate to A. Note, rate is the bottom 4 bits.

    enable_irq(IRQ_RTC);                    // IRQ number of RTC is 8
    return 0;
}

/*
 * rtc_open
 *   DESCRIPTION: set the RTC interrupt to 2HZ, which corresponding to 512 counts.
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: 0 for successful
 *   SIDE EFFECTS: Enable the interrupt frequence to 2HZ
 */
int32_t rtc_open(){
    rtc_count[cur_terminal_id] = MACCOUNT;
    // rtc_use[cur_terminal_id] = 1;
    return 0;
}

/*
 * rtc_open_intf
 *   DESCRIPTION: general wrapper function for uniform interface
 *   INPUTS: filename
 *   OUTPUTS: None
 *   RETURN VALUE: follow function inside wrapper
 */
int32_t rtc_open_intf(const uint8_t* filename){
    return rtc_open();
}

/*
 * rtc_close
 *   DESCRIPTION: close rtc
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: 0 for successful
 *   SIDE EFFECTS: 
 */
int32_t rtc_close(){
    return 0;
}

/*
 * rtc_close_intf
 *   DESCRIPTION: general wrapper function for uniform interface
 *   INPUTS: USELESS
 *   OUTPUTS: None
 *   RETURN VALUE: follow function inside wrapper
 */
int32_t rtc_close_intf(int32_t* dummy){
    return rtc_close();
}

/*
 * rtc_open
 *   DESCRIPTION: wait for one rtc period, similiar to sleep()
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: 0 for successful
 *   SIDE EFFECTS: sleep()
 */
int32_t rtc_read(){
    int32_t save_cur_terminal_id = search_owner_terminal(cur_pid); //cur_terminal_id;
    read_flag[save_cur_terminal_id] = 0;
    while(read_flag[save_cur_terminal_id] == 0){}
    return 0;
}

/*
 * rtc_read_intf
 *   DESCRIPTION: general wrapper function for uniform interface
 *   INPUTS: USELESS
 *   OUTPUTS: None
 *   RETURN VALUE: follow function inside wrapper
 */
int32_t rtc_read_intf(int32_t fd, uint32_t* offset, void* buf, int32_t nbytes){
    return rtc_read();
}

/*
 * rtc_write
 *   DESCRIPTION: set the rtc frequence to the required one
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: 0 for successful
 *   SIDE EFFECTS: set the max_count
 */
int32_t rtc_write(int32_t freq){
    if(freq_ref(freq) == -1){
        return -1;
    }
    else{
        max_count = freq_ref(freq);
        rtc_count[cur_terminal_id] = max_count;
    }
    return 0;
}

/*
 * rtc_write_intf
 *   DESCRIPTION: general wrapper function for uniform interface
 *   INPUTS: fd - USELESS
 *           buf - contain 4 bytes for one int
 *           nbytes - must be 4
 *   OUTPUTS: None
 *   RETURN VALUE: follow function inside wrapper, or return -1 on failure
 */
int32_t rtc_write_intf(int32_t fd, const void* buf, int32_t nbytes){
    if (nbytes!=4 || buf==NULL)
        return -1;
    int32_t * ptr = (int32_t *)buf;
    return rtc_write(*ptr);
}
/*
 * rtc_interrupt (for cp1)
 *   DESCRIPTION: rtc interrupt handler
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: can test the rtc interrupt
 */
void rtc_interrupt(){
    send_eoi(IRQ_RTC);          // send end-of-interrupt signal
    outb(RTC_SR_C, RTC_IO);     // select registers C
    inb(CMOS_IO);               // discard value
 
    int32_t i = 0;
    rtc_beep++;
    cursor_update--;

    int screen_x;
    int screen_y;
    if((cursor_update/512)==0)
    {
        get_screen_pos(&screen_x, &screen_y);
        graphic_cursor_clear(screen_x, screen_y);
    }

    if((cursor_update/512)!=0)
    {
        get_screen_pos(&screen_x, &screen_y);
        graphic_cursor_update(screen_x, screen_y);
    }

    if(cursor_update==0)
    {
        cursor_update=1024;
    }

    //delete the i
    for(; i<=2; i++){
        rtc_count[i] --;
        if(rtc_count[i] == 0){   
        rtc_count[i] = max_count;
        read_flag[i] = 1;
        update_multi_process_vidmem(i);
        //clock_update_for_sb();
        update_multi_process_vidmem(search_owner_terminal(cur_pid));

    }
    
    rtc_count_date --;
    if(rtc_count_date == 0){
        rtc_count_date = RTC_DATA_COUNT;
        clock_update_for_sb();
    }
    // int32_t save_cur_terminal_id = search_owner_terminal(cur_pid);
    // rtc_count[save_cur_terminal_id] --;
    // if(rtc_count[save_cur_terminal_id] == 0){   
    // rtc_count[save_cur_terminal_id] = max_count;
    // read_flag[save_cur_terminal_id] = 1; 
    // #if(RTC_ENABLE_PRINT!=0)
    //     own_test_interrupts();      // use our own test
    //     // test_interrupts();       // you can also use the test provided in lib.c
    // #endif  
    }

}

/*
 * own_test_interrupts (for cp1)
 *   DESCRIPTION: test the rtc
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: can print the sentence one by one and repeat
 */
// void own_test_interrupts(){
//     char print_words[RTC_MESSAGE_LEN] = "The RTC interrupt works well.\n";
//     rtc_count --;
//     static int i = 0;
//     if(rtc_count == -1){        // we want to print the character one by one
//         rtc_count = MACCOUNT;
//         printf("%c",print_words[i]);
//         i ++;
//         if(i == RTC_MESSAGE_LEN){            // the length of the sentence is 30
//             i = 0;
//         }
//     }
// }

