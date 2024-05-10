#ifndef INPUT_H_INCLUDED
#define INPUT_H_INCLUDED

#include <genesis.h>
#include "misc/KeyDef.h"

bool is_KeyDown(u16 key);
bool is_KeyUp(u16 key);
u16 get_KeyPress(u8 KeyState);
void set_KeyPress(u16 key, u8 KeyState);

void Input_JP(u16 joy, u16 changed, u16 state);
void Input_Init();
void InputTick();

#endif // INPUT_H_INCLUDED
