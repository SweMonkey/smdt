#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <genesis.h>

#define STATUS_TEXT "SMDTC v0.26"

#define CHAR_RED PAL0      // 2
#define CHAR_GREEN PAL0    // 0
#define CHAR_WHITE PAL0    // 3

#define STATUS_NET_RECV_POS 37
#define STATUS_NET_SEND_POS 38
#define STATUS_KB_POS 36

//#define EMU_BUILD
#define NO_LOGGING
//#define IAC_LOGGING
//#define KB_DEBUG

extern u8 PrintDelay;
extern bool bPALSystem;

// Legacy character printing functions
void print_charXY_W(const char c, u16 x);
void print_charXY_WP(const char c, u16 x, u8 palette);

void TRM_drawChar(const u8 c, u8 x, u8 y, u8 palette);
void TRM_drawText(const char *str, u16 x, u16 y, u8 palette);
void TRM_clearTextArea(u16 x, u16 y, u16 w, u16 h);

u8 atoi(char *c);
u16 atoi16(char *c);

#endif // UTILS_H_INCLUDED
