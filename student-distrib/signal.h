#ifndef _SIGNAL_H
#define _SIGNAL_H
#include "lib.h"
#include "x86_desc.h"
#include "process_crtl.h"
#include "do_syscall.h"


#define NUM_SIGNAL 5
#define SIGRETURN_VAL 10
#define RET_LENGTH 8

#define DIV_ZERO 0
#define SEGFAULT 1
#define INTERRUPT 2
#define ALARM 3
#define USER1 4

#define UNMASK 0
#define MASK 1
#define NOPENDING 0
#define PENDING 1

void sig_init(process_crtl_block_t* pcb);
void signal_raise(int32_t signum);
#endif
