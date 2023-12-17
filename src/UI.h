#ifndef UI_H_INCLUDED
#define UI_H_INCLUDED

#include <genesis.h>

typedef struct s_window
{
    u8 WinBuffer[28][40];
    u8 WinAttr[28][40];
    char Title[35];
    bool bVisible;
} SM_Window;

typedef struct s_menu
{
    u8 MenuBuffer[28][20];
    bool bVisible;
    u8 SelectedIdx;
    //u8 SubLevel;
    //u8 SubParent;
    u8 Level[8];
    u8 CurLevel;
    u8 EntryCnt;
} SM_Menu;

#define UC_VLINE_SINGLE 0xB3
#define UC_VLINE_DOUBLE 0xBA
#define UC_HLINE_SINGLE 0xC4
#define UC_HLINE_DOUBLE 0xCD
#define UC_PANEL_SINGLE 0x1
#define UC_PANEL_DOUBLE 0x2

void UI_Begin(SM_Window *w);
void UI_End();
void UI_RepaintWindow();
void UI_SetWindowTitle(const char *title);
void UI_CreateWindow(SM_Window *w, const char *title);
void UI_DrawText(u8 x, u8 y, const char *text);
void UI_ClearRect(u8 x, u8 y, u8 width, u8 height);
void UI_DrawVLine(u8 x, u8 y, u8 height, u8 linechar);
void UI_DrawHLine(u8 x, u8 y, u8 width, u8 linechar);
void UI_DrawPanel(u8 x, u8 y, u8 width, u8 height, u8 linetype);
void UI_DrawVScrollbar(u8 x, u8 y, u8 height, u16 min, u16 max, u16 pos);

void UI_BeginMenu(SM_Menu *m);
void UI_EndMenu();
//void UI_AddMenuEntry(const char *text, VoidCallback *cb);
bool UI_MenuItem(const char *text, u8 x, u8 y/*, u8 level*/);
//void UI_MenuSelect(SM_Menu *m, s8 idx);

#endif // UI_H_INCLUDED
