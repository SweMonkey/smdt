#ifndef KEYBOARD_PS2_H_INCLUDED
#define KEYBOARD_PS2_H_INCLUDED

#include "main.h"

#define KB_PORT_DATA PORT1_DATA
#define KB_PORT_CTRL PORT1_CTRL

#define ICO_KB_UNKNOWN 0x1F //0x3F
#define ICO_KB_OK 0x1C      //0x4B
#define ICO_KB_ERROR 0x1D   //0x58

extern u8 KB_Initialized;

void KB_Init();
u8 KB_Poll(u8 *data);
void KB_Handle_Scancode(u8 scancode);
void KB_Handle_EXT_Scancode(u8 scancode);  // Handles extended keys

#endif // KEYBOARD_PS2_H_INCLUDED
