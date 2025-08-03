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
extern u8 bKB_CapsLock;
extern u8 bKB_NumLock;
extern u8 bKB_ScrLock;

typedef bool KB_Poll_CB(u8 *data);
typedef void KB_SetLED_CB(u8 leds);

void KB_Init();
void KB_SetPoll_Func(KB_Poll_CB *cb);
void KB_SetLED_Func(KB_SetLED_CB *cb);
bool KB_Poll(u8 *data);
void KB_SetLED(u8 leds);
void KB_Interpret_Scancode(u8 scancode);

#endif // KEYBOARD_H_INCLUDED
