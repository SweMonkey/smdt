#include "Mouse.h"
#include "Input.h"
#include "Utils.h"
#include "Screensaver.h"
#include "devices/MegaMouse.h"

#define MOUSE_SPEED 1

#define MOUSE_0         0
#define MOUSE_LEFT      0x1
#define MOUSE_RIGHT     0x2
#define MOUSE_MIDDLE    0x4
#define MOUSE_C         0x8

/*  single tick "DOWN" state instead of continuous DOWN state for as long as its held down
#define BTN_STATE(curr, last, mask) \
    ((!(last & (mask)) &&  (curr & (mask))) ? KEYSTATE_DOWN : \
      ((last & (mask)) && !(curr & (mask))) ? KEYSTATE_UP : KEYSTATE_NONE)*/

#define BTN_STATE(curr, last, mask) \
    ((curr & mask) ? KEYSTATE_DOWN :\
    ((last & mask) ? KEYSTATE_UP : KEYSTATE_NONE))

static u16 MouseButtons, MouseLastState;
static s16 MouseX = 156, MouseY = 108;
u8 sv_PointerStyle = 1;

// TODO: 1. Save these variables - 2. Make them user configurable
u16 sv_MBind_Click    = MOUSE_LEFT_BTN;
u16 sv_MBind_AltClick = MOUSE_RIGHT_BTN;
u16 sv_MBind_Menu     = MOUSE_MIDDLE_BTN;
u16 sv_MBind_Function = MOUSE_C_BTN;


void Mouse_Init()
{
    MouseButtons = MouseLastState = 0;
    MouseX = 156;
    MouseY = 108;

    u16 ps = (sv_PointerStyle ? 0x6000 : 0x2000) | 0x8017;  // 0x6000 = PAL3, 0x2000 = PAL1, 0x8000 = High prio, 0x17 = pointer tile

    SetSprite_Y(SPRITE_ID_POINTER, 0);
    SetSprite_SIZELINK(SPRITE_ID_POINTER, SPR_SIZE_1x1, 0); // Redundant; This is also setup during startup in main.c
    SetSprite_TILE(SPRITE_ID_POINTER, ps);
    SetSprite_X(SPRITE_ID_POINTER, 0);
}

void Mouse_Poll()
{
    s16 dx = 0, dy = 0;

    if (MM_Mouse_Poll(&dx, &dy, &MouseButtons))
    {
        MouseX += (dx > 1) | (dx < -1) ? dx >> MOUSE_SPEED : dx;
        MouseY -= (dy > 1) | (dy < -1) ? dy >> MOUSE_SPEED : dy;

        if (MouseX < 0) MouseX = 0;
        if (MouseX > 318) MouseX = 318;

        if (MouseY < 0) MouseY = 0;
        if (MouseY > 222) MouseY = 222;

        set_KeyPress(MOUSE_LEFT_BTN,   BTN_STATE(MouseButtons, MouseLastState, MOUSE_LEFT));
        set_KeyPress(MOUSE_RIGHT_BTN,  BTN_STATE(MouseButtons, MouseLastState, MOUSE_RIGHT));
        set_KeyPress(MOUSE_MIDDLE_BTN, BTN_STATE(MouseButtons, MouseLastState, MOUSE_MIDDLE));
        set_KeyPress(MOUSE_C_BTN,      BTN_STATE(MouseButtons, MouseLastState, MOUSE_C));

        //kprintf("MM: %u - state: %u - now: %04X - last: %04X", get_KeyPress(MOUSE_MIDDLE_BTN), BTN_STATE(MouseButtons, MouseLastState, MOUSE_MIDDLE), MouseButtons, MouseLastState);

        if (MouseX || MouseY)
        {
            SetSprite_X(SPRITE_ID_POINTER, MouseX+128);
            SetSprite_Y(SPRITE_ID_POINTER, MouseY+128);
            InactiveCounter = -1;   // Reset screensaver counter
        }

        MouseLastState = MouseButtons;
    }
}

u16 Mouse_GetRect(const MRect *r)
{
    u8 i = 0;
    while (r[i].x != 255 || i == 255)
    {
        if (MouseX >= r[i].x &&
            MouseX <= (r[i].x + r[i].w) && 
            MouseY >= r[i].y && 
            MouseY <= (r[i].y + r[i].h))
            {
                return r[i].v;
            }
        i++;
    }

    return MAX_U16;
}

s16 Mouse_GetX()
{
    return MouseX;
}

s16 Mouse_GetY()
{
    return MouseY;
}
