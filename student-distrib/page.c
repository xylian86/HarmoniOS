#include "page.h"
#include "x86_desc.h"
#include "process_crtl.h"
#include "terminal.h"
#include "do_syscall.h"
#include "vga_design.h"
// #define KERNEL_PD_IDX       KERNEL_PAGE_BEGIN>>22
// #define VID_PD_IDX          VIDEO_MEM_BEGIN>>22
// #define VID_PT_IDX_BEGIN    (VIDEO_MEM_BEGIN & PTE_BASE_MASK)>>12
// #define VID_PT_IDX_END      (VIDEO_MEM_END & PTE_BASE_MASK)>>12

#define KERNEL_PD_IDX       1
#define VID_PD_IDX          0
#define VID_PT_IDX_BEGIN    0xB8
#define VID_PT_IDX_END      0xB9

static void enable_paging();

/*
 * init_paging
 *   DESCRIPTION: initialize the page directory and page table
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: fill in the bit fields of kernel page and video mem
 */
void init_paging(){
    int i;

    // uint32_t check_point0, check_point1, check_point2, check_point3;
    for (i=0; i<PAGE_ENTRY_NUM; i++){
        // disable the pages
        page_directory[i].val = 0;
        page_table[i].val = 0;

        if((i>=(((uint32_t) qemu_vga_addr>>OFFSET_22))&&(i<(((uint32_t)qemu_vga_addr+VGA_MEMORY)>>OFFSET_22)))){
            page_directory[i].MB.present = 1;
            page_directory[i].MB.page_size = 1;
            page_directory[i].MB.base_addr = i;
            page_directory[i].MB.read_write = 1;
            page_directory[i].MB.page_cached = 1;            
            page_directory[i].MB.global = 1; 
            page_directory[i].MB.usr_or_supervisor = 0;
        }

    }
    // set present 1 for kernel
    page_directory[VID_PD_IDX].KB.present = 1;
    page_directory[KERNEL_PD_IDX].MB.present = 1;
    // r/w
    page_directory[VID_PD_IDX].KB.read_write = 1;
    page_directory[KERNEL_PD_IDX].MB.read_write = 1;
    // u/s
    page_directory[VID_PD_IDX].KB.usr_or_supervisor = 0;
    page_directory[KERNEL_PD_IDX].MB.usr_or_supervisor = 0;
    // page size
    page_directory[VID_PD_IDX].KB.page_size = 0;
    page_directory[KERNEL_PD_IDX].MB.page_size = 1;
    // global
    page_directory[VID_PD_IDX].KB.global = 0;
    page_directory[KERNEL_PD_IDX].MB.global = 1;
    // cache
    page_directory[VID_PD_IDX].KB.page_cached = 0;
    page_directory[KERNEL_PD_IDX].MB.page_cached = 1;


    // set base addr
    // check_point0 = (uint32_t)page_directory;
    // check_point1 = (uint32_t)page_table;
    // check_point2 = (uint32_t)page_table>>22;
    // check_point3 = KERNEL_PD_IDX;
    page_directory[VID_PD_IDX].KB.base_addr = (uint32_t)page_table>>12;
    page_directory[KERNEL_PD_IDX].MB.base_addr = KERNEL_PD_IDX;
    // set page tables 
    for (i=VID_PT_IDX_BEGIN; i<VID_PT_IDX_END+BG_BUF_NUM; i++){
        page_table[i].present = 1;
        page_table[i].read_write = 1;
        page_table[i].usr_or_supervisor = 0;
        page_table[i].base_addr = i;
    }

    enable_paging();
}

/*
 * enable_paging
 *   DESCRIPTION: Change the control registers, especially the PDBR
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: tell the CPU about the paging stuff
 * 
 * ref: https://wiki.osdev.org/Paging
 */
static void enable_paging(){                                      
    asm volatile(                           
        "movl %0, %%eax \n\t"  
        "movl %%eax, %%cr3 \n\t"
        "xorl %%eax, %%eax \n\t"
        "movl %%cr4, %%eax \n\t"
        "orl $0x00000010, %%eax \n\t"
        "movl %%eax, %%cr4 \n\t"
        "xorl %%eax, %%eax \n\t"
        "movl %%cr0, %%eax \n\t"
        "orl $0x80000000, %%eax \n\t"
        "movl %%eax, %%cr0 \n\t"

        : :"r"(page_directory) : "memory", "%eax"
    );
}

/* set_user_pt
 *   DESCRIPTION: set up page table (4MB) for user program          
 *   INPUTS: pid - process id
 *   OUTPUTS: none
 *   RETURN VALUE: none
*/
void set_user_pt(uint32_t pid) {
    /* determine the index of user memory in the paging directory */
    static uint32_t user_idx = USER_MEMORY >> OFFSET_4MB;  

    /* set the pagin parameters */
    page_directory[user_idx].MB.present = 1;
    page_directory[user_idx].MB.read_write = 1;
    page_directory[user_idx].MB.usr_or_supervisor = 1;
    page_directory[user_idx].MB.page_write_though = 0;
    page_directory[user_idx].MB.page_cached = 0;
    page_directory[user_idx].MB.accessed = 0;
    page_directory[user_idx].MB.dirty = 0;
    page_directory[user_idx].MB.page_size = 1;
    page_directory[user_idx].MB.global = 0;
    page_directory[user_idx].MB.avail = 0;
    page_directory[user_idx].MB.page_attr_table = 0;
    page_directory[user_idx].MB.reserved = 0;
    /* physical address, start from 8MB */
    page_directory[user_idx].MB.base_addr = (KERNEL_PAGE_END >> OFFSET_4MB) + pid;

    /* flush TLB */
    flush_tlb();
}


/*
 * flush_tlb
 *   DESCRIPTION: flush TLB when changing process
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   REF: https://wiki.osdev.org/TLB
 */
void flush_tlb() {
	asm volatile (
		"movl %%cr3, %%eax;"
		"movl %%eax, %%cr3;"
		: : : "%eax"
    );
    return;
}

/*
 * setup_user_vidmem
 *   DESCRIPTION: set up the paging(user page table) for a virtual address space of video
 *   INPUTS: the virtual address for the screen data
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void setup_user_vidmem(uint8_t * vmem){
    uint32_t pd_idx = (uint32_t)vmem>>22;
    uint32_t pt_idx = ((uint32_t)vmem & PTE_BASE_MASK)>>12;
    // clear page directory entry
    page_directory[pd_idx].val = 0;
    // customized settings
    page_directory[pd_idx].KB.page_size = 0;
    page_directory[pd_idx].KB.usr_or_supervisor = 1;
    page_directory[pd_idx].KB.read_write = 1;
    page_directory[pd_idx].KB.base_addr = ((uint32_t)user_page_4K)>>12;
    // user page table
    user_page_4K[pt_idx].val = 0;
    user_page_4K[pt_idx].read_write = 1;
    user_page_4K[pt_idx].usr_or_supervisor = 1;
    user_page_4K[pt_idx].base_addr = VID_PT_IDX_BEGIN;
    user_page_4K[pt_idx].present = 1;
    // ready to use user level video memory mapping
    page_directory[pd_idx].KB.present = 1;
}

/*
 * disable_user_vidmem
 *   DESCRIPTION: clear the paging for the video memory (virtual)
 *   INPUTS: virtual address of screen 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void disable_user_vidmem(uint8_t * vmem){
    uint32_t pd_idx = (uint32_t)vmem>>22;
    uint32_t pt_idx = ((uint32_t)vmem & PTE_BASE_MASK)>>12;
    page_directory[pd_idx].val = 0;
    user_page_4K[pt_idx].val = 0;
}

void set_multi_process_vidmem(int32_t flags, uint32_t *arg){
    // schedule process's terminal == cur_terminal_id ?
    int32_t cur_pid_owner_terminal = search_owner_terminal(cur_pid);
    if (flags==VIDMEM_SET_PHYSICAL){
        *arg = page_table[VID_PT_IDX_BEGIN].base_addr;
        page_table[VID_PT_IDX_BEGIN].base_addr = VID_PT_IDX_BEGIN;
        flush_tlb();
        return;
    }
    if (flags==VIDMEM_FORCE_MAPPING && arg!=NULL){
        page_table[VID_PT_IDX_BEGIN].base_addr = *arg;
        flush_tlb();
        return;
    }
    if (cur_pid_owner_terminal != cur_terminal_id){
        uint8_t * cur_vid_buffer = terminal_list[cur_pid_owner_terminal].video_buffer;
        page_table[VID_PT_IDX_BEGIN].base_addr = (uint32_t)cur_vid_buffer>>12;
    }
    else{
        page_table[VID_PT_IDX_BEGIN].base_addr = VID_PT_IDX_BEGIN;
    }
    flush_tlb();
}

// update video memory
void update_multi_process_vidmem(int32_t terminal_id){
    // schedule process's terminal == cur_terminal_id ?

    // set_screen_pos(terminal_list[terminal_id].cursor_x, terminal_list[terminal_id].cursor_y);
    uint32_t pt_idx = ((uint32_t)USR_VIDMEM_ADDR & PTE_BASE_MASK)>>12;
    if(terminal_id == cur_terminal_id)
    {
        page_table[VIDEO_MEM_BEGIN>>12].base_addr = VIDEO_MEM_BEGIN>>12;
        user_page_4K[pt_idx].present = terminal_list[terminal_id].vidmap;
        user_page_4K[pt_idx].base_addr = VIDEO_MEM_BEGIN>>12;
    }else{
        page_table[VIDEO_MEM_BEGIN>>12].base_addr = (uint32_t)terminal_list[terminal_id].video_buffer>>12;
        user_page_4K[pt_idx].present = terminal_list[terminal_id].vidmap;
        user_page_4K[pt_idx].base_addr = (uint32_t)terminal_list[terminal_id].video_buffer>>12;
    }

    flush_tlb();
}


// setup user video memory
void setup_user_vidmem_for_switch(uint8_t * vmem, uint32_t terminal_offset){
    uint32_t pd_idx = (uint32_t)vmem>>22;
    uint32_t pt_idx = ((uint32_t)vmem & PTE_BASE_MASK)>>12;
    // clear page directory entry
    page_directory[pd_idx].val = 0;
    // customized settings
    page_directory[pd_idx].KB.page_size = 0;
    page_directory[pd_idx].KB.usr_or_supervisor = 1;
    page_directory[pd_idx].KB.read_write = 1;
    page_directory[pd_idx].KB.base_addr = ((uint32_t)user_page_4K)>>12;
    // user page table
    user_page_4K[pt_idx].val = 0;
    user_page_4K[pt_idx].read_write = 1;
    user_page_4K[pt_idx].usr_or_supervisor = 1;
    user_page_4K[pt_idx].base_addr = terminal_offset>>12;
    user_page_4K[pt_idx].present = 1;
    // ready to use user level video memory mapping
    page_directory[pd_idx].KB.present = 1;
}

void close_user_vidmem_for_switch(uint8_t * vmem, uint32_t terminal_offset){
    uint32_t pd_idx = (uint32_t)vmem>>22;
    uint32_t pt_idx = ((uint32_t)vmem & PTE_BASE_MASK)>>12;
    // clear page directory entry
    page_directory[pd_idx].val = 0;
    // customized settings
    page_directory[pd_idx].KB.page_size = 0;
    page_directory[pd_idx].KB.usr_or_supervisor = 1;
    page_directory[pd_idx].KB.read_write = 1;
    page_directory[pd_idx].KB.base_addr = ((uint32_t)user_page_4K)>>12;
    // user page table
    user_page_4K[pt_idx].val = 0;
    user_page_4K[pt_idx].read_write = 1;
    user_page_4K[pt_idx].usr_or_supervisor = 1;
    user_page_4K[pt_idx].base_addr = terminal_offset>>12;
    user_page_4K[pt_idx].present = 0;
    // ready to use user level video memory mapping
    page_directory[pd_idx].KB.present = 1;
}

// close user video memory (pd)
void close_user_vidmem(uint8_t * vmem){
    uint32_t pd_idx = (uint32_t)vmem>>22;
    uint32_t pt_idx = ((uint32_t)vmem & PTE_BASE_MASK)>>12;
    // clear page directory entry
    page_directory[pd_idx].val = 0;
    // customized settings
    page_directory[pd_idx].KB.page_size = 0;
    page_directory[pd_idx].KB.usr_or_supervisor = 1;
    page_directory[pd_idx].KB.read_write = 1;
    page_directory[pd_idx].KB.base_addr = ((uint32_t)user_page_4K)>>12;
    // user page table
    user_page_4K[pt_idx].val = 0;
    user_page_4K[pt_idx].read_write = 1;
    user_page_4K[pt_idx].usr_or_supervisor = 1;
    user_page_4K[pt_idx].base_addr = page_table[pt_idx].base_addr;
    user_page_4K[pt_idx].present = 0;
    // ready to use user level video memory mapping
    page_directory[pd_idx].KB.present = 1;
}





