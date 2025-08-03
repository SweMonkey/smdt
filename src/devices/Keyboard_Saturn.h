#ifndef KEYBOARD_SATURN_H_INCLUDED
#define KEYBOARD_SATURN_H_INCLUDED

#include "DevMgr.h"

extern SM_Device DRV_KBSATURN;

bool KB_Saturn_Init();
bool KB_Saturn_Poll(u8 *data);
void KB_Saturn_SetLED(u8 leds);

#endif // KEYBOARD_SATURN_H_INCLUDED
