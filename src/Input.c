
#include "Input.h"
#include "Keyboard_PS2.h"
#include "Utils.h"

#define KM_SZ 0x1FF
static u8 KeyMap[KM_SZ];

void QMenu_Input();
void HexView_Input();

void Input_JP(u16 joy, u16 changed, u16 state)
{
    set_KeyPress(KEY_RETURN,   (changed & state & BUTTON_A)     ? KEYSTATE_DOWN : KEYSTATE_UP);
    set_KeyPress(KEY_ESCAPE,   (changed & state & BUTTON_B)     ? KEYSTATE_DOWN : KEYSTATE_UP);
    set_KeyPress(KEY_BACKSPACE,(changed & state & BUTTON_C)     ? KEYSTATE_DOWN : KEYSTATE_UP);
    set_KeyPress(KEY_RWIN,     (changed & state & BUTTON_START) ? KEYSTATE_DOWN : KEYSTATE_UP);
    set_KeyPress(KEY_UP,       (changed & state & BUTTON_UP)    ? KEYSTATE_DOWN : KEYSTATE_UP);
    set_KeyPress(KEY_DOWN,     (changed & state & BUTTON_DOWN)  ? KEYSTATE_DOWN : KEYSTATE_UP);
    set_KeyPress(KEY_LEFT,     (changed & state & BUTTON_LEFT)  ? KEYSTATE_DOWN : KEYSTATE_UP);
    set_KeyPress(KEY_RIGHT,    (changed & state & BUTTON_RIGHT) ? KEYSTATE_DOWN : KEYSTATE_UP);

    return;
}

void Input_Init()
{
    memset(KeyMap, KEYSTATE_NONE, KM_SZ);

    #ifdef EMU_BUILD
        JOY_setSupport(PORT_1, JOY_SUPPORT_6BTN);
        JOY_setEventHandler(Input_JP);
    #else
        JOY_setSupport(PORT_1, JOY_SUPPORT_OFF);
        KB_Init();
    #endif

    JOY_setSupport(PORT_2, JOY_SUPPORT_OFF);
}

bool is_KeyDown(u16 key)
{
    return (KeyMap[key & KM_SZ] == KEYSTATE_DOWN)?TRUE:FALSE;
}

bool is_KeyUp(u16 key)
{
    return (KeyMap[key & KM_SZ] == KEYSTATE_UP)?TRUE:FALSE;
}

u16 get_KeyPress(u8 KeyState)
{
    return 0;//KeyMap[KeyState & KM_SZ];
}

void set_KeyPress(u16 key, u8 KeyState)
{
    KeyMap[key & KM_SZ] = KeyState;
}

void InputTick()
{
    QMenu_Input();
    HexView_Input();

    memset(KeyMap, KEYSTATE_NONE, KM_SZ);
}
