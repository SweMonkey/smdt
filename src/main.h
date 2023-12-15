#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include <genesis.h>

#define STATUS_TEXT "SMDTC v0.25"

// Port Addresses
#define PORT1_DATA 0xA10003
#define PORT1_CTRL 0xA10009

#define PORT1_SCTRL 0xA10013
#define PORT1_SRx 0xA10011
#define PORT1_STx 0xA1000F


#define PORT2_DATA 0xA10005
#define PORT2_CTRL 0xA1000B

#define PORT2_SCTRL 0xA10019
#define PORT2_SRx 0xA10017
#define PORT2_STx 0xA10015

#define CHAR_RED PAL0      // 2
#define CHAR_GREEN PAL0    // 0
#define CHAR_WHITE PAL0    // 3

#define STATUS_NET_RECV_POS 37
#define STATUS_NET_SEND_POS 38
#define STATUS_KB_POS 36

#define EMU_BUILD
#define NO_LOGGING
#define IAC_LOGGING
//#define KB_DEBUG

extern u8 PrintDelay;
extern bool bPALSystem;

void print_charXY_W(const char c, u16 x);
void print_charXY_WP(const char c, u16 x, u8 palette);

void TRM_drawChar(const u8 c, u8 x, u8 y, u8 palette);
void TRM_drawText(const char *str, u16 x, u16 y, u8 palette);
void TRM_clearTextArea(u16 x, u16 y, u16 w, u16 h);

#endif // MAIN_H_INCLUDED
