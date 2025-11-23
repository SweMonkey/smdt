#ifndef QMENU_H_INCLUDED
#define QMENU_H_INCLUDED

#include <genesis.h>

extern u8 sv_QBGCL;
extern u8 sv_QFGCL;
extern u8 sv_QCURCL;

void QMenu_Input();
void SetupQItemTags();
u16 QMenu_Open();
void QMenu_Close();
void ChangeText(u8 menu_idx, u8 entry_idx, const char *new_text);

#endif // QMENU_H_INCLUDED
