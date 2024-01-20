#ifndef KEYBOARD_PS2_H_INCLUDED
#define KEYBOARD_PS2_H_INCLUDED

#include "DevMgr.h"

extern u8 KB_Initialized;
extern const u8 SCTable_US[3][128];

void KB_Init();
u8 KB_Poll(u8 *data);
void KB_Interpret_Scancode(u8 scancode);

void KB_SendCommand(u8 cmd);

#endif // KEYBOARD_PS2_H_INCLUDED
