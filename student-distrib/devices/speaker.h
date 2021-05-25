#ifndef _SPEAKER_H
#define _SPEAKER_H

#include "../types.h"

#define PIT_FREQ                    1193180
#define KEYBOARD_CONTROLLER_PORT    0x61
#define SPEAKER_CMD_PORT            0x43        // cmd port in PIT
#define SPEAKER_DATA_PORT           0x42        // channel 2 in PIT
#define SPEAKER_MUTE_MASK           0xFC        // 1011 0110 use mode 3 (output to irq #0)
#define SPEAKER_UNMUTE_MASK         0x03
#define SPEAKER_MODE                0xB6

void speaker_play_sound(uint32_t freq);
void speaker_no_sound();
void speaker_beep();

#endif
