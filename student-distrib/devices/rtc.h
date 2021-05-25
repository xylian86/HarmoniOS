#ifndef _RTC_H
#define _RTC_H

#include "../types.h"

#define RTC_DATA_COUNT 100
#define MACCOUNT    512     // count used for the rtc_interrupt interval
#define RTC_SR_A    0x8A    // RTC status register A
#define RTC_SR_B    0x8B    // RTC status register B     
#define RTC_SR_C    0x8C    // RTC status register C
#define IRQ_RTC     8       // number of PIC number for the RTC
#define RTC_IO      0x70    // RIO I/O port
#define CMOS_IO     0x71    // CMOS I/O port

#define LOWER_MASK  0xF0    // clear lower 4 bits
#define SIXTH_BIT  0x40     // get 6th bit
#define RATE_SET    0x06    // set rate

#define RTC_MESSAGE_LEN 30  //length of message string.

// Initialize the RTC interrupt. The entry is 8.
extern int32_t rtc_init();
// using as rtc count, for our printing test
extern volatile int32_t rtc_count[3];
extern int32_t rtc_beep;
// set the RTC interrupt to 2HZ, which corresponding to 512 counts
extern int32_t rtc_open();
extern int32_t rtc_open_intf(const uint8_t* filename);
// close rtc
extern int32_t rtc_close();
extern int32_t rtc_close_intf(int32_t* dummy);
// wait for one rtc period, similiar to sleep()
extern int32_t rtc_read();
extern int32_t rtc_read_intf(int32_t fd, uint32_t* offset, void* buf, int32_t nbytes);
// set the rtc frequence to the required one
extern int32_t rtc_write(int32_t freq);
extern int32_t rtc_write_intf(int32_t fd, const void* buf, int32_t nbytes);
//rtc interrupt function
extern void rtc_interrupt();
#endif

