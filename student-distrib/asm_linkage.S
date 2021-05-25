#define ASM     1
#include "asm_linkage.h"

.globl keyboard_interrupt_savereg
.globl rtc_interrupt_savereg
.globl pit_interrupt_savereg
.globl mouse_interrupt_savereg
.globl syscall_dispatch, system_call_invalid, system_call_finish, syscall_jump_table
.globl jump_to_execute_return
.globl jump_to_next_process
.globl exception_signal_handler
.globl Divide_Error, Debug_handler, NMI_Interrupt, Breakpoint, Overflow, BOUND_Range_Exceeded, Invalid_Opcode, Device_Not_Available, Double_Fault, Coprocessor_Segment_Overrun, Invalid_TSS, Segment_Not_Present, Stack_Segment_Fault, General_Protection, Page_Fault, Reserved_Exception, FPU_Floating_Point_Exception, Alignment_Check, Machine_Check, SIMD_loating_Point
.globl sigreturn_fuc
.text
/*
    Interrupt invoation part, referred from lecture 12.
    Steps:
    1. push all registers 
    2. push flag 
    3. call interrupt handler 
    4. pop flage 
    5. pop all register values 
    6. return 
*/

/*
    keyboard invocation
*/

exception_signal_handler:
    # movl  4(%esp), %ecx
    # pushl %ecx
    push %fs
    push %es
    push %ds

    pushl %eax
    pushl %ebp
    pushl %edi
    pushl %esi 
    pushl %edx # 3rd argument 
    pushl %ecx # 2nd argument 
    pushl %ebx # 1st argument
    call excep_signal_raise
    call sig_handler_func
    popl %ebx 
    popl %ecx 
    popl %edx 

    # pop all registers
    popl %esi
    popl %edi 
    popl %ebp 
    popl %eax
    pop  %ds
    pop  %es 
    pop  %fs 
    addl $8, %esp
    iret
    

keyboard_interrupt_savereg:
    pushl $0
    pushl $33
    # pushal                          # push all registers
    # pushfl                          # push flag
    push %fs
    push %es
    push %ds

    pushl %eax
    pushl %ebp
    pushl %edi
    pushl %esi 
    pushl %edx # 3rd argument 
    pushl %ecx # 2nd argument 
    pushl %ebx # 1st argument
    call keyboard_handler
    call sig_handler_func
    # popfl                           # pop flag
    # popal                           # pop all registers
    popl %ebx 
    popl %ecx 
    popl %edx 

    # pop all registers
    popl %esi
    popl %edi 
    popl %ebp 
    popl %eax
    pop  %ds
    pop  %es 
    pop  %fs 
    addl $8, %esp
    iret

/*
    rtc invocation
*/
rtc_interrupt_savereg:
    pushl $0
    pushl $41
    # pushal                          # push all registers
    # pushfl                          # push flag
    push %fs
    push %es
    push %ds

    pushl %eax
    pushl %ebp
    pushl %edi
    pushl %esi 
    pushl %edx # 3rd argument 
    pushl %ecx # 2nd argument 
    pushl %ebx # 1st argument
    call rtc_interrupt
    call sig_handler_func
    # popfl                           # pop flag
    # popal                           # pop all registers
    popl %ebx 
    popl %ecx 
    popl %edx 

    # pop all registers
    popl %esi
    popl %edi 
    popl %ebp 
    popl %eax
    pop  %ds
    pop  %es 
    pop  %fs 
    addl $8, %esp
    iret

/*
    pit invocation
*/
pit_interrupt_savereg:
    pushl $0
    pushl $32
    # pushal                          # push all registers
    # pushfl                          # push flag
    push %fs
    push %es
    push %ds

    pushl %eax
    pushl %ebp
    pushl %edi
    pushl %esi 
    pushl %edx # 3rd argument 
    pushl %ecx # 2nd argument 
    pushl %ebx # 1st argument
    call pit_handler
    call sig_handler_func
    # popfl                           # pop flag
    popl %ebx 
    popl %ecx 
    popl %edx 

    # pop all registers
    popl %esi
    popl %edi 
    popl %ebp 
    popl %eax
    pop  %ds
    pop  %es 
    pop  %fs 
    addl $8, %esp
    iret

/*
    mouse invocation
*/
mouse_interrupt_savereg:
    pushal                          # push all registers
    pushfl                          # push flag
    call mouse_handler
    popfl                           # pop flag
    popal                           # pop all registers
    iret

/*
    syscall_dispatch part, referred from lecture 19.
    Steps:
    1. push normal registers with flag register 
    2. push local variable 
    3. call system call function 
    4. pop local variable 
    5. pop normal registers with flag register  
    6. ireturn  
*/ 
syscall_dispatch:
    # save all registers
    pushl $0
    pushl $0x80
    push %fs
    push %es
    push %ds

    pushl %eax
    pushl %ebp
    pushl %edi
    pushl %esi 

    # save input parameters
    # pay attention to the sequence
    # caller side: func(arg1, arg2, arg3)
    # user space syscall wrapper: put arg3 in %edx, arg2 in %ecx and arg1 in %ebx
    pushl %edx # 3rd argument 
    pushl %ecx # 2nd argument 
    pushl %ebx # 1st argument
    # eax is the id of syscall function which user wants to perform
    # make sure the command is valid
    cmpl $1, %eax 
    jl system_call_invalid 
    cmpl $14, %eax 
    jg system_call_invalid 

    # call the function in jump table
    call *syscall_jump_table(,%eax,4)
    movl %eax, 24(%esp)
    call sig_handler_func
    # system call finish 
    jmp system_call_finish 

system_call_invalid:
    xorl %eax, %eax 
    subl $1, %eax 

system_call_finish:
    popl %ebx 
    popl %ecx 
    popl %edx 

    # pop all registers
    popl %esi
    popl %edi 
    popl %ebp 
    popl %eax
    pop  %ds
    pop  %es 
    pop  %fs 
    addl $8, %esp

    # fake return to user mode
    iret 

# jump table for system call
syscall_jump_table:    
    .long   0x0
    .long   halt
    .long   execute
    .long   read
    .long   write
    .long   open
    .long   close
    .long   getargs
    .long   vidmap
    .long   set_handler
    .long   sigreturn_fuc
    .long   new_poke
    .long   beep
    .long   ps
    .long   random

# void jump_to_execute_return(uint32_t status, int32_t parent_esp, int32_t parent_ebp);
jump_to_execute_return:
    pushl %ebp
    movl %esp, %ebp
    # use caller save regs to contain params
    movl 8(%ebp), %eax  # status
    movl 12(%ebp), %ecx # parent_esp
    movl 16(%ebp), %edx # parent_ebp
    movl %edx, %ebp
    movl %ecx, %esp
    leave
    ret

jump_to_next_process:
    pushl %ebp
    movl %esp, %ebp

    movl 8(%ebp), %eax
    movl 12(%ebp), %edx
    movl %eax, %esp
    movl %edx, %ebp
    leave
    ret

sigreturn:
    movl    64(%esp), %esi
    addl    $4, %esi
    movl    %esp, %edi
    addl    $4, %edi

    xorl    %ecx, %ecx
    HW_loop:
        movl    (%esi, %ecx, 4), %eax
        movl    %eax, (%edi, %ecx, 4)
        incl    %ecx
        cmpl    $17, %ecx
        jne     HW_loop
    
    movl    24(%edi), %eax

    ret

Divide_Error:
    pushl $0
    pushl $0
    jmp exception_signal_handler    
Debug_handler:
    pushl $0
    pushl $1
    jmp exception_signal_handler
NMI_Interrupt:
    pushl $0
    pushl $2
    jmp exception_signal_handler
Breakpoint:
    pushl $0
    pushl $3
    jmp exception_signal_handler
Overflow:
    pushl $0
    pushl $4
    jmp exception_signal_handler
BOUND_Range_Exceeded:
    pushl $0
    pushl $5
    jmp exception_signal_handler
Invalid_Opcode:
    pushl $0
    pushl $6
    jmp exception_signal_handler
Device_Not_Available:
    pushl $0
    pushl $7
    jmp exception_signal_handler
Double_Fault:
    pushl $8
    jmp exception_signal_handler
Coprocessor_Segment_Overrun:
    pushl $0
    pushl $9
    jmp exception_signal_handler
Invalid_TSS:
    pushl $10
    jmp exception_signal_handler
Segment_Not_Present:
    pushl $11
    jmp exception_signal_handler
Stack_Segment_Fault:
    pushl $12
    jmp exception_signal_handler
General_Protection:
    pushl $13
    jmp exception_signal_handler
Page_Fault:
    pushl $14
    jmp exception_signal_handler
Reserved_Exception:
    pushl $0
    pushl $15
    jmp exception_signal_handler
FPU_Floating_Point_Exception:
    pushl $0
    pushl $16
    jmp exception_signal_handler
Alignment_Check:
    pushl $17
    jmp exception_signal_handler
Machine_Check:
    pushl $0
    pushl $18
    jmp exception_signal_handler
SIMD_loating_Point:
    pushl $0
    pushl $19
    jmp exception_signal_handler

 
