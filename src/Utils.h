#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <genesis.h>

#define STATUS_TEXT "SMDTC v0.27"

#define CHAR_RED PAL0      // 2
#define CHAR_GREEN PAL0    // 0
#define CHAR_WHITE PAL0    // 3

// Icon positions (Y)
#define STATUS_NET_RECV_POS 37  // Rx
#define STATUS_NET_SEND_POS 38  // Tx
#define STATUS_ID_POS 36        // Input device

//#define EMU_BUILD
#define NO_LOGGING
//#define IAC_LOGGING
//#define KB_DEBUG

// Check if a character is printable
#define isPrintable(x) ((x != '\n')&&(x))

extern u8 PrintDelay;
extern bool bPALSystem;

void TRM_SetStatusText(const char *t);
void TRM_ResetStatusText();
void TRM_SetWinHeight(u8 h);
void TRM_SetWinWidth(u8 w);
void TRM_SetWinParam(bool from_bottom, bool from_right, u8 w, u8 h);
void TRM_ResetWinParam();

void TRM_SetStatusIcon(const char icon, u16 pos, u8 palette);
void TRM_drawChar(const u8 c, u8 x, u8 y, u8 palette);
void TRM_drawText(const char *str, u16 x, u16 y, u8 palette);
void TRM_clearTextArea(u16 x, u16 y, u16 w, u16 h);

u8 atoi(char *c);
u16 atoi16(char *c);

u16 snprintf(char *buffer, u16 size, const char *fmt, ...) __attribute__ ((format (printf, 3, 4)));

#endif // UTILS_H_INCLUDED
