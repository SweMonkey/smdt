#ifndef QMENU_H_INCLUDED
#define QMENU_H_INCLUDED

#include "main.h"

extern bool bShowQMenu;

void QMenu_Input();
void DrawMenu(u8 idx);
void EnterMenu();
void ExitMenu();
void UpMenu();
void DownMenu();
void QMenu_Toggle();
void ChangeText(u8 menu_idx, u8 entry_idx, const char *new_text);

#endif // QMENU_H_INCLUDED
