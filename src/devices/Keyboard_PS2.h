#ifndef KEYBOARD_PS2_H_INCLUDED
#define KEYBOARD_PS2_H_INCLUDED

#include "DevMgr.h"

extern SM_Device DRV_KBPS2;

bool KB_PS2_Init(DevPort port);
bool KB_PS2_Poll(u8 *data);
u8 KB_PS2_SendCommand(u8 cmd);

#endif // KEYBOARD_PS2_H_INCLUDED
