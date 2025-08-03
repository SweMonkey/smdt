#ifndef TERMINAL_H_INCLUDED
#define TERMINAL_H_INCLUDED

#include "DevMgr.h"

// Cursor movement direction
#define TTY_CURSOR_UP 0
#define TTY_CURSOR_DOWN 1
#define TTY_CURSOR_LEFT 2
#define TTY_CURSOR_RIGHT 3
#define TTY_CURSOR_DUMMY 4, 0

// VDP tilemap width
#define D_COLUMNS_80 128
#define D_COLUMNS_40 64

// Max number of columns available
#define DCOL4_64  80//126   // Using 4x8 font with 64 wide tilemap
#define DCOL4_128 80//254   // Using 4x8 font with 128 wide tilemap
#define DCOL8_64  40//39    // Using 8x8 font with 64 wide tilemap
#define DCOL8_128 40//126   // Using 8x8 font with 128 wide tilemap

// Default cursor minmaxing
#define C_YMAX_PAL 29
#define C_YMAX_NTSC 27
#define C_HTAB 8
#define C_VTAB 2
#define C_YSTART 1  // 0 if no window plane
#define C_SYSTEM_YMAX (bPALSystem ? C_YMAX_PAL : C_YMAX_NTSC)

// Default X/Y Scroll
#define D_VSCROLL 0

// Default FG/BG colors
#define CL_FG 7
#define CL_BG 0

// Fonts
#define FONT_8x8_16 0
#define FONT_4x8_8  1
#define FONT_4x8_1  2
#define FONT_4x8_16 3


typedef enum 
{
    TF_None             = 0,
    TF_ClearScreen      = 0x1, 
    TF_ResetPlaneSize   = 0x2, 
    TF_ResetPalette     = 0x4,
    TF_ResetNetCount    = 0x8,
    TF_ReloadFont       = 0x10,

    TF_Everything       = 0xFF,
} TTY_InitFlags;

// Modifiable variables
extern s8 sv_HSOffset;
extern u8 sv_TermType;
extern char sv_Baud[];
extern u8 vNewlineConv;
extern u8 vDoEcho;
extern u8 vLineMode;
extern u8 vBackspace;

// TTY
extern s16 sy;
extern s16 HScroll;
extern s16 VScroll;
extern u8 C_XMAX;
extern u8 C_YMAX;
extern u8 C_XMIN;
extern u8 C_YMIN;
extern u8 sv_bWrapAround;
extern u8 sv_TermColumns;
extern u16 BufferSelect;
extern s16 Saved_VScroll;
extern u8 PendingWrap;

extern u16 sv_CBGCL;
extern u16 sv_CFG0CL;   // Custom text colour for 4x8 font
extern u16 sv_CFG1CL;   // Custom text antialiasing colour for 4x8 font 

extern const char * const TermTypeList[];

// Font
extern u8 sv_Font;
extern u8 sv_BoldFont;
extern u8 EvenOdd;
extern u8 sv_bHighCL;

void TTY_Init(TTY_InitFlags flags);
void TTY_SetDarkColours();
void TTY_ReloadPalette();
void TTY_SetFontSize(u8 size);

void TTY_PrintChar(u8 c);
void TTY_PrintString(const char *str);
void TTY_ClearLine(u16 y, u16 line_count);
void TTY_ClearLineSingle(u16 y);
void TTY_ClearPartialLine(u16 y, u16 from_x, u16 to_x);
void TTY_SetAttribute(u8 v);

void TTY_SetSX(s16 x);
s16 TTY_GetSX();

void TTY_SetSY_A(s16 y);    // Set SY without VScroll   (VScroll gets added in function)
s16 TTY_GetSY_A();          // Get SY without VScroll   (VScroll gets removed in function)
void TTY_SetSY(s16 y);      // Set SY + VScroll         (VScroll does NOT get added in function)
s16 TTY_GetSY();            // Get SY + VScroll         (VScroll does NOT get removed in function)

void TTY_SetVScroll(s16 v);     // Set vscroll to relative value
void TTY_SetVScrollAbs(s16 v);  // Set vscroll to absolute value
void TTY_ResetVScroll();        // Reset vscroll to default value (0)

void TTY_MoveCursor(u8 direction, u8 lines);
void TTY_DrawScrollback(u8 num);
void TTY_DrawScrollback_RI(u8 num);

u16 TTY_ReadCharacter(u8 x, u8 y);

#endif // TERMINAL_H_INCLUDED
