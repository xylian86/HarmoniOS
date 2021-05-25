#include "pit.h"
#include "i8259.h"
#include "../lib.h"
#include "../page.h"
#include "../process_crtl.h"
#include "../signal.h"
// int32_t counter = 0;

/* pit_init
 *   DESCRIPTION: Initialize PIT
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Set PIT with irq #0
 * 
 * ref: https://wiki.osdev.org/Programmable_Interval_Timer
 * ref: http://www.osdever.net/bkerndev/Docs/pit.htm
 */

void pit_init(void) {  
    /* set the mode to 3 via command  port */
    outb(PIT_MODE_3, PIT_CMD_PORT);

    /* sent lower 8 bits */
    outb(PIT_FREQ_100HZ & 0x00ff, PIT_CHANNEL_0);

    /* sent higher 8 bits */
    outb(PIT_FREQ_100HZ >> 8, PIT_CHANNEL_0);
    
    enable_irq(PIT_IRQ);
}


/* pit_handler
 *   DESCRIPTION: Handle PIT interrupt and call Scheduler
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: call Scheduler
 */
void pit_handler(void){
    send_eoi(PIT_IRQ);
    // counter = counter + 1;
    // if (counter == 100) {
    //     printf("test pit \n");
    //     counter = 0;
    // }
    // TODO: Scheduler
    process_crtl_block_t* pcb = get_pcb(cur_pid);
    pcb->alarm_time += 1;
    if(pcb->alarm_time == 1000){
        signal_raise(3);
        pcb->alarm_time = 0;
    }
    if (cur_pid==NULL_PROCESS)
        return;
    process_switch();
}


