#ifndef UI_H_INCLUDED
#define UI_H_INCLUDED

#include <genesis.h>

typedef struct
{
    u8 WinBuffer[30][40];
    u8 WinAttribute[30][40];
    char Title[35];
    bool bVisible;
    u8 Flags;
} SM_Window;

typedef enum {WF_None = 0, WF_NoBorder = 1, WF_Modal = 2} UI_WindowFlags;
typedef enum {CM_Ok_Cancel, CM_Yes_No, CM_Apply_Cancel, CM_Add_Remove} UI_ConfirmModel;
typedef enum {IL_None = 0, IL_NoBorder = 1} UI_ItemListFlags;

extern bool bModalWindowActive;

void UI_ApplyTheme();

void UI_Begin(SM_Window *w);
void UI_End();
void UI_EndNoPaint();

void UI_SetVisible(SM_Window *w, bool v);
void UI_ToggleVisible(SM_Window *w);
bool UI_GetVisible(SM_Window *w);

void UI_RepaintWindow();
void UI_RepaintRow(u8 row, u8 num);
void UI_RepaintColumn(u8 column, u8 num);
void UI_RepaintTile(u8 x, u8 y);

void UI_SetWindowTitle(const char *title);
void UI_CreateWindow(SM_Window *w, const char *title, UI_WindowFlags flags);

void UI_DrawText(u8 x, u8 y, u8 attribute, const char *text);
void UI_ClearRect(u8 x, u8 y, u8 width, u8 height);
void UI_FillRect(u8 x, u8 y, u8 width, u8 height, u8 fillbyte);
void UI_SetTile(u8 x, u8 y, u8 tile);
void UI_FillAttributeRect(u8 x, u8 y, u8 width, u8 height, u8 attribute);
void UI_FillAttributeRow(u8 x1, u8 x2, u8 y, u8 attribute);
void UI_SetAttribute(u8 x, u8 y, u8 attribute);
void UI_DrawVLine(u8 x, u8 y, u8 height);
void UI_DrawHLine(u8 x, u8 y, u8 width);
void UI_DrawPanel(u8 x, u8 y, u8 width, u8 height);
void UI_DrawGroupBox(u8 x, u8 y, u8 width, u8 height, const char *caption);
void UI_DrawWindow(u8 x, u8 y, u8 width, u8 height, bool bChild, const char *title);
void UI_DrawVScrollbar(u8 x, u8 y, u8 height, u8 selected, u16 min, u16 max, u16 pos);
void UI_DrawItemList(u8 x, u8 y, u8 width, u8 height, char *list[], u16 item_count, u16 scroll);
void UI_DrawTextInput(u8 x, u8 y, u8 width, const char *caption, char str[], bool bShowCaret);
void UI_DrawItemListSelect(u8 x, u8 y, u8 width, u8 height, const char *caption, char *list[], u8 item_count, u8 selected_item, UI_ItemListFlags flags);
void UI_DrawColourPicker(u8 x, u8 y, u16 *rgb, u8 selected);
void UI_DrawConfirmBox(u8 x, u8 y, UI_ConfirmModel model, u8 selected);
void UI_DrawTabs(u8 x, u8 y, u8 w, u8 num_tabs, u8 active_tab, u8 selected, const char * const tab_text[]);
void UI_DrawHProgressBar(u8 x, u8 y, u8 max, u8 value);

#endif // UI_H_INCLUDED
