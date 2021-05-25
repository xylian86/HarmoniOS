#ifndef DO_SYSCALL_H
#define DO_SYSCALL_H

#include "types.h"
#include "filesystem/filesys.h"

#define EXCEPTION_HANDLER 256
#define MAX_FDA           8
#define MIN_FDA           0
#define EXECTURE_BUF_SIZE 4

#define USR_VIDMEM_ADDR 0x10000000

extern file_ops_table_t empty_op;

int32_t halt (uint8_t status);
int32_t execute (const int8_t* command);
int32_t read(int32_t fd, void* buf, int32_t nbytes);
int32_t write(int32_t fd, const void* buf, int32_t nbytes);
int32_t open(const uint8_t* filename);
int32_t close(int32_t fd);
int32_t getargs(uint8_t* buf, int32_t nbytes);
int32_t vidmap(uint8_t** screen_start);
int32_t set_handler(int32_t signum, void* handler_address);
int32_t sigreturn(void);
int32_t new_poke(uint32_t pos, uint32_t data);
int32_t beep(void);
int32_t ps(void);
int32_t random(void);

#endif
