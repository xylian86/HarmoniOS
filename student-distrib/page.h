#ifndef _PAGE_H
#define _PAGE_H

#include "types.h"

#define KERNEL_PAGE_BEGIN 0x400000
#define VIDEO_MEM_BEGIN 0x0B8000 
#define VIDEO_MEM_END   0x0B9000
#define PTE_BASE_MASK   0x003FF000
#define BG_BUF_NUM      3
#define PAGE_SIZE_4K    0x1000

#define VGA_MEMORY      0x1000000
#define OFFSET_22       22

void init_paging();

void set_user_pt(uint32_t pid);

void flush_tlb();

void setup_user_vidmem(uint8_t * vmem);

void disable_user_vidmem(uint8_t * vmem);

#define VIDMEM_SET_PHYSICAL     1
#define VIDMEM_FORCE_MAPPING    2
void set_multi_process_vidmem(int32_t flags, uint32_t *arg);

void update_multi_process_vidmem(int32_t terminal_id);

void setup_user_vidmem_for_switch(uint8_t * vmem, uint32_t terminal_offset);

void close_user_vidmem_for_switch(uint8_t * vmem, uint32_t terminal_offset);

void close_user_vidmem(uint8_t * vmem);
#endif
