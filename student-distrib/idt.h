/* idt.h 
 */

#ifndef _IDT_H
#define _IDT_H

// init idt function
#define MAGIC_HALT 0x0F
extern void init_idt();
extern int32_t halt_flag;

#endif
