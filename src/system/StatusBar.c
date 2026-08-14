#include "StatusBar.h"
#include "Utils.h"
#include "system/Time.h"

// Default 36
#define STATUS_TEXT_LEN 128
#define STATUS_SCROLL_MAX_LEN 36

bool bStatusAtTop = TRUE;
u8 SB_StatusY = 0;
static u8 TitleMaxLen = STATUS_SCROLL_MAX_LEN;
static u8 StatusTextLen = 0;
static char StatusText[STATUS_TEXT_LEN];                // Full status bar text
static char StatusTextScroll[STATUS_SCROLL_MAX_LEN];    // Onscreen text visible in statusbar (scrolled if longer than 36)
static u32 ScrollUpdate;    // Is it time to update the scroll position? Frame counter
static u16 ScrollPause = 8; // Number of frames to wait until next tick - Is set to 64 when reaching either end of text
static u8 ScrollPos = 0;    // Current position in StatusText array
static s8 Direction = 1;    // Direction to scroll (-1 = left - 1 = right)

extern bool bRLNetwork;
extern bool bXPNetwork;
extern bool bMouse;
extern bool bKeyboard;

// Note to self; The rest of TRM / Window functions reside in Utils.c


void SB_SetTitleMaxLen(u8 len)
{
    TitleMaxLen = len;

    if (TitleMaxLen > STATUS_SCROLL_MAX_LEN) TitleMaxLen = STATUS_SCROLL_MAX_LEN;
}

void SB_SetStatusPosition(bool at_top)
{
    bStatusAtTop = at_top;
    SB_StatusY = bStatusAtTop ? 0 : 27;

    TRM_SetWinParam(!bStatusAtTop, FALSE, 0, 1);

    /*
    TODO:
        Account for extra PAL lines.
        Fullscreen windows don't render when at bottom and using an NTSC system.    
    */
}

// Set status test "quickly"
void SB_SetStatusTextQ(const char *t)
{
    TRM_DrawText(t, 1, SB_StatusY, PAL1);
}

void SB_SetStatusText(const char *t)
{
    StatusTextLen = strlen(t);

    if (StatusTextLen >= 128) StatusTextLen = 127;

    ScrollPause = 8;
    ScrollPos = 0;
    Direction = 1;

    memset(StatusText, 0, STATUS_TEXT_LEN);
    memset(StatusTextScroll, 0, STATUS_SCROLL_MAX_LEN);

    strncpy(StatusText, t, STATUS_TEXT_LEN);    // Copy full status text
    strncpy(StatusTextScroll, StatusText, TitleMaxLen);  // Copy only what can fit on screen into the scroll buffer 

    SB_ClearStatusText();
    TRM_DrawText(StatusTextScroll, 1, SB_StatusY, PAL1);
}

void SB_ClearStatusText()
{
    TRM_ClearArea(0, SB_StatusY, TitleMaxLen, 1, PAL1, TRM_CLEAR_WINDOW);
}

void SB_ResetStatusText()
{
    TitleMaxLen = STATUS_SCROLL_MAX_LEN;

    SB_ClearStatusText();

    SB_SetStatusText(STATUS_TEXT_SHORT);
}

void SB_ScrollText()
{
    if (StatusTextLen <= TitleMaxLen) return;

    if (FrameElapsed(&ScrollUpdate, ScrollPause))
    {
        ScrollPause = 8;
        strncpy(StatusTextScroll, StatusText + ScrollPos, TitleMaxLen);
        TRM_DrawText(StatusTextScroll, 1, SB_StatusY, PAL1);

        if (ScrollPos + TitleMaxLen >= StatusTextLen)
        {
            Direction = -1;
            ScrollPause = 64;
        }
        else if (ScrollPos == 0)
        {
            Direction = 1;
            ScrollPause = 64;
        }

        ScrollPos += Direction;
    }
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
    *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR(VDP_WINDOW + ((pos & 63) * 2) + (SB_StatusY * 128));
    *((vu16*) VDP_DATA_PORT) = icon;
}
