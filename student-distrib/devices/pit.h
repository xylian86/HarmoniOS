#ifndef _PIT_H
#define _PIT_H

/* IRQ for PIT is 0 -- highest priority */
#define PIT_IRQ         0x0


/* I/O port for PIT */
#define PIT_CHANNEL_0   0x40
#define PIT_CHANNEL_1   0x41
#define PIT_CHANNEL_2   0x42
#define PIT_CMD_PORT    0x43

#define PIT_MODE_3      0x36        /* 0011 0110 use mode 3 (output to irq #0) */
#define PIT_FREQ_100HZ  11932       /* To get 100HZ (default 1193180) */


void pit_init(void);
void pit_handler(void);

#endif

