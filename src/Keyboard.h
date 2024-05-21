#ifndef KEYBOARD_H_INCLUDED
#define KEYBOARD_H_INCLUDED

#include <genesis.h>

extern u8 sv_KeyLayout;        // Selected keyboard layout
extern u8 vKB_BATStatus;
extern u8 bKB_ExtKey;
extern u8 bKB_Break;
extern u8 bKB_Shift;
extern u8 bKB_Alt;
extern u8 bKB_Ctrl;

typedef u8 KB_Poll_CB(u8 *data);

void KB_Init();
void KB_SetKeyboard(KB_Poll_CB *cb);
u8 KB_Poll(u8 *data);
void KB_Interpret_Scancode(u8 scancode);

#endif // KEYBOARD_H_INCLUDED
