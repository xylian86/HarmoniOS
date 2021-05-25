#include "process_crtl.h"
#include "do_syscall.h"
#include "terminal.h"
#include "x86_desc.h"
#include "lib.h"
#include "page.h"
#include "asm_linkage.h"
#include "devices/keyboard.h"
#include "timer.h"
#include "signal.h"

// static helper function
static void _init_fda(process_crtl_block_t* pcb_ptr);
static int32_t round_robin_next_pid(int32_t cur_pid_owner_terminal);

// global variable
int32_t cur_pid = NULL_PROCESS;
int32_t cur_terminal_id = DEFAULT_TERMINAL;
process_map_t process_map[MAX_PROCESS_NUM] = {{UNOCCUPIED, NOT_RUNNING, NULL_PROCESS, DEFAULT_TERMINAL},
                                              {UNOCCUPIED, NOT_RUNNING, NULL_PROCESS, DEFAULT_TERMINAL},
                                              {UNOCCUPIED, NOT_RUNNING, NULL_PROCESS, DEFAULT_TERMINAL},
                                              {UNOCCUPIED, NOT_RUNNING, NULL_PROCESS, DEFAULT_TERMINAL},
                                              {UNOCCUPIED, NOT_RUNNING, NULL_PROCESS, DEFAULT_TERMINAL},
                                              {UNOCCUPIED, NOT_RUNNING, NULL_PROCESS, DEFAULT_TERMINAL}};

/*
 * init_fda
 *   DESCRIPTION: initialize the fd array
 *   INPUTS: pid number requested
 *   OUTPUTS: None
 *   RETURN VALUE: None
 */
void init_fda(int32_t request_pid){
    process_crtl_block_t * pcb_ptr = get_pcb(request_pid);
    _init_fda(pcb_ptr);
}

/*
 * _init_fda
 *   DESCRIPTION: static helper for init_fda, initialize by pcb pointer
 *   INPUTS: pcb_ptr
 *   OUTPUTS: None
 *   RETURN VALUE: None
 */
static void _init_fda(process_crtl_block_t* pcb_ptr){
    int32_t i;
    for (i=0; i<MAX_PROCESS_FILE_NUM; i++){
        pcb_ptr->fda[i].file_op = empty_op;
        pcb_ptr->fda[i].inode = 0;
        pcb_ptr->fda[i].file_pos = 0;
        pcb_ptr->fda[i].flags = FILE_FLAG_FREE;
    }
    pcb_ptr->fda[0].file_op = get_stdin_ops();
    pcb_ptr->fda[0].flags = FILE_FLAG_IN_USE;
    pcb_ptr->fda[1].file_op = get_stdout_ops();
    pcb_ptr->fda[1].flags = FILE_FLAG_IN_USE;
    terminal_open();
}

/* load_file_tomemory
 *   DESCRIPTION: load the file into user memory          
 *   INPUTS: fname - file name
 *   OUTPUTS: none
 *   RETURN VALUE: 1 for success
 *                 0 for fail
*/
int load_file_tomemory(const uint8_t* fname){
    dentry_t dir_dentry;
    inode_blk_t* temp_inode;
    if (read_dentry_by_name((char*)fname, &dir_dentry) == FAILURE) return FAILURE;          // load the parameter of dir_dentry by the file name
    temp_inode = (inode_blk_t*)fs_start_ptr + 1 + dir_dentry.inode;                         // get the pointer of the inode
    read_data(dir_dentry.inode, 0, (char*)VIRTUAL_MEMORY_BASE_ADDRESS, temp_inode->size);   // load the whole memory to the defined address
    return SUCCESS;
}

/* create_PCB
 *   DESCRIPTION: initialize the new pcb and set the parameters          
 *   INPUTS: next_pid - the pid of the pcb
 *           args - the argument needed to be stored
 *   OUTPUTS: none
 *   RETURN VALUE: the pcb pointer
*/
process_crtl_block_t* create_PCB(int32_t next_pid, int8_t* args, int32_t flags){
    process_crtl_block_t* next_pcb_ptr = get_pcb(next_pid);         // get the pcb pointer
    next_pcb_ptr->pid = next_pid;                                   // set the parameter
    next_pcb_ptr->parent_pid = (flags==PROCESS_FORK) ? cur_pid : NULL_PROCESS;
    next_pcb_ptr->alarm_time = 0;

    _init_fda(next_pcb_ptr);                                        // initialize the fd array
    sig_init(next_pcb_ptr);
    next_pcb_ptr->tss_esp0 = KERNEL_PAGE_END-KERNEL_STACK_SIZE*next_pid-KERNEL_STACK_OFFSET;    // store the tss-esp0
    strncpy((int8_t*)next_pcb_ptr->cmd_arg, (int8_t*)(args), MAX_COMMEND_ARG);                  // copy the argument string to the pcb
    uint32_t i = 0;
    cmos_read(0, &i, next_pcb_ptr->create_time, TIMER_BUF_LEN);
    return next_pcb_ptr;
}

/*
 * save_iret_context
 *   DESCRIPTION: change the tss to kernel space with new pcb. This helper function will help us finish this step and store the content
 *                used for iret.
 *   INPUTS: current pcb 
 *   OUTPUTS: none
 *   RETURN VALUE: work for ireturn.
 */
void save_iret_context(process_crtl_block_t* cur_pcb_ptr)
{
    // XSS,ESP,XCS,EIP: c
    // store IRET 
    // %%eax means we need to distinguish the %1,%2 and registers.

    // Jump to kernel tss
    tss.esp0 = cur_pcb_ptr->tss_esp0;
    tss.ss0 = KERNEL_DS;    // store the kernel type

    // load all the registers.
    uint32_t CS = USER_CS;
    uint32_t cur_ESP = USER_MEMORY + USER_STACK_SIZE - USER_OFFSET;
    uint32_t SS = USER_DS;

    // find the content of eip
    uint8_t* entry_pointer = (uint8_t* ) VIRTUAL_MEMORY_BASE_ADDRESS+BEGIN_OFFSET; //store a pointer in this place, for next instruction.
    uint32_t cur_EIP = *(uint32_t*)entry_pointer; //get the content of this pointer, load 24-27 bytes
    
    sti();

    // if we modify the flag register, we need to use cc to tell the memory.
    asm volatile(
        "movw %%ax, %%ds;"
        "pushl %%eax;"
        "pushl %%ebx;"
        "pushfl;"
        "pushl %%ecx;"
        "pushl %%edx;"
        :
        :"a"(SS), "b"(cur_ESP), "c"(CS), "d"(cur_EIP)
        :"cc", "memory"
    );
    asm volatile("IRET");
}

/* get_cur_pcb
 *   DESCRIPTION: get the current process pcb        
 *   INPUTS: none
 *   OUTPUTS: 
 *   RETURN VALUE: the pcb pointer
*/
process_crtl_block_t * get_cur_pcb(){
    if (cur_pid==NULL_PROCESS)      
        return NULL;         
    return (process_crtl_block_t *)(KERNEL_PAGE_END-KERNEL_STACK_SIZE*(cur_pid+1));
}

/* get_pcb
 *   DESCRIPTION: get the process pcb  pointer using the pid number       
 *   INPUTS: pid number
 *   OUTPUTS: 
 *   RETURN VALUE: the pcb pointer
*/
process_crtl_block_t * get_pcb(int32_t request_pid){
    if (request_pid<0|| request_pid >= MAX_PROCESS_NUM)
        return NULL;
    return (process_crtl_block_t *)(KERNEL_PAGE_END-KERNEL_STACK_SIZE*(request_pid+1));
}

/* allocate_process
 *   DESCRIPTION: allocate one new process
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: pid for the new process
*/
int32_t allocate_process(void){
    int32_t i;
    for (i = 0; i < MAX_PROCESS_NUM; i++) {
        if (process_map[i].status == UNOCCUPIED) {
            process_map[i].status = OCCUPIED;
            process_map[i].is_running = NOT_RUNNING;
            process_map[i].pid = i;
            process_map[i].terminal_id = cur_terminal_id;
            return i;
        }
    }
    /* no space for new process, return -1 */
    return FAILURE;
}

/* free_process
 *   DESCRIPTION: free the process with given pid
 *   INPUTS: pid number
 *   OUTPUTS: none
 *   RETURN VALUE: none
*/
void free_process(int32_t pid){
    process_map[pid].status = UNOCCUPIED;
    process_map[pid].is_running = NOT_RUNNING;
    process_map[pid].pid = NULL_PROCESS;
    process_map[pid].terminal_id = DEFAULT_TERMINAL;
}

/* search_process
 *   DESCRIPTION: search the current running pid in the terminal
 *   INPUTS: terminal id
 *   OUTPUTS: none
 *   RETURN VALUE: pid of the running process in current terminal
*/
int32_t search_process(int32_t terminal_id){
    int32_t i;
    for (i = 0; i < MAX_PROCESS_NUM; i++) {
        /* find the current running process of the terminal */
        if (process_map[i].status == OCCUPIED && process_map[i].is_running == RUNNING && process_map[i].terminal_id == terminal_id) {
            return i;
        }
    }
    /* terminal_id has no current running process, return -1 */
    return FAILURE;
}

/* search_owner_terminal
 *   DESCRIPTION: the owner terminal (window) of current running process 
 *   INPUTS: request_pid
 *   OUTPUTS: none
 *   RETURN VALUE: the id of terminal that the input pid belongs to
*/
int32_t search_owner_terminal(int32_t request_pid){
    if(request_pid<0)
        return 0;
    if (request_pid>=MAX_PROCESS_NUM)
        return FAILURE;
    if (process_map[request_pid].status == OCCUPIED){
        return process_map[request_pid].terminal_id;
    }
    return FAILURE;
}

static int32_t round_robin_next_pid(int32_t cur_pid_owner_terminal){
    int32_t active_list[2] = {search_process((cur_pid_owner_terminal+1)%TERMINAL_NUM),
                              search_process((cur_pid_owner_terminal+2)%TERMINAL_NUM)};
    int32_t i, next_pid;
    next_pid = cur_pid;
    for (i=0; i<2; i++){
        if(active_list[i]!=FAILURE){
            next_pid = active_list[i];
            break;
        }
    }
    return next_pid;
}

void process_switch(){
    int32_t cur_pid_owner_terminal = search_owner_terminal(cur_pid);
    int32_t next_pid = round_robin_next_pid(cur_pid_owner_terminal);
    // save the esp and ebp every time we go in 
    process_crtl_block_t * cur_pcb_ptr = get_pcb(cur_pid);
    uint32_t cur_esp = 0;
    uint32_t cur_ebp = 0;
    asm volatile(
        "movl %%ebp, %0 \n\t"
        "movl %%esp, %1 \n\t"
        :"=g"(cur_ebp), "=g"(cur_esp)
        :
        :"memory"
    );
    // backup my stack ptr of scheduler
    cur_pcb_ptr->esp = cur_esp;
    cur_pcb_ptr->ebp = cur_ebp;

    // keep track of screen pos for current pid's terminal
    int32_t cur_screen_x, cur_screen_y;
    get_screen_pos(&cur_screen_x, &cur_screen_y);
    terminal_list[cur_pid_owner_terminal].cursor_x = cur_screen_x;
    terminal_list[cur_pid_owner_terminal].cursor_y = cur_screen_y;
    // same process? do nothing then
    if (next_pid==cur_pid)
        return;
    process_crtl_block_t * next_pcb_ptr = get_pcb(next_pid);
    // TODO: context switch to next process
    // switch to another one
    uint32_t next_esp, next_ebp;
    next_esp = next_pcb_ptr->esp;
    next_ebp = next_pcb_ptr->ebp;
    // change paging
    set_user_pt(next_pid);
    // change tss
    tss.esp0 = next_pcb_ptr->tss_esp0;

    // maintain next terminal's screen pos
    int32_t next_terminal, new_screen_x, new_screen_y;
    next_terminal = search_owner_terminal(next_pid);
    new_screen_x = terminal_list[next_terminal].cursor_x;
    new_screen_y = terminal_list[next_terminal].cursor_y;
    set_screen_pos(new_screen_x, new_screen_y);
    cur_pid = next_pid;
    //set_multi_process_vidmem(0,NULL);
    update_multi_process_vidmem(search_owner_terminal(cur_pid));
    active_terminal = search_owner_terminal(cur_pid);
    jump_to_next_process(next_esp, next_ebp);
}
