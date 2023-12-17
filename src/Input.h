#ifndef INPUT_H_INCLUDED
#define INPUT_H_INCLUDED

#include <genesis.h>
#include "KeyDef.h"

bool is_KeyDown(u8 key);
bool is_KeyUp(u8 key);
u8 get_KeyPress(u8 KeyState);
void set_KeyPress(u8 key, u8 KeyState);

void Input_Init();
void InputTick();

#endif // INPUT_H_INCLUDED
