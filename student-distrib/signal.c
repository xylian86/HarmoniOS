#include "lib.h"
#include "signal.h"
#include "process_crtl.h"
#include "do_syscall.h"
#include "idt.h"

extern void signal_set_up_stack_helper(signal_handler handler, int32_t signum, sig_regs *hw_context_addr);

#define asmlinkage __attribute__((regparm(0)))


// reference : https://wiki.osdev.org/Signals

// default signal handler
int32_t signal_div_zero_default_handler();
int32_t signal_segfault_default_handler();
int32_t signal_interrupt_default_handler();
int32_t signal_alarm_default_handler();
int32_t signal_user1_default_handler();
int32_t set_frame(sig_regs* r, int32_t signum, process_crtl_block_t* pcb, signal_handler sig_handler);
// default signal handler list, convenient to get
signal_handler default_handlers[NUM_SIGNAL] = {signal_div_zero_default_handler, signal_segfault_default_handler, signal_interrupt_default_handler, signal_alarm_default_handler, signal_user1_default_handler};

/*
 * sig_init
 *   DESCRIPTION: initialize the signal part in the pcb 
 *   INPUTS: pcb - pcb struct need to be initialized 
 *   OUTPUTS: None
 *   RETURN VALUE: None
 */
// it has been added to the function create_PCB
void sig_init(process_crtl_block_t* pcb){
    int32_t i;
    for(i = 0; i < NUM_SIGNAL; i++){
        pcb->sig[i].signum = i;
        pcb->sig[i].sig_handler = default_handlers[i];
        pcb->sig[i].mask = UNMASK;
        pcb->sig[i].pending = NOPENDING;
    }
}

/*
 * sig_handler_func
 *   DESCRIPTION: handlers the signal
 *   INPUTS: r    - the H/W content from the system call
 *   OUTPUTS: None
 *   RETURN VALUE: None
 */
asmlinkage void sig_handler_func( sig_regs r){
    process_crtl_block_t *pcb  = get_cur_pcb();
    int32_t i, j;
    // iterate the five signal struct
    for(i = 0; i < NUM_SIGNAL; i ++){
        signal_struct *sig = &pcb->sig[i];
        // if there is no pending  or the signal is masked
        if((sig->pending == NOPENDING) || (sig->mask == MASK)){
            continue;
        }
        else{
            sig->pending = NOPENDING; 
            // set all the signal to masked
            for(j = 0; j < NUM_SIGNAL; j++){
                pcb->sig[j].mask = MASK;
            }
            // if the handler is the default handler
            if((sig->sig_handler == default_handlers[i]) || (sig->sig_handler == NULL)){
                default_handlers[i]();
            }
            else{
                // printf("This is not the default signal handler");
                set_frame(&r, i, pcb, sig->sig_handler);
            }
        }
    }
}

/*
 * set_frame
 *   DESCRIPTION: set the frame in the user_stack
 *   INPUTS: r      - signal struct that need to be set
 *           signum - species of the signal
 *           pcb    - pcb struct that  generate the signal
 *           sig_hangler - the user-set signal  handler
 *   OUTPUTS: None
 *   RETURN VALUE: None
 */
int32_t set_frame(sig_regs* r, int32_t signum, process_crtl_block_t* pcb, signal_handler sig_handler){
    sig_stack* cur_sig_stack;
    // set the parameter(which is equal to fill the  frame)
    cur_sig_stack = (sig_stack*) (r->useresp - (sizeof(sig_stack) & (~0x3)));
    cur_sig_stack->ret_addr = (uint32_t*) (r->useresp - RET_LENGTH*sizeof(uint8_t));
    cur_sig_stack->sig_num = signum;
    cur_sig_stack->hwcontent = *r;


    // the following  part is from the linux source file 
            static const struct { 
        uint16_t poplmovl;
        uint32_t val;
        uint16_t int80;    
        uint16_t pad; 
    } __attribute__((packed)) code = { 
        0xb8fc,		 /* popl %eax ; movl $...,%eax */
        SIGRETURN_VAL,   
        0x80cd,		/* int $0x80 */
        0,
    };


    memcpy(cur_sig_stack->ret_addr, &code, RET_LENGTH);
    // because we need to begin ar the 0xb8fc, but now the ret_addr will point to SIGRETUREN_VAL
    *cur_sig_stack->ret_addr += 1;
    // need to set  the kernal stack so the kernal will back to user when the expection or system call finished
    r->useresp = (unsigned int)cur_sig_stack;
    r->eip =  (uint32_t) sig_handler;
    r->ds = USER_DS;
    r->es = USER_DS;
    r->ss = USER_DS;
    r->cs = USER_CS;

    return 0;
}

/*
 * signal_raise
 *   DESCRIPTION: raise a signal interrupt
 *   INPUTS: signum - species of the signal
 *   OUTPUTS: None
 *   RETURN VALUE: None
 */
void signal_raise(int32_t signum){
    process_crtl_block_t *pcb;
    // if ctrl+C, we need to halt the program we saw
    if(signum ==2){
        int32_t pid = search_process(cur_terminal_id);
        pcb = get_pcb(pid);
    }
    else{
        pcb = get_cur_pcb();
    }
    signal_struct *sig = &pcb->sig[signum];
    sig->pending = PENDING;
}


/*
 * set_handler
 *   DESCRIPTION: set the signal handler defined by user
 *   INPUTS: signum - species of the signal
 *           handler_address - the user-defined handler
 *   OUTPUTS: None
 *   RETURN VALUE: None
 */
// extra credits
int32_t set_handler (int32_t signum, void* handler_address)
{   
    if(signum < 0 || signum >= NUM_SIGNAL){
        return -1;
    }
    process_crtl_block_t* pcb = get_cur_pcb();
    // if the handler is NULL, just using the default handler
    if(handler_address == NULL){
        pcb->sig[signum].sig_handler = default_handlers[signum];
    }
    else{
        pcb->sig[signum].sig_handler = handler_address;
    }
    
    return SUCCESS;
}

/*
 * sigreturn_func
 *   DESCRIPTION: sigreturn part
 *   INPUTS: r - parameters of the hw_context
 *   OUTPUTS: None
 *   RETURN VALUE: None
 */
void sigreturn_fuc(sig_regs r){
    r = *(sig_regs*)(r.useresp+4);
    process_crtl_block_t* pcb = get_cur_pcb();
    uint32_t i;
    for(i = 0; i < NUM_SIGNAL; i++){
        pcb->sig[i].mask = 0;
    }
    asm volatile(
        "movl %0, %%eax"
        :
        :"g"(r.eax)
        :"%eax"
    );
}



















/*************************** Signal Handlers  ***************************/

/**
 * Default handler for SIGNAL_DIV_ZERO
 * Default action: kill 
 */
int32_t signal_div_zero_default_handler() {
    uint32_t flags;
    cli_and_save(flags);
    {
        halt_flag = (uint32_t) 1;
        halt(255);
    }
    restore_flags(flags);

    return -1;
}

/**
 * Default handler for SIGNAL_SEGFAULT
 * Default action: kill 
 */
int32_t signal_segfault_default_handler() {
    uint32_t flags;
    cli_and_save(flags);
    {
        halt_flag = (uint32_t) 1;
        halt(255);
    }
    restore_flags(flags);

    return -1;
}

/**
 * Default handler for SIGNAL_INTERRUPT
 * Default action: kill 
 */
int32_t signal_interrupt_default_handler() {
    uint32_t flags;
    cli_and_save(flags);
    {
        halt(15);
    }
    restore_flags(flags);

    return -1;
}

/**
 * Default handler for SIGNAL_ALARM
 * Default action: ignore
 */
int32_t signal_alarm_default_handler() {
    return 0;
}

/**
 * Default handler for SIGNAL_USER1
 * Default action: ignore 
 */
int32_t signal_user1_default_handler() {
    return 0;
}
