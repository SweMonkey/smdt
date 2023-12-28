#ifndef TERMINAL_H_INCLUDED
#define TERMINAL_H_INCLUDED

#include "DevMgr.h"

#define TTY_CURSOR_UP 0
#define TTY_CURSOR_DOWN 1
#define TTY_CURSOR_LEFT 2
#define TTY_CURSOR_RIGHT 3
#define TTY_CURSOR_DUMMY 4, 0

#define ICO_NET_SEND 0x1A       // Up arrow
#define ICO_NET_RECV 0x1B       // Down arrow
#define ICO_NET_IDLE_SEND 0x18
#define ICO_NET_IDLE_RECV 0x19
#define ICO_NET_ERROR 0x1D

#define D_COLUMNS_80 128
#define D_COLUMNS_40 64

// Default cursor minmaxing
#define C_YMAX_PAL 29
#define C_YMAX_NTSC 27
#define C_HTAB 4
#define C_VTAB 2
#define C_YSTART 1  // 0 if no window plane

// Default X/Y Scroll
#define D_HSCROLL 0    // Old default: 9
#define D_VSCROLL 0

// Default FG/BG colors
#define CL_FG 7
#define CL_BG 0

// Tx flags
#define TXF_NOBUFFER 1

// Modifiable variables
extern u8 vNewlineConv;
extern u8 vTermType;
extern u8 vDoEcho;
extern u8 vLineMode;
extern char *vSpeed;

// Telnet flags
extern u8 vtEcho;

// TTY
extern u8 TTY_Initialized;
extern s32 sy;
extern s16 HScroll;
extern s16 VScroll;
extern u8 C_XMAX;
extern u8 C_YMAX;
extern u8 bWrapAround;
extern u8 TermColumns;

// Font
extern u8 FontSize;
extern u8 EvenOdd;

// Statistics
extern u32 RXBytes;
extern u32 TXBytes;

void TTY_Init(u8 bHardReset);
void TTY_Reset(u8 bClearScreen);
void TTY_SetColumns(u8 col);
void TTY_SetFontSize(u8 size);
void TTY_SendChar(const u8 c, u8 flags);
void TTY_TransmitBuffer();
void TTY_SendString(const char *str);
void TTY_PrintChar(u8 c);
void TTY_ClearLine(u16 y, u16 line_count);
void TTY_ClearPartialLine(u16 y, u16 from_x, u16 to_x);
void TTY_SetAttribute(u8 v);

void TTY_SetSX(s32 x);
s32 TTY_GetSX();

void TTY_SetSY_A(s32 x);    // Set SY without VScroll   (VScroll gets added in function)
s32 TTY_GetSY_A();          // Get SY without VScroll   (VScroll gets removed in function)
void TTY_SetSY(s32 y);      // Set SY + VScroll         (VScroll does NOT get added in function)
s32 TTY_GetSY();            // Get SY + VScroll         (VScroll does NOT get removed in function)

void TTY_MoveCursor(u8 direction, u8 lines);

#endif // TERMINAL_H_INCLUDED
