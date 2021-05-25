/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "../lib.h"

#define MASK_ALL    0xFF

/* Interrupt masks to determine which interrupts are enabled and disabled */
// uint8_t master_mask = MASK_ALL; /* IRQs 0-7  */
// uint8_t slave_mask = MASK_ALL;  /* IRQs 8-15 */

/* reference: the initialization on the slide */
/* Initialize the 8259 PIC */
/*
 * i8259_init
 *   DESCRIPTION: Initialize the master and slave PICs in order to handle the interrupts.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void i8259_init(void) {

    master_mask = MASK_ALL;
    slave_mask = MASK_ALL;

    outb(MASK_ALL, MASTER_8259_DATA);               // 0xff -- mask all of 8259A-1
    outb(MASK_ALL, SLAVE_8259_DATA);                // 0xff -- mask all of 8259A-2

    /* Initialize master pic */
    outb(ICW1, MASTER_8259_CMD);                // ICW1: select master init
    outb(ICW2_MASTER, MASTER_8259_DATA);        // ICW2: master pic IR 0-7 mapped to 0x20-0x27
    outb(ICW3_MASTER, MASTER_8259_DATA);        // ICW3: indicates the slave
    outb(ICW4, MASTER_8259_DATA);               // master expects normal EOI

    /* Initialize slave pic */
    outb(ICW1, SLAVE_8259_CMD);                 // ICW1: select slave init
    outb(ICW2_SLAVE, SLAVE_8259_DATA);          // ICW2: slave pic IR 0-7 mapped to 0x28-0x2f
    outb(ICW3_SLAVE, SLAVE_8259_DATA);          // ICW3: indicates the slave on master's IR2
    outb(ICW4, SLAVE_8259_DATA);

    outb(master_mask, MASTER_8259_DATA);
    outb(slave_mask, SLAVE_8259_DATA);
    /* Enable the slave's interrupts, 2 on the master */
    enable_irq(SLAVE_IRQ);
}

/* reference: https://wiki.osdev.org/8259_PIC, http://www.brokenthorn.com/Resources/OSDevPic.html */
/* Enable (unmask) the specified IRQ */
/*
 * enable_irq
 *   DESCRIPTION: Enable the IRQ specified by irq_num.
 *   INPUTS: irq_num -- the IRQ to be enabled, range from 0 to 15, 0-7 on master PIC, 8-15 on slave PIC
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: The corresponding IRQ is unmasked (set to 0), and the mask is output to corresponding port
 */
void enable_irq(uint32_t irq_num) {

    if (irq_num >= I8259_TOTAL_IRQ * 2) {
        printf("Unknown IRQ \n");
        return;
    }

    /* if it is on the master pic */
    if (irq_num < I8259_TOTAL_IRQ) {
        /* unmask the corresponding bit <=> (irq_num)th bit from right */
        master_mask &= (~(1 << irq_num));
        /* output to the master data port */
        outb(master_mask, MASTER_8259_DATA);
    }
    /* if it is on the slave pic */
    else {
        irq_num = irq_num - I8259_TOTAL_IRQ;
        /* unmask the corresponding bit <=> (irq_num)th bit from right */
        slave_mask &= (~ (1 << (irq_num)));
        /* output to the slave data port */
        outb(slave_mask, SLAVE_8259_DATA);
    }
    // //irq_num range from 8-15 is on slave
    // if (irq_num >= I8259_TOTAL_IRQ){
    //     slave_mask &= ~(1 << (irq_num - I8259_TOTAL_IRQ));       
    //     outb(slave_mask, SLAVE_8259_DATA);
    // }
    // //irq_num range from 0-7 is on master
    // else{
    //     master_mask &= ~(1 << irq_num);
    //     outb(master_mask, MASTER_8259_DATA);
    // }
}

/* reference: https://wiki.osdev.org/8259_PIC, http://www.brokenthorn.com/Resources/OSDevPic.html */
/* Disable (mask) the specified IRQ */
/*
 * disable_irq
 *   DESCRIPTION: Disable the IRQ specified by irq_num.
 *   INPUTS: irq_num -- the IRQ to be disabled, range from 0 to 15, 0-7 on master PIC, 8-15 on slave PIC
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: The corresponding IRQ is masked (set to 1), and the mask is output to corresponding port
 */
void disable_irq(uint32_t irq_num) {

    if (irq_num >= I8259_TOTAL_IRQ * 2) {
        printf("Unknown IRQ \n");
        return;
    }
    
    /* if it is on the master pic */
    if (irq_num < I8259_TOTAL_IRQ) {
        /* mask the corresponding bit <=> (irq_num)th bit from right */
        master_mask = master_mask | (1 << irq_num);
        /* output to the master data port */
        outb(master_mask, MASTER_8259_DATA);
    }
    /* if it is on the slave pic */
    else {
        irq_num -= I8259_TOTAL_IRQ;
        /* mask the corresponding bit <=> (irq_num)th bit from right */
        slave_mask = slave_mask | (1 << irq_num);
        /* output to the slave data port */
        outb(slave_mask, SLAVE_8259_DATA);
    }
}

/* Send end-of-interrupt signal for the specified IRQ */
/*
 * send_eoi
 *   DESCRIPTION: Send EOI signal. 
 *                If just the master PIC, send to master POI; 
 *                if both master and slave PIC, first send to slave then send to master
 *   INPUTS: irq_num -- the IRQ to send the EOI, range from 0 to 15, 
 *                      0-7 on master PIC, 8-15 on slave PIC
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: PIC reveives the EOI.
 */
void send_eoi(uint32_t irq_num) {
    uint8_t eoi_signal = EOI;
    /* if it is on the master pic */
    if (irq_num < I8259_TOTAL_IRQ) {
        eoi_signal |= irq_num;
        /* output eoi to the master command port */
        outb(eoi_signal, MASTER_8259_CMD);
    }
    /* if it is on the slave pic */
    else {
        /* output eoi to the slave command port */
        eoi_signal |= irq_num - I8259_TOTAL_IRQ;
        outb(eoi_signal, SLAVE_8259_CMD);
        /* output eoi to the master command port */
        eoi_signal = EOI | SLAVE_IRQ;
        outb(eoi_signal, MASTER_8259_CMD);
    }
}

