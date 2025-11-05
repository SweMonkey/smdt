#ifndef MEGAMOUSE_H_INCLUDED
#define MEGAMOUSE_H_INCLUDED

#include "DevMgr.h"

extern SM_Device DRV_MMOUSE;

bool MM_Mouse_Init();
bool MM_Mouse_Poll(s16 *delta_x, s16 *delta_y, u16 *buttons);

#endif // MEGAMOUSE_H_INCLUDED
