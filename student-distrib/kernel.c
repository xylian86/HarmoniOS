/* kernel.c - the C part of the kernel
 * vim:ts=4 noexpandtab
 */

#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "devices/i8259.h"
#include "debug.h"
#include "tests.h"
#include "idt.h"
#include "devices/rtc.h"
#include "devices/keyboard.h"
#include "page.h"
#include "filesystem/filesys.h"
#include "devices/cursor.h"
#include "do_syscall.h"
#include "devices/pit.h"
#include "terminal.h"
#include "vga_design.h"
#include "status_bar.h"
#include "pci.h"
#include "data/desktop.h"
#include "cursor_graphic.h"
#include "devices/mouse.h"
#include "mouse_graphic.h"

#define RUN_TESTS

/* Macros. */
/* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags, bit)   ((flags) & (1 << (bit)))

/* Check if MAGIC is valid and print the Multiboot information structure
   pointed by ADDR. */
void entry(unsigned long magic, unsigned long addr) {

    multiboot_info_t *mbi;

    /* Clear the screen. */
    clear();

    /* Am I booted by a Multiboot-compliant boot loader? */
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        printf("Invalid magic number: 0x%#x\n", (unsigned)magic);
        return;
    }

    /* Set MBI to the address of the Multiboot information structure. */
    mbi = (multiboot_info_t *) addr;

    /* Print out the flags. */
    printf("flags = 0x%#x\n", (unsigned)mbi->flags);

    /* Are mem_* valid? */
    if (CHECK_FLAG(mbi->flags, 0))
        printf("mem_lower = %uKB, mem_upper = %uKB\n", (unsigned)mbi->mem_lower, (unsigned)mbi->mem_upper);

    /* Is boot_device valid? */
    if (CHECK_FLAG(mbi->flags, 1))
        printf("boot_device = 0x%#x\n", (unsigned)mbi->boot_device);

    /* Is the command line passed? */
    if (CHECK_FLAG(mbi->flags, 2))
        printf("cmdline = %s\n", (char *)mbi->cmdline);

    if (CHECK_FLAG(mbi->flags, 3)) {
        int mod_count = 0;
        int i;
        module_t* mod = (module_t*)mbi->mods_addr;
        init_filesys((uint32_t*)(mod->mod_start));
        while (mod_count < mbi->mods_count) {
            printf("Module %d loaded at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_start);
            printf("Module %d ends at address: 0x%#x\n", mod_count, (unsigned int)mod->mod_end);
            printf("First few bytes of module:\n");
            for (i = 0; i < 16; i++) {
                printf("0x%x ", *((char*)(mod->mod_start+i)));
            }
            printf("\n");
            mod_count++;
            mod++;
        }
    }
    /* Bits 4 and 5 are mutually exclusive! */
    if (CHECK_FLAG(mbi->flags, 4) && CHECK_FLAG(mbi->flags, 5)) {
        printf("Both bits 4 and 5 are set.\n");
        return;
    }

    /* Is the section header table of ELF valid? */
    if (CHECK_FLAG(mbi->flags, 5)) {
        elf_section_header_table_t *elf_sec = &(mbi->elf_sec);
        printf("elf_sec: num = %u, size = 0x%#x, addr = 0x%#x, shndx = 0x%#x\n",
                (unsigned)elf_sec->num, (unsigned)elf_sec->size,
                (unsigned)elf_sec->addr, (unsigned)elf_sec->shndx);
    }

    /* Are mmap_* valid? */
    if (CHECK_FLAG(mbi->flags, 6)) {
        memory_map_t *mmap;
        printf("mmap_addr = 0x%#x, mmap_length = 0x%x\n",
                (unsigned)mbi->mmap_addr, (unsigned)mbi->mmap_length);
        for (mmap = (memory_map_t *)mbi->mmap_addr;
                (unsigned long)mmap < mbi->mmap_addr + mbi->mmap_length;
                mmap = (memory_map_t *)((unsigned long)mmap + mmap->size + sizeof (mmap->size)))
            printf("    size = 0x%x, base_addr = 0x%#x%#x\n    type = 0x%x,  length    = 0x%#x%#x\n",
                    (unsigned)mmap->size,
                    (unsigned)mmap->base_addr_high,
                    (unsigned)mmap->base_addr_low,
                    (unsigned)mmap->type,
                    (unsigned)mmap->length_high,
                    (unsigned)mmap->length_low);
    }

    /* Construct an LDT entry in the GDT */
    {
        seg_desc_t the_ldt_desc;
        the_ldt_desc.granularity = 0x0;
        the_ldt_desc.opsize      = 0x1;
        the_ldt_desc.reserved    = 0x0;
        the_ldt_desc.avail       = 0x0;
        the_ldt_desc.present     = 0x1;
        the_ldt_desc.dpl         = 0x0;
        the_ldt_desc.sys         = 0x0;
        the_ldt_desc.type        = 0x2;
        // merge the base address information of variable ldt (whose storage 
        // is in the x86_desc.S) into the ldt desciptor and then put it inside
        // GDT by changing the ldt_desc_ptr which is a label in x86_desc.S file
        SET_LDT_PARAMS(the_ldt_desc, &ldt, ldt_size);
        // fill in data in the GDT here
        ldt_desc_ptr = the_ldt_desc;
        // load local descriptor table
        lldt(KERNEL_LDT);
    }

    /* Construct a TSS entry in the GDT */
    {
        seg_desc_t the_tss_desc;
        the_tss_desc.granularity   = 0x0;
        the_tss_desc.opsize        = 0x0;
        the_tss_desc.reserved      = 0x0;
        the_tss_desc.avail         = 0x0;
        the_tss_desc.seg_lim_19_16 = TSS_SIZE & 0x000F0000;
        the_tss_desc.present       = 0x1;
        the_tss_desc.dpl           = 0x0;
        the_tss_desc.sys           = 0x0;
        the_tss_desc.type          = 0x9;
        the_tss_desc.seg_lim_15_00 = TSS_SIZE & 0x0000FFFF;
        // merge the address variable tss (exist in x86_desc.S)
        // into the local variable the_tss_desc, and put in into
        // the GDT by directly modifying the label's content in 
        // assembly file x86_desc.S
        SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);
        // fill in the tss_desc_ptr into GDT here
        tss_desc_ptr = the_tss_desc;
        // the data space of tss is in x86_desc.S file
        tss.ldt_segment_selector = KERNEL_LDT;
        tss.ss0 = KERNEL_DS;
        tss.esp0 = 0x800000;
        // load task register
        ltr(KERNEL_TSS);
    }

    /* init file system */
    // module_t* mod = (module_t*)mbi->mods_addr;
    // init_filesys((uint32_t*)(mod->mod_start));

    /* Init the PIC */
    i8259_init();
    /* Init the idt */
    init_idt();
    
    /* Initialize devices, memory, filesystem, enable device interrupts on the
     * PIC, any other initialization stuff... */
    /* Init the rtc */
    rtc_init();
    /* Init the keyboard */
    init_keyboard();
    /* init VGA*/
    pci_init(); //work for VGA
    /* paging */    
    qemu_vga_init(QEMU_VGA_DEFAULT_WIDTH, QEMU_VGA_DEFAULT_HEIGHT, QEMU_VGA_DEFAULT_BPP);
    init_paging();
    /* init the cursor */
    enable_cursor(13, 14);
    /* initialize multi-process scheduling */
    multi_terminal_init();
    pit_init();
    /* ready to go! */
    // init history buffer
    init_history_list();
    // init history buffer list
    graphic_cursor_init();
    /* initialize mouse */
    mouse_init();
    animation();
    char startw[] = "This is our OS!";
    message_update_for_sb(startw, strlen(startw), PARM_BLACK_ON_WHITE);

    draw_terminal_icon();
    graphic_mouse_init();

    // show the graph at start
    qemu_vga_show_picture(DESKTOP_IMAGE_WIDTH, DESKTOP_IMAGE_HEIGHT, QEMU_VGA_DEFAULT_BPP, (uint8_t*)DESKTOP_IMAGE_DATA);
    execute("shell");
    /*printf("Enabling Interrupts\n");
    sti();*/
    sti();

#ifdef RUN_TESTS
    /* Run tests */
    //clear();
    //clear_screen_pos();
    //launch_tests();
#endif
    /* Execute the first program ("shell") ... */
    while(1){}
    /* Spin (nicely, so we don't chew up cycles) */
    asm volatile (".1: hlt; jmp .1;");
}
