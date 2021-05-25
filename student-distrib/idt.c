#include "x86_desc.h"
#include "idt.h"
#include "lib.h"
#include "devices/rtc.h"
#include "devices/keyboard.h"
#include "tests.h"
#include "asm_linkage.h"
#include "do_syscall.h"
#include "types.h"
#include "signal.h"

#define SIZE_OF_EXCEPTION  20      //Length of the exception table
#define PIC_LOWER 0x20              // 0x20 is the idt start for PIC
#define PIC_UPPER 0x2F              // 0x2F is the idt end for PIC


#define RTC_VEC      0x28           // RTC vector index
#define KEYBOARD_VEC 0x21           // Keyboard vector index
#define PIT_VEC      0x20           // PIT vector index
#define MOUSE_VEC    0x2c           // Mouse vector index

#define SYSTEM_CALL_VEC 0x80        // System call vector index

#define NMI_VEC         0x02        //NMI interrupt vector index

//handler function

//global variable for halt
int32_t halt_flag = 0;
//exception string test
char* EXCEPTIONS[SIZE_OF_EXCEPTION]={ 
    "Division by Zero Exception Handler",
    "Debug Exception Handler",
    "The Nonmaskable Interrupt Handler",
    "Breakpoint Exception Handler",
    "Overflow Exception Handler",
    "Bound Range Exceeded Exception Handler",
    "Invalid Opcode Exception Handler",
    "Device Not Available Exception Handler",
    "Double Fault Exception Handler",
    "Coprocessor Segment Overrun Exception Handler",
    "Invalid TSS Exception Handler",
    "Segment Not Present Exception Handler",
    "Stack Fault Exception Handler",
    "General Protection Exception Handler",
    "Page-Fault Exception Handler",
    "Reserved Exception Handler",
    "x87 FPU Floating-Point Exception Handler",
    "Alignment Check Exception Handler",
    "Machine-Check Exception Handler",
    "SIMD Floating-Point Exception Handler"
};


/*
 * init_idt
 *   DESCRIPTION: Initialize the IDT array.
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Set initialization for IDT.
 * 
 *   For trap table/interrupt table format, we refer I32A_manual 5-14 vol 3.
 */

void init_idt()
{
    int i;
    for(i=0; i<NUM_VEC; i++)
    {
        idt[i].seg_selector = KERNEL_CS;            // set segementation selector value as kernel_CS 
        idt[i].reserved4 = 0;                       // set reserved4 value as 0
        idt[i].reserved3 = 1;                       // We initilize all tables as trap tables, so we set reserved3 as 0.

        // idt[i].reserved3 = ((i >= PIC_LOWER) && (i <= PIC_UPPER)) ? 0 : 1;

        idt[i].reserved2 = 1;                       // set reserved2 value as 1
        idt[i].reserved1 = 1;                       // set reserved1 value as 1
        idt[i].size = 1;                            // set size value as 1                  
        idt[i].reserved0 = 0;                       // set reserved0 value as 1

        idt[i].dpl = (i==SYSTEM_CALL_VEC) ? 3 : 0;  //Pay attention to system call, we need to change DPL as 3 for system call operation

        idt[i].present = 0;                         //set present value as 0
    }

   //Set idt entry for exception
    SET_IDT_ENTRY(idt[0],Divide_Error);
    SET_IDT_ENTRY(idt[1],Debug_handler);
    SET_IDT_ENTRY(idt[2],NMI_Interrupt);
    SET_IDT_ENTRY(idt[3],Breakpoint);
    SET_IDT_ENTRY(idt[4],Overflow);
    SET_IDT_ENTRY(idt[5],BOUND_Range_Exceeded);
    SET_IDT_ENTRY(idt[6],Invalid_Opcode);
    SET_IDT_ENTRY(idt[7],Device_Not_Available);
    SET_IDT_ENTRY(idt[8],Double_Fault);
    SET_IDT_ENTRY(idt[9],Coprocessor_Segment_Overrun);
    SET_IDT_ENTRY(idt[10],Invalid_TSS);
    SET_IDT_ENTRY(idt[11],Segment_Not_Present);
    SET_IDT_ENTRY(idt[12],Stack_Segment_Fault);
    SET_IDT_ENTRY(idt[13],General_Protection);
    SET_IDT_ENTRY(idt[14],Page_Fault);
    SET_IDT_ENTRY(idt[15],Reserved_Exception);
    SET_IDT_ENTRY(idt[16],FPU_Floating_Point_Exception);
    SET_IDT_ENTRY(idt[17],Alignment_Check);
    SET_IDT_ENTRY(idt[18],Machine_Check);
    SET_IDT_ENTRY(idt[19],SIMD_loating_Point);

    // keyboard+rtc
    SET_IDT_ENTRY(idt[RTC_VEC],rtc_interrupt_savereg);
    idt[RTC_VEC].reserved3 = 0;

    SET_IDT_ENTRY(idt[KEYBOARD_VEC], keyboard_interrupt_savereg);
    idt[KEYBOARD_VEC].reserved3 = 0;

    // pit
    SET_IDT_ENTRY(idt[PIT_VEC], pit_interrupt_savereg);
    idt[PIT_VEC].reserved3 = 0;

    // mouse
    SET_IDT_ENTRY(idt[MOUSE_VEC], mouse_interrupt_savereg);
    idt[MOUSE_VEC].reserved3 = 0;

    // system call 
    SET_IDT_ENTRY(idt[SYSTEM_CALL_VEC], syscall_dispatch);
    // idt[SYSTEM_CALL_VEC].reserved3 = 0;

}

void excep_signal_raise(sig_regs r){
    if(r.num == 0){
        signal_raise(0);
    }
    else{
        signal_raise(1);
    }
}


