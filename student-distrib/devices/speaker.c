#include "speaker.h"
#include "../lib.h"
#include "rtc.h"

//Code reference: https://wiki.osdev.org/Text_Mode_Cursor


uint32_t tone[7] = {262, 294, 330, 349, 392, 440, 494};


/*
 * speaker_play_sound
 *   DESCRIPTION: Enabling the speaker with given frequency by setting the PIT frequency 
 *                and mask the keyboard to "out" position.
 *   INPUTS: freq - frequency to be set for PC speaker.
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Set frequency and enable PC speaker.
 */
void speaker_play_sound(uint32_t freq) {
    uint32_t divisor;
    uint8_t sound;

    // Set the PIT to the desired frequency at channel 2
    divisor = PIT_FREQ / freq;
    outb(SPEAKER_MODE, SPEAKER_CMD_PORT);
    outb((uint8_t) (divisor), SPEAKER_DATA_PORT);
    outb((uint8_t) (divisor >> 8), SPEAKER_DATA_PORT);

    // play the sound using the PC speaker, mask with unmute (set bit 0 and bit 1 to 1)
    sound = inb(KEYBOARD_CONTROLLER_PORT);
    if (sound != (sound | SPEAKER_UNMUTE_MASK)) {
        outb(sound | SPEAKER_UNMUTE_MASK, KEYBOARD_CONTROLLER_PORT);
    }
}

/*
 * speaker_no_sound
 *   DESCRIPTION: Disable the PC speaker, mask the keyboard to "in" position.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Disable PC speaker.
 */
void speaker_no_sound() {
    uint8_t mute = inb(KEYBOARD_CONTROLLER_PORT) & SPEAKER_MUTE_MASK;
    outb(mute, KEYBOARD_CONTROLLER_PORT);
}

/*
 * beep
 *   DESCRIPTION: Make the PC speaker to produce "beep" sound by adjusting the "in" and "out" position.
 *   INPUTS: freq - frequency to be set for PC speaker.
 *   OUTPUTS: none
 *   RETURN VALUE: none
 */
void speaker_beep() {
    // speaker_play_sound(400);
    // // timer_wait(10);
    // int32_t i = 0;
    // while (i < 10) {
    //     i++;
    // }
    // speaker_no_sound();
    // //set_PIT_2(old_frequency);
    int32_t i = 0;
    uint32_t freq = tone[0];
    for (i = 0; i < 42; i++){
        freq = tone[i%7];
        rtc_beep = 0;
        while(rtc_beep < 128) {}
        if (i % 2 == 0) speaker_play_sound(freq);
        else speaker_no_sound();
    }
}
