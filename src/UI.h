#ifndef UI_H_INCLUDED
#define UI_H_INCLUDED

#include <genesis.h>

typedef struct s_window
{
    u8 WinBuffer[30][40];
    u8 WinAttribute[30][40];
    char Title[35];
    bool bVisible;
    u8 Flags;
} SM_Window;

#define UC_NONE 0
#define UC_NOBORDER 1
#define UC_MODAL 2

#define UC_MODEL_OK_CANCEL 0
#define UC_MODEL_YES_NO 1
#define UC_MODEL_APPLY_CANCEL 2
#define UC_MODEL_ADD_REMOVE 3

typedef enum e_ItemListFlags {IL_None = 0, IL_NoBorder} ItemListFlags;

extern bool bModalWindowActive;

void UI_ApplyTheme();
void UI_Begin(SM_Window *w);
void UI_End();
void UI_EndNoPaint();
void UI_SetVisible(SM_Window *w, bool v);
void UI_ToggleVisible(SM_Window *w);
bool UI_GetVisible(SM_Window *w);
void UI_RepaintWindow();
void UI_SetWindowTitle(const char *title);
void UI_CreateWindow(SM_Window *w, const char *title, u8 flags);
void UI_DrawText(u8 x, u8 y, u8 attribute, const char *text);
void UI_ClearRect(u8 x, u8 y, u8 width, u8 height);
void UI_FillRect(u8 x, u8 y, u8 width, u8 height, u8 fillbyte);
void UI_DrawVLine(u8 x, u8 y, u8 height);
void UI_DrawHLine(u8 x, u8 y, u8 width);
void UI_DrawPanel(u8 x, u8 y, u8 width, u8 height);
void UI_DrawWindow(u8 x, u8 y, u8 width, u8 height, bool bChild, const char *title);
void UI_DrawVScrollbar(u8 x, u8 y, u8 height, u16 min, u16 max, u16 pos);
void UI_DrawItemList(u8 x, u8 y, u8 width, u8 height, char *list[], u16 item_count, u16 scroll);
void UI_DrawTextInput(u8 x, u8 y, u8 width, const char *caption, char str[], bool bShowCaret);
void UI_DrawItemListSelect(u8 x, u8 y, u8 width, u8 height, const char *caption, char *list[], u8 item_count, u8 selected_item, ItemListFlags flags);
void UI_DrawColourPicker(u8 x, u8 y, u16 *rgb, u8 selected);
void UI_DrawConfirmBox(u8 x, u8 y, u8 model, u8 selected);
void UI_DrawTabs(u8 x, u8 y, u8 w, u8 num_tabs, u8 active_tab, u8 selected, const char *tab_text[]);

#endif // UI_H_INCLUDED
