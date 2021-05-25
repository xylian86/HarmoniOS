#ifndef STATUS_BAR_H
#define STATUS_BAR_H


#define STATUS_BAR_MESSAGE_START 0
#define STATUS_BAR_MESSAGE_END 60
#define CONTROL_BLOCK_END      30

#define STATUS_BAR_TIMER_START 60
#define STATUS_BAR_TIMER_END   80

#define TIMER_BUF_LEN          20

#define STATUS_BAR_HEIGHT       25
#define FOUR_OFFSET             4


#define PARM_YELLOW_ON_BLACK 0x0e
#define PARM_RED_ON_BLACK 0x0c
#define PARM_WHITE_ON_BLUE 0x1f
#define PARM_BLACK_ON_WHITE 0xf0
#define PARM_PUP_ON_BLACK 0x0d

#define PARM_1_ON_BLACK 0x0a
#define PARM_2_ON_BLACK 0x0b
#define PARM_3_ON_BLACK 0x0c
void swtich_terminal_for_sb();
void message_update_for_sb(char* message, uint32_t len, uint8_t param);
void clock_update_for_sb();

// updated version: draw terminal icon
#define TERMINAL_ICON_BLOCK_DIM     16
void draw_terminal_icon(void);

#endif
