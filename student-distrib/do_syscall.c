#include "do_syscall.h"
#include "process_crtl.h"
#include "x86_desc.h"
#include "asm_linkage.h"
#include "terminal.h"
#include "idt.h"
#include "page.h"
#include "vga_design.h"
#include "lib.h"
#include "devices/speaker.h"



// user video memory
#define USR_VIDMEM_ADDR 0x10000000

// static helper functions
static int32_t parse_argument(const int8_t* command, uint8_t* filename, uint8_t* args);
static int32_t check_executable(uint8_t* filename);
static void close_terminal_file(file_desc_entry_t* fda);

file_ops_table_t empty_op = {NULL, NULL, NULL, NULL};

/*
 * close_terminal_file
 *   DESCRIPTION: close stdin and stdout
 *   INPUTS: fda - file descriptor array
 *   OUTPUTS: None
 *   RETURN VALUE: None
 */
static void close_terminal_file(file_desc_entry_t* fda){
    fda[0].flags = FILE_FLAG_FREE;
    fda[0].file_op = empty_op;
    fda[1].flags = FILE_FLAG_FREE;
    fda[1].file_op = empty_op;
}


/*
 * halt
 *   DESCRIPTION: halt a program, return from execute stack frame
 *   INPUTS: status
 *   OUTPUTS: None
 *   RETURN VALUE: status from user program
 */
int32_t halt (uint8_t status)
{
    /* get current PCB */
    cli();
    uint32_t status_new = (uint32_t) status;
    process_crtl_block_t * cur_pcb = get_cur_pcb();
    if (cur_pcb == NULL){
        sti();
        return FAILURE;
    }

    if (cur_pcb->parent_pid == NULL_PROCESS) {
        /* current process is the base shell, need to restart */
        printf("==== DON'T EXIT ROOT SHELL ==== \n");
        free_process(cur_pid);
        terminal_list[cur_terminal_id].shell_opened = 0;
        execute("shell");
    } else {
        /* current process is not the base shell */
        int32_t parent_pid, parent_esp, parent_ebp;
        parent_pid = cur_pcb->parent_pid;
        process_crtl_block_t * parent_pcb = get_pcb(parent_pid);
        parent_esp = parent_pcb->esp;
        parent_ebp = parent_pcb->ebp;

        /* restore the parent data */
        // tss.esp0 = KERNEL_STACK_SIZE - parent_pid * KERNEL_STACK_SIZE - 4;
        tss.esp0 = parent_pcb->tss_esp0;
        tss.ss0 = KERNEL_DS;

        /* restore the parent paging */
        set_user_pt(parent_pid);

        /* close any relevant FDs */
        int32_t i, ret;
        file_desc_entry_t* cur_fda = cur_pcb->fda;
        if (cur_fda == NULL) {
            sti();
            return FAILURE;
        }
        close_terminal_file(cur_fda);
        for (i = 2; i < MAX_PROCESS_FILE_NUM; i++) {
            /* close opened files/devices */
            if (cur_fda[i].flags == FILE_FLAG_IN_USE) {
                ret = close(i);
                if (ret == -1) {
                    printf("fail to close file\n");
                    sti();
                    return FAILURE;
                }
            }
        }
        /* close vidmap */ 
        if (cur_pcb->use_vidmem){
            // disable_user_vidmem(cur_pcb->vmem);
            // cur_pcb->use_vidmem = 0;
            // cur_pcb->vmem = NULL;
            cur_pcb->use_vidmem = 0;
            terminal_list[search_owner_terminal(cur_pid)].vidmap = 0;
            
            //qemu_vga_enabled = 1;
            
            if(cur_terminal_id == search_owner_terminal(cur_pid))
            {
                close_user_vidmem_for_switch((uint8_t*) USR_VIDMEM_ADDR, VIDEO_MEM_BEGIN);
            }else{
                close_user_vidmem_for_switch((uint8_t*) USR_VIDMEM_ADDR, (uint32_t) terminal_list[search_owner_terminal(cur_pid)].video_buffer);
            }
        }
        /* update cur_pid */
        free_process(cur_pid);
        process_map[parent_pid].is_running = RUNNING;
        // TODO: SYNC PROBLEM HERE
        cur_pid = parent_pid;
        sti();
        /* restore parent esp and ebp, switch back to the parent user stack */
        // asm volatile (
        //     "xorl %%eax, %%eax;"
        //     "movw %0, %%eax;"
        //     "movl %1, %%esp;"
        //     "movl %2, %%ebp;"
        //     "leave;"
        //     "ret;"
        //     : 
        //     : "r" (status), "r" (parent_esp), "r" (parent_ebp)
        //     : "eax", "esp", "ebp"
        // );

        if(halt_flag == 1)
        {
            halt_flag = 0;
            status_new = EXCEPTION_HANDLER;
        }

        jump_to_execute_return(status_new, parent_esp, parent_ebp);
    }
    sti();
    return SUCCESS;
}

/*
 * parse_argument
 *   DESCRIPTION: seperate the input command into cmd and filename. for example, for "cat frame0.txt", filename will be "cat" and args will be
 *   "frame0.txt"
 *   INPUTS: command from execute input
 *   OUTPUTS: filename string and args string, the content is shown in the description part.
 *   RETURN VALUE: 0 for success and -1 for failure
 */

static int32_t parse_argument(const int8_t* command, uint8_t* filename, uint8_t* args)
{
    int filename_index = 0;     // index for filename
    int args_index = 0;         // index for argument
    int filename_length = 0;    // count the length of filename part
    int i;                      // index count in loop
    int count_blank = 0;        // count the number of space
    int command_len = strlen(command); //get the length of string command.

    // if command is NULL, return failure
    if(command == NULL)
        return  FAILURE;

    // fill the two buffers as NULL.
    for(i=0; i<MAX_FILENAME_LEN+1; i++)
    {
        filename[i] = '\0';
        args[i] = '\0';
    }

    //parse filename
    for(i=0;i<command_len;i++)
    {
        // check space
        if(command[i] != ' ')
        {
            filename[filename_index] = command[i];
            filename_index++;
            filename_length++;
        }else{
            count_blank ++;
            if(filename_length > 0)
                break;
        }
    }
    
    //prase arg
    for(i=filename_index+count_blank;i<command_len;i++)
    {
        // check space
        if(command[i] != ' ' && args_index<MAX_COMMEND_ARG)
        {
            args[args_index] = command[i];
            args_index ++;
        }else{
            // meet space means end.
            if(args_index > 0)
            {
                break;
            }
        }
    }
    return SUCCESS;
}

/*
 * check_executable
 *   DESCRIPTION: check if the file is executable
 *   INPUTS: filename
 *   OUTPUTS: None
 *   RETURN VALUE: 0 for success and -1 for failure
 */
static int32_t check_executable(uint8_t* filename){
    dentry_t dir_dentry;
    char buf[EXECTURE_BUF_SIZE];
    int ret = read_dentry_by_name((char*)filename, &dir_dentry);
    if (ret==FAILURE)
        return FAILURE;
    read_data(dir_dentry.inode,0,buf,4);
    // magic number to check if the file is executable
    // number referred from mp3 document appendix C
    if (buf[0]!=0x7f || buf[1]!=0x45 || buf[2]!=0x4c || buf[3]!=0x46)
        return FAILURE;
    return SUCCESS;
}

/*
 * execute
 *   DESCRIPTION: execute the command, including five steps: parse arguments, check for executable, set up paging, load file into memory,
 *                create PCB, prepare for the context switch, push iret context to kernel stack and iret.
 *   INPUTS: command 
 *   OUTPUTS: Nothing.
 *   RETURN VALUE: 0 for success and -1 for failure
 */

int32_t execute(const int8_t* command)
{
    if (command == NULL)
        return FAILURE;
    // forbid the interrupt.
    cli();

    // parse arg
    uint8_t filename[MAX_COMMEND_ARG]; 
    uint8_t arg[MAX_COMMEND_ARG];
    parse_argument(command, filename, arg);

    // check for executable 
    if (check_executable(filename)==FAILURE){
        sti();
        return FAILURE;
    }

    // set up paging
    int32_t next_pid = allocate_process();
    if (next_pid == FAILURE){
        printf("NO MORE PROCESS!\n");
        sti();
        return FAILURE;
    }

    set_user_pt(next_pid);

    // load file into memory
    int load_result = load_file_tomemory(filename);
    if (load_result==FAILURE){
        // restore paging back
        set_user_pt(cur_pid);
        sti();
        return FAILURE;
    }
    int32_t process_fork_flag = terminal_list[cur_terminal_id].shell_opened == 0? PROCESS_NO_FORK : PROCESS_FORK;
    if (!strncmp((int8_t*)filename,(int8_t*)"shell",6)){
        terminal_list[cur_terminal_id].shell_opened = 1;
    }
    // create PCB
    process_crtl_block_t* new_pcb = create_PCB(next_pid, (int8_t*)arg, process_fork_flag);
    // printf("%s", filename);
    strcpy((char*) &(new_pcb->cmd), (char*) filename);

    // prepare for context switch
    process_crtl_block_t* cur_pcb_ptr = get_cur_pcb();


    int32_t esp_backup, ebp_backup;     // old ebp and esp
    asm volatile(
        "movl %%esp, %0;"
        "movl %%ebp, %1;"
        :"=r" (esp_backup), "=r"(ebp_backup)
        :
        :"cc", "memory"
    );

    // Do not store the esp and ebp of base shell pcb.
    if (cur_pid!=NULL_PROCESS){
        // save the esp and ebp in parent process when we want to fork process from shell
        cur_pcb_ptr->ebp = ebp_backup;
        cur_pcb_ptr->esp = esp_backup;
    }
    // ready for everything for new process
    process_map[cur_pid].is_running = (process_fork_flag==PROCESS_FORK) ? NOT_RUNNING : RUNNING;
    process_map[next_pid].is_running = RUNNING;
    // update global variable cur_pid
    cur_pid = next_pid;

    active_terminal = search_owner_terminal(cur_pid);
    // maybe we should update the video memory
    // make sure the cur_pid is mapping to right vidmem/vid_buffer
    if (process_fork_flag==PROCESS_NO_FORK){
        set_multi_process_vidmem(0,NULL);
    }
    // save iret context, run program
    save_iret_context(new_pcb);

    return SUCCESS;
}


/*
 * read
 *   DESCRIPTION: general interface of read operations
 *   INPUTS: fd - index to file descriptor array
 *           buf - buffer to contain data
 *           nbytes      
 *   OUTPUTS: None
 *   RETURN VALUE: number of bytes read for success and -1 for failure
 */
int32_t read (int32_t fd, void* buf, int32_t nbytes)
{
    if (fd<MIN_FDA || fd>=MAX_FDA)
        return FAILURE;
    process_crtl_block_t * cur_pcb = get_cur_pcb();
    file_desc_entry_t* file = &(cur_pcb->fda[fd]);
    if (cur_pcb==NULL || file==NULL)
        return FAILURE;
    if (file->flags==FILE_FLAG_IN_USE){
        if (file->file_op.read == NULL)
            return FAILURE;
        //printf("read success!\n");
        return (file->file_op).read(file->inode, &(file->file_pos), buf, nbytes);
    }
    else{
        // printf("haven't opened the file\n");
        return FAILURE;
    }
}

/*
 * write
 *   DESCRIPTION: general interface to write
 *   INPUTS: fd - index to file descriptor array
 *           buf - buffer to contain data
 *           nbytes      
 *   OUTPUTS: None
 *   RETURN VALUE: number of bytes written for success and -1 for failure
 */
int32_t write (int32_t fd, const void* buf, int32_t nbytes)
{
    if (fd<MIN_FDA || fd>=MAX_FDA)
        return FAILURE;
    process_crtl_block_t * cur_pcb = get_cur_pcb();
    file_desc_entry_t* file = &(cur_pcb->fda[fd]);
    if (cur_pcb==NULL || file==NULL)
        return -1;
    if (file->flags==FILE_FLAG_IN_USE){
        if (file->file_op.write == NULL)
            return FAILURE;
        //printf("Write success!\n");
        return (file->file_op).write(file->inode, buf, nbytes);
    }
    else{
        // printf("haven't opened the file\n");
        return FAILURE;
    }
}   

/*
 * open
 *   DESCRIPTION: general interface to open
 *   INPUTS: filename
 *   OUTPUTS: None
 *   RETURN VALUE: fd
 */
int32_t open (const uint8_t* filename)
{
    static int8_t dirname[] = ".";
    static int8_t rtcname[] = "rtc";
    int32_t file_type = -1;
    if (filename==NULL)
        return FAILURE;
    process_crtl_block_t * cur_pcb = get_cur_pcb();
    if (cur_pcb==NULL)
        return FAILURE;
    int32_t i, found_free_slot, ret;
    file_desc_entry_t * file;
    found_free_slot = 0;
    for (i=2; i<MAX_PROCESS_FILE_NUM; i++){
        if (cur_pcb->fda[i].flags==FILE_FLAG_FREE){
            found_free_slot = 1;
            file = &(cur_pcb->fda[i]);
            break;
        }
    }
    if (found_free_slot==0 || file==NULL)
        return FAILURE;
    // parse the filename, load function operations
    if (!strncmp((int8_t*)filename, dirname, strlen((int8_t*)filename)+1)){
        file_type = 1;
        file->file_op = get_file_ops(1);
    }
    if (!strncmp((int8_t*)filename, rtcname, strlen((int8_t*)filename)+1)){
        file_type = 0;
        file->file_op = get_file_ops(0);
    }
    if (file_type<0){
        // regular file name perhaps
        file_type = 2;
        file->file_op = get_file_ops(2);
    }
    if (file->file_op.open == NULL)
        return FAILURE;
    // call open function
    ret = file->file_op.open(filename);
    if (ret==FAILURE)
        return FAILURE;
    file->flags = FILE_FLAG_IN_USE;
    file->file_pos = 0;
    file->inode = (file_type==2)? ret : 0;
    //printf("open success!\n");
    // i is the fd for this open
    return i;
}

/*
 * close
 *   DESCRIPTION: genetral interface of close
 *   INPUTS: fd - index to file descriptor array
 *   OUTPUTS: None
 *   RETURN VALUE: 0 for success and -1 for failure
 */
int32_t close (int32_t fd)
{
    if (fd<MIN_FDA || fd>=MAX_FDA)
        return FAILURE;
    process_crtl_block_t * cur_pcb = get_cur_pcb();
    file_desc_entry_t* file = &(cur_pcb->fda[fd]);
    int32_t ret;
    if (cur_pcb==NULL || file==NULL || file->file_op.close==NULL || file->flags==FILE_FLAG_FREE)
        return FAILURE;
    ret = file->file_op.close(&(file->inode));
    if (ret==FAILURE)
        return FAILURE;
    // close success
    file->file_pos = 0;
    file->file_op = empty_op;
    file->flags = FILE_FLAG_FREE;

    //printf("close success!\n");
    return 0;
}

// for cp3.4
/*
 * getargs
 *   DESCRIPTION: Work for store the current pcb command into a buffer.
 *   INPUTS: bytes and a idle buffer.
 *   OUTPUTS: fill buffer or failure
 *   RETURN VALUE: 0 for success and -1 for failure
 */
int32_t getargs (uint8_t* buf, int32_t nbytes)
{
    // if buffer pointer is NULL, return failure
    if(buf == NULL)
        return FAILURE;


    // get current pcb.
    process_crtl_block_t* current_pcb = get_cur_pcb();

    // check whether we have argument here.
    if(current_pcb->cmd_arg[0] == NULL)
        return FAILURE;

    // copy the current cmd argument to the buffer
    strncpy((int8_t*)buf, (int8_t*)(current_pcb->cmd_arg), nbytes);
    
    //printf("getargs success!\n");
    return SUCCESS;
}

/*
 * vidmap
 *   DESCRIPTION: map a region to video region
 *   INPUTS: screen_start - the address of screen data array (in user program), double pointer
 *   OUTPUTS: None
 *   RETURN VALUE: -1 for failure and 0 for success
 */
int32_t vidmap (uint8_t** screen_start)
{
    // check boundary condition
    if (screen_start==NULL ||
        (uint32_t)screen_start<=VIRTUAL_MEMORY_BASE_ADDRESS||
        (uint32_t)screen_start>=VIRTUAL_MEMORY_END_ADDRESS)
        return FAILURE;
    
    //qemu_vga_enabled = 0;
    
    // get current pcb
    process_crtl_block_t * cur_pcb = get_cur_pcb();
    // set target video memory address
    uint8_t * target_vidmem_addr = (uint8_t *)USR_VIDMEM_ADDR;
    *screen_start = target_vidmem_addr;
    if (cur_pcb==NULL)
        return FAILURE;
    // set up the video map for user
    if(search_process(cur_terminal_id))
    {
        setup_user_vidmem_for_switch(target_vidmem_addr, VIDEO_MEM_BEGIN);
    }else{
        setup_user_vidmem_for_switch(target_vidmem_addr, (uint32_t) terminal_list[search_owner_terminal(cur_pid)].video_buffer);
    }
    flush_tlb();

    // set relevant parameters 
    cur_pcb->vmem = target_vidmem_addr;
    cur_pcb->use_vidmem = 1;

    terminal_list[search_owner_terminal(cur_pid)].vidmap = 1;
    // success
    return SUCCESS;
}


// extra credits
int32_t sigreturn (void)
{
    return SUCCESS;
}

//extra
int32_t new_poke(uint32_t pos, uint32_t data)
{
    uint32_t x = (pos/2)%80;
    uint32_t y = (pos/2)/80+1;
    if(x>=SCREEN_WIDTH||y>=SCREEN_HEIGHT)
    {
        return FAIL;
    }

    uint8_t ch = (uint8_t) data;
    uint8_t attr = 0x7;
    *(uint8_t *)(video_mem + ((NUM_COLS * y + x) << 1)) = ch;
    *(uint8_t *)(video_mem + ((NUM_COLS * y + x) << 1) + 1) = attr;

    qemu_vga_putc(x * 8, y * FONT_ACTUAL_HEIGHT,
        ch, qemu_vga_get_terminal_color(attr), qemu_vga_get_terminal_color(attr >> 4));

    return SUCCESS;
}


int32_t beep(void) {
    speaker_beep();
    return SUCCESS;
}


int32_t ps(void) {
    int32_t i;
    printf("\t PID   Terminal#    Status      Command    CreateTime\n");
    for (i = 0; i < MAX_PROCESS_NUM; i++) {
        /* only see current active pid */
        if (process_map[i].status == OCCUPIED) {
            process_crtl_block_t* temp = get_pcb(i);
            /* skip current process */
            if (i == cur_pid) continue;
             /* set the parent to running */
            if (i == get_cur_pcb()->parent_pid) {
                printf("\t  %d        %d        running     %s     %s\n", i, process_map[i].terminal_id, &(temp->cmd), &(temp->create_time));
                continue;
            }
            /* calculate running time */
            // TODO: calculate run time
            /* print current running and waiting processes */
            if (process_map[i].is_running == RUNNING) 
                printf("\t  %d        %d        running     %s     %s\n", i, process_map[i].terminal_id, &(temp->cmd), &(temp->create_time));
            else
                printf("\t  %d        %d        sleeping    %s     %s\n", i, process_map[i].terminal_id, &(temp->cmd), &(temp->create_time));
        }
    }
    return SUCCESS;
}



int32_t random(void)
{
    return 0;
}
