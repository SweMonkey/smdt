#ifndef MOUSE_H_INCLUDED
#define MOUSE_H_INCLUDED

#include <genesis.h>

typedef struct 
{
    u16 x, y, w, h;
    u16 v;
} MRect;

extern u8 sv_PointerStyle;

// TODO: 1. Save these variables - 2. Make them user configurable
extern u16 sv_MBind_Click;
extern u16 sv_MBind_AltClick;
extern u16 sv_MBind_Menu;
extern u16 sv_MBind_Function;

extern bool bMouse;

void Mouse_Init();
void Mouse_Poll();

u16 Mouse_GetRect(const MRect *r);
bool Mouse_GetRect_TEST(u16 x, u16 y, u16 w, u16 h);
s16 Mouse_GetX();
s16 Mouse_GetY();

#endif // MOUSE_H_INCLUDED
