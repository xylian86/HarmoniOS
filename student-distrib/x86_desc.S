# x86_desc.S - Set up x86 segment descriptors, descriptor tables
# vim:ts=4 noexpandtab

#define ASM     1
#include "x86_desc.h"

.text

.globl ldt_size, tss_size
.globl gdt_desc, ldt_desc, tss_desc
.globl tss, tss_desc_ptr, ldt, ldt_desc_ptr
.globl gdt_ptr
.globl idt_desc_ptr, idt
.globl page_directory, page_table, user_page_4K

# how to define a table in x86?
# table:
# content
# table_bottom :
# the code structure above defines a table
# the size can be calculated by table_bottom - table + 1

# MEMORY SEGMENTATION SUPPORT BELOW

# .align integer, causes the next data generated to be aligned modulo integer bytes
.align 4
tss_size:
    .long tss_bottom - tss - 1

ldt_size:
    .long ldt_bottom - ldt - 1

    .word 0 # Padding

# declaration: uint16_t ldt_desc;
ldt_desc:
    .word KERNEL_LDT
    .long ldt

# TASK STATE SEGMENT
# it contains all the information for a program's running state
    .align 4
tss:
_tss:
    .rept 104
    .byte 0
    .endr
tss_bottom:

# GLOBAL DESCRIPTOR TABLE
    .align  16
gdt:
_gdt:

    # First GDT entry cannot be used
    .quad 0

    # NULL entry
    .quad 0

    # Segmentation will not be used
    # CS and DS both are 0-4GB r/w segments
    #
    # The layout is (from Intel IA-32 reference manual):
    #  31        24 23  22  21  20  19   16 15  14 13 12  11   8 7          0
    # |----------------------------------------------------------------------|
    # |            |   | D |   | A |  Seg  |   |  D  |   |      |            |
    # | Base 31:24 | G | / | 0 | V | Limit | P |  P  | S | Type | Base 23:16 |
    # |            |   | B |   | L | 19:16 |   |  L  |   |      |            |
    # |----------------------------------------------------------------------|
    #
    # |----------------------------------------------------------------------|
    # |                                    |                                 |
    # | Base 15:0                          | Segment Limit 15:0              |
    # |                                    |                                 |
    # |----------------------------------------------------------------------|

gdt_ptr:
    # Set up an entry for kernel CS
    .quad 0x00CF9A000000FFFF

    # Set up an entry for kernel DS
    .quad 0x00CF92000000FFFF

    # Set up an entry for user CS
    .quad 0x00CFFA000000FFFF

    # Set up an entry for user DS
    .quad 0x00CFF2000000FFFF

    # Set up an entry for TSS

# Note that we maintain two desc. pointers in the GDT
# they should point to the tss and ldt which has been 
# defined in this file with space allocated!

# declaration: seg_desc_t tss_desc_ptr;
tss_desc_ptr:
    .quad 0

    # Set up one LDT
# decalration: seg_desc_t ldt_desc_ptr;
ldt_desc_ptr:
    .quad 0

gdt_bottom:

# gdt descriptor, should be a struct with addr and size
    .align 4
    .word 0 # Padding
gdt_desc:
    .word gdt_bottom - gdt - 1
    .long gdt


# LOCAL DESCRIPTOR TABLE
# the table size should be 32bytes
# .rept 4 - code - .endr
# the instruction above refers to repeating the code
# by four times, we use such code here to assign space
# in memory
    .align  16
ldt:
    .rept 4
    .quad 0
    .endr
ldt_bottom:


# INTERRUPT SUPPORT BELOW

# x86_desc_t idt_desc_ptr
# this desc_ptr has both ptr to the idt and its size
# the size is caulculated by bottom-top-1, here the
# size actually means max_index according to Intel
# the code below initialize the struct x86_desc_ptr
.align 4
    .word 0 # Padding
idt_desc_ptr:
    .word idt_bottom - idt - 1
    .long idt

# INTERRUPT DESCRIPTOR TABLE
    .align  16
idt:
_idt:
    .rept NUM_VEC
    .quad 0
    .endr

idt_bottom:


# allocate space for paging 
# head page table
# always make the page table align by 4K
    .align  4096 
page_directory:
_page_directory:
    .rept 1024
    .long 0
    .endr
page_directory_bottom:

# page table
    .align  4096
page_table:
_page_table:
    .rept 1024
    .long 0
    .endr 
page_table_bottom:

    .align 4096
user_page_4K:
_user_page_4K:
    .rept 1024
    .long 0
    .endr
user_page_4K_bottom:
