#ifndef PROCESS_CTRL_H
#define PROCESS_CTRL_H

#include "types.h"
#include "filesystem/filesys.h"
#include "x86_desc.h"

#define MAX_PROCESS_NUM         6
#define MAX_PROCESS_FILE_NUM    8
#define MAX_COMMEND_ARG         128


#define KERNEL_PAGE_END     0x800000
#define KERNEL_STACK_SIZE   0x2000

#define USER_MEMORY         0x08000000
#define USER_STACK_SIZE     0x400000
#define USER_OFFSET         4
#define KERNEL_STACK_OFFSET 4
#define BEGIN_OFFSET        24
#define OFFSET_4MB          22

#define FAILURE         -1
#define SUCCESS         0
#define NULL_PROCESS    -1
#define DEFAULT_TERMINAL 0
#define OCCUPIED        1
#define UNOCCUPIED      0
#define RUNNING         1
#define NOT_RUNNING     0

#define MEMORY_LEAK 4
#define TIMER_BUF_LEN   20

#define VIRTUAL_MEMORY_BASE_ADDRESS 0x8048000
#define VIRTUAL_MEMORY_END_ADDRESS  0x8400000
#define EIP_OFFSET  24


typedef int32_t(*signal_handler)(void);

typedef struct  sig_regs
{
    uint32_t    ebx;
    uint32_t    ecx;
    uint32_t    edx;
    uint32_t    esi;
    uint32_t    edi;
    uint32_t    ebp;
    uint32_t    eax;
    uint32_t    ds;
    uint32_t    es;
    uint32_t    fs;
    uint32_t    num;
    uint32_t    err;
    uint32_t    eip;
    uint32_t    cs;
    uint32_t    EFLAGS;
    uint32_t    useresp;
    uint32_t    ss;
} sig_regs;

typedef struct sig_stack{
    uint32_t* ret_addr;
    int32_t sig_num;
    sig_regs hwcontent;
    uint8_t sigreturn[8];
}sig_stack;

typedef struct{
    int32_t signum;
    signal_handler sig_handler;
    uint8_t mask;
    uint8_t pending;
    uint32_t alarm_time;
}signal_struct;



typedef struct {
    // process id for this pcb
    int32_t pid;
    // parent pid
    int32_t parent_pid;
    //command and command arguments
    uint8_t cmd[MAX_COMMEND_ARG];
    uint8_t cmd_arg[MAX_COMMEND_ARG];
    // vidmem
    uint16_t use_vidmem;
    uint8_t *vmem;
    // current pos on stack
    uint32_t esp;
    uint32_t ebp;
    // stack switch address
    uint32_t tss_esp0;
    // file descriptor array
    file_desc_entry_t fda[MAX_PROCESS_FILE_NUM];
    // signal struct
    signal_struct sig[5];
    uint32_t alarm_time;
    // create time
    char create_time[TIMER_BUF_LEN];
}process_crtl_block_t;

typedef struct {
    uint8_t status;
    uint8_t is_running;
    int32_t pid;
    int32_t terminal_id;
} process_map_t;

#define ONTO_DISPLAY_WRAP(code) {               \
    video_mem = (char*) 0xb7000;   \
    code;                                       \
    video_mem = (char*) 0xb8000;                  \
}

extern int32_t cur_pid;
extern int32_t cur_terminal_id;
extern process_map_t process_map[MAX_PROCESS_NUM];

void init_fda(int32_t request_pid);

void save_iret_context(process_crtl_block_t* cur_pcb_ptr);

process_crtl_block_t * get_cur_pcb();

#define PROCESS_FORK    1
#define PROCESS_NO_FORK 2
process_crtl_block_t * get_pcb(int32_t request_pid);

process_crtl_block_t* create_PCB(int32_t next_pid, int8_t* arg, int32_t flags);

int load_file_tomemory(const uint8_t* fname);

int32_t allocate_process(void);

void free_process(int32_t pid);

int32_t search_process(int32_t terminal_id);

int32_t search_owner_terminal(int32_t request_pid);

void process_switch();

#endif
