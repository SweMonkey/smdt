#ifndef UI_H_INCLUDED
#define UI_H_INCLUDED

#include <genesis.h>

typedef struct s_window
{
    u8 WinBuffer[28][40];
    char Title[35];
    bool bVisible;
    u8 Flags;
} SM_Window;

#define UC_VLINE_SINGLE 0x93
#define UC_VLINE_DOUBLE 0x9A
#define UC_HLINE_SINGLE 0xA4
#define UC_HLINE_DOUBLE 0xAD
#define UC_PANEL_SINGLE 0x1
#define UC_PANEL_DOUBLE 0x2

#define UC_NONE 0
#define UC_NOBORDER 1

void UI_Begin(SM_Window *w);
void UI_End();
void UI_EndNoPaint();
void UI_SetVisible(SM_Window *w, bool v);
void UI_ToggleVisible(SM_Window *w);
bool UI_GetVisible(SM_Window *w);
void UI_RepaintWindow();
void UI_SetWindowTitle(const char *title);
void UI_CreateWindow(SM_Window *w, const char *title, u8 flags);
void UI_DrawText(u8 x, u8 y, const char *text);
void UI_ClearRect(u8 x, u8 y, u8 width, u8 height);
void UI_FillRect(u8 x, u8 y, u8 width, u8 height, u8 fillbyte);
void UI_DrawVLine(u8 x, u8 y, u8 height, u8 linechar);
void UI_DrawHLine(u8 x, u8 y, u8 width, u8 linechar);
void UI_DrawPanel(u8 x, u8 y, u8 width, u8 height, u8 linetype);
void UI_DrawPanelSimple(u8 x, u8 y, u8 width, u8 height);
void UI_DrawVScrollbar(u8 x, u8 y, u8 height, u16 min, u16 max, u16 pos);
void UI_DrawItemList(u8 x, u8 y, u8 width, u8 height, const char *caption, char list[][16], u16 item_count, u16 scroll);
void UI_DrawTextInput(u8 x, u8 y, u8 width, const char *caption, char str[], bool bShowCaret);

#endif // UI_H_INCLUDED
