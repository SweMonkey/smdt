#ifndef KEYBOARD_SATURN_H_INCLUDED
#define KEYBOARD_SATURN_H_INCLUDED

#include "DevMgr.h"

extern SM_Device DRV_KBSATURN;

bool KB_Saturn_Init();
u8 KB_Saturn_Poll(u8 *data);

#endif // KEYBOARD_SATURN_H_INCLUDED
