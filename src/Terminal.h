#ifndef TERMINAL_H_INCLUDED
#define TERMINAL_H_INCLUDED

#include "DevMgr.h"

#define TTY_CURSOR_UP 0
#define TTY_CURSOR_DOWN 1
#define TTY_CURSOR_LEFT 2
#define TTY_CURSOR_RIGHT 3
#define TTY_CURSOR_DUMMY 4, 0

#define D_COLUMNS_80 128
#define D_COLUMNS_40 64

// Default cursor minmaxing
#define C_YMAX_PAL 29
#define C_YMAX_NTSC 27
#define C_HTAB 4
#define C_VTAB 2
#define C_YSTART 1  // 0 if no window plane

// Default X/Y Scroll
#define D_VSCROLL 0

// Default FG/BG colors
#define CL_FG 7
#define CL_BG 0

// Fonts
#define FONT_8x8_16 0
#define FONT_4x8_8  1
#define FONT_4x8_1  2

// Modifiable variables
extern s8 sv_HSOffset;
extern u8 sv_TermType;
extern char sv_Baud[];
extern u8 vNewlineConv;
extern u8 vDoEcho;
extern u8 vLineMode;

// TTY
extern s32 sy;
extern s16 HScroll;
extern s16 VScroll;
extern u8 C_XMAX;
extern u8 C_YMAX;
extern u8 sv_bWrapAround;
extern u8 sv_TermColumns;

extern u16 sv_CBGCL;
extern u16 sv_CFG0CL;   // Custom text colour for 4x8 font
extern u16 sv_CFG1CL;   // Custom text antialiasing colour for 4x8 font 

extern const char * const TermTypeList[];

// Font
extern u8 sv_Font;
extern u8 EvenOdd;
extern u8 sv_bHighCL;

// Statistics
extern u32 RXBytes;
extern u32 TXBytes;

void TTY_Init(u8 bHardReset);
void TTY_Reset(u8 bClearScreen);
void TTY_SetColumns(u8 col);
void TTY_ReloadPalette();
void TTY_SetFontSize(u8 size);

void TTY_PrintChar(u8 c);
void TTY_PrintString(const char *str);
void TTY_ClearLine(u16 y, u16 line_count);
void TTY_ClearLineSingle(u16 y);
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
