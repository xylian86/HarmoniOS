#ifndef ASM_LINKAGE_H
#define ASM_LINKAGE_H

#ifndef ASM
    #include "types.h"
    #include "do_syscall.h"
    #include "idt.h"
    // Function head for assembly linkage functions used in interrupt
    extern void keyboard_interrupt_savereg();   // work for keyboard interrupt
    extern void rtc_interrupt_savereg();        // work for rtc interrupt
    extern void pit_interrupt_savereg();        // work for pit interrupt
    extern void mouse_interrupt_savereg();      // work for mouse interrupt
    extern void syscall_dispatch();
    extern void jump_to_execute_return(uint32_t status, int32_t parent_esp, int32_t parent_ebp);
    extern void jump_to_next_process(uint32_t next_esp, uint32_t next_ebp);

    extern void Divide_Error();
    extern void Debug_handler();
    extern void NMI_Interrupt();
    extern void Breakpoint();
    extern void Overflow();
    extern void BOUND_Range_Exceeded();
    extern void Invalid_Opcode();
    extern void Device_Not_Available();
    extern void Double_Fault();
    extern void Coprocessor_Segment_Overrun();
    extern void Invalid_TSS();
    extern void Segment_Not_Present();
    extern void Stack_Segment_Fault();
    extern void General_Protection();
    extern void Page_Fault();
    extern void Reserved_Exception();
    extern void FPU_Floating_Point_Exception();
    extern void Alignment_Check();
    extern void Machine_Check();
    extern void SIMD_loating_Point();
#endif /*ASM*/

#endif
