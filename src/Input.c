#include "Input.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "Utils.h"
#include "Screensaver.h"
#include "WinMgr.h"

#define KM_SZ 0x1FF

static u8 KeyMap[KM_SZ+1];
static u16 JoyLastState;
void QMenu_Input();
void HexView_Input();
void FavView_Input();


void Input_JP(u16 joy, u16 changed, u16 state)
{
    set_KeyPress(KEY_RETURN,    BTN_STATE(state, JoyLastState, BUTTON_A));
    set_KeyPress(KEY_ESCAPE,    BTN_STATE(state, JoyLastState, BUTTON_B));
    set_KeyPress(KEY_BACKSPACE, BTN_STATE(state, JoyLastState, BUTTON_C));
    set_KeyPress(KEY_RWIN,      BTN_STATE(state, JoyLastState, BUTTON_START));
    set_KeyPress(KEY_UP,        BTN_STATE(state, JoyLastState, BUTTON_UP));
    set_KeyPress(KEY_DOWN,      BTN_STATE(state, JoyLastState, BUTTON_DOWN));
    set_KeyPress(KEY_LEFT,      BTN_STATE(state, JoyLastState, BUTTON_LEFT));
    set_KeyPress(KEY_RIGHT,     BTN_STATE(state, JoyLastState, BUTTON_RIGHT));
    
    JoyLastState = state;

    return;
}

void Input_Init()
{
    memset(KeyMap, KEYSTATE_NONE, KM_SZ);

    JoyLastState = 0;

    JOY_setSupport(PORT_1, JOY_SUPPORT_OFF);
    JOY_setSupport(PORT_2, JOY_SUPPORT_OFF);

    KB_Init();
    Mouse_Init();
}

inline bool is_KeyDown(u16 key)
{
    return KeyMap[key & KM_SZ] == KEYSTATE_DOWN;
}

inline bool is_KeyUp(u16 key)
{
    return KeyMap[key & KM_SZ] == KEYSTATE_UP;
}

inline bool is_AnyKey()
{
    for (u16 i = 0; i < KM_SZ; i++)
    {
        if (KeyMap[i] == KEYSTATE_DOWN) return TRUE;
    }

    return FALSE;
}

u8 get_KeyPress(u16 key)
{
    return KeyMap[key & KM_SZ];
}

inline void set_KeyPress(u16 key, u8 KeyState)
{
    if (KeyMap[key & KM_SZ] == KEYSTATE_NONE) KeyMap[key & KM_SZ] = KeyState;
    else if (key > 0x1EF) KeyMap[key & KM_SZ] = KeyState;   // Don't remember the reason why the above if excludes KEYSTATE_NONE, but to play it safe I'll just leave it alone and add this line for mouse state change to NONE

    InactiveCounter = -1;
}

inline void InputTick()
{
    WinMgr_Input();

    memset(KeyMap, KEYSTATE_NONE, KM_SZ - 0xF); // Don't touch the last 16 states (mouse code will take care of it)
}
