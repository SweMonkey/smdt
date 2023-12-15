
#include "Input.h"
#include "QMenu.h"
#include "HexView.h"
#include "Keyboard_PS2.h"
#include "main.h"

static u8 KeyMap[0xFF];

void Input_JP(u16 joy, u16 changed, u16 state)
{
    set_KeyPress(KEY_RETURN, (changed & state & BUTTON_A)     ? KEYSTATE_DOWN : KEYSTATE_UP);
    set_KeyPress(KEY_ESCAPE, (changed & state & BUTTON_B)     ? KEYSTATE_DOWN : KEYSTATE_UP);
    set_KeyPress(KEY_RWIN,   (changed & state & BUTTON_START) ? KEYSTATE_DOWN : KEYSTATE_UP);
    set_KeyPress(KEY_UP,     (changed & state & BUTTON_UP)    ? KEYSTATE_DOWN : KEYSTATE_UP);
    set_KeyPress(KEY_DOWN,   (changed & state & BUTTON_DOWN)  ? KEYSTATE_DOWN : KEYSTATE_UP);
    set_KeyPress(KEY_LEFT,   (changed & state & BUTTON_LEFT)  ? KEYSTATE_DOWN : KEYSTATE_UP);
    set_KeyPress(KEY_RIGHT,  (changed & state & BUTTON_RIGHT) ? KEYSTATE_DOWN : KEYSTATE_UP);

    return;
}

void Input_Init()
{
    for (u16 i = 0; i < 0xFF; i++) KeyMap[i] = KEYSTATE_NONE;

    #ifdef EMU_BUILD
        JOY_setSupport(PORT_1, JOY_SUPPORT_6BTN);
        JOY_setEventHandler(Input_JP);
    #else
        JOY_setSupport(PORT_1, JOY_SUPPORT_OFF);
        KB_Init();
    #endif

    JOY_setSupport(PORT_2, JOY_SUPPORT_OFF);
}

bool is_KeyDown(u8 key)
{
    return (KeyMap[key & 0xFF] == KEYSTATE_DOWN)?TRUE:FALSE;
}

bool is_KeyUp(u8 key)
{
    return (KeyMap[key & 0xFF] == KEYSTATE_UP)?TRUE:FALSE;
}

u8 get_KeyPress(u8 KeyState)
{
    return 0;//KeyMap[KeyState & 0xFF];
}

void set_KeyPress(u8 key, u8 KeyState)
{
    KeyMap[key & 0xFF] = KeyState;
}

void InputTick()
{
    QMenu_Input();
    HexView_Input();

    for (u16 i = 0; i < 0xFF; i++)
    {
        if (KeyMap[i & 0xFF] == KEYSTATE_UP) KeyMap[i & 0xFF] = KEYSTATE_NONE;
        if (KeyMap[i & 0xFF] == KEYSTATE_DOWN) KeyMap[i & 0xFF] = KEYSTATE_NONE;
    }
}