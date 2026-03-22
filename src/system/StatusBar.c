#include "StatusBar.h"
#include "Utils.h"

static char StatusText[36];
extern bool bRLNetwork;
extern bool bXPNetwork;
extern bool bMouse;
extern bool bKeyboard;


void SB_SetStatusText(const char *t)
{
    strncpy(StatusText, t, 36);
    TRM_DrawText(StatusText, 1, 0, PAL1);
}

void SB_ResetStatusText()
{
    TRM_ClearArea(0, 0, 36, 1, PAL1, TRM_CLEAR_WINDOW);
    TRM_DrawText(StatusText, 1, 0, PAL1);
}

void SB_ResetStatusBar()
{
    SB_ResetStatusText();

    SB_SetStatusIcon(ICO_NONE, ICO_POS_3);

    if (bKeyboard)
    {
        SB_SetStatusIcon(ICO_KB_OK, ICO_POS_0);
    }
    else if (bMouse)
    {
        SB_SetStatusIcon(ICO_MOUSE_OK, ICO_POS_0);
    }
    else if (JOY_getJoypadType(JOY_1) != JOY_TYPE_UNKNOWN)
    {
        SB_SetStatusIcon(ICO_JP_OK, ICO_POS_0);
    }
    else
    {
        SB_SetStatusIcon(ICO_ID_UNKNOWN, ICO_POS_0);
    }
        
    if (bRLNetwork || bXPNetwork)
    {
        SB_SetStatusIcon(ICO_NET_IDLE_RECV, ICO_POS_1);
        SB_SetStatusIcon(ICO_NET_IDLE_SEND, ICO_POS_2);
    }
    else
    {
        SB_SetStatusIcon(ICO_NET_ERROR, ICO_POS_1);
        SB_SetStatusIcon(ICO_NET_ERROR, ICO_POS_2);
    }
}

void SB_SetStatusIcon(const char icon, u16 pos)
{
    *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR(VDP_WINDOW + ((pos & 63) * 2));
    *((vu16*) VDP_DATA_PORT) = icon;
}