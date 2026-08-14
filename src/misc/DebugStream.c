#include "DebugStream.h"
#include "Utils.h"
#include "Network.h"
#include "Telnet.h"
#include "Input.h"
#include "Keyboard.h"
#include "StateCtrl.h"
#include "Terminal.h"
#include "WinMgr.h"
#include "Mouse.h"
#include "system/Time.h"
#include "system/StatusBar.h"

#ifdef DEBUG_STREAM
#include "kdebug.h"
asm(".global streamdump\nstreamdump:\n.incbin \"tmp/streams/rx_df_title.log\"");
extern const unsigned char streamdump[];
static const u32 dumpsize = 63819;
static const u32 dumpstart = 0;
extern u8 bDoCursorBlink;
extern u16 LastCursor;
#endif


void Run_DebugStream(u32 len)
{
    #ifdef DEBUG_STREAM
    u32 p = dumpstart;
    u8 kbdata;
    u16 speed = 4;
    u32 ticks = 0;
    bool bStepping = FALSE;
    char title[38];

    RXBytes = 0;

    if (len == 0) len = dumpsize;

    kprintf("Stream replay start.");
    KDebug_StartTimer();

    SB_ResetStatusText();
    SB_SetTitleMaxLen(36);
    
    while (p < len)
    {
        while (KB_Poll(&kbdata))
        {
            KB_Interpret_Scancode(kbdata);
        }

        if (bMouse) Mouse_Poll();

        if (is_KeyDown(KEY_F1))// || (p == 0x1624))
        {
            bStepping = TRUE;
        }
        else if (is_KeyDown(KEY_F3))
        {
            speed = 8;
        }
        else if (is_KeyDown(KEY_F4))
        {
            speed = 4;
        }
        else if (is_KeyDown(KEY_F5))
        {
            speed = 0;
        }
        else if (is_KeyDown(KEY_F6))
        {
            bDoCursorBlink = TRUE;
            
            if (TTY_GetFont()) LastCursor = 0x13;
            else               LastCursor = 0x10;
        }

        snprintf(title, 37, "$%lX / $%lX - %s", p, len, bStepping ? "Stepping" : "Running ");
        SB_SetStatusTextQ(title);

        while (bStepping)
        {
            while (KB_Poll(&kbdata))
            {
                KB_Interpret_Scancode(kbdata);
            }

            if (bMouse) Mouse_Poll();

            if (is_KeyDown(KEY_F2))
            {
                bStepping = FALSE;
                break;
            }
            else if (is_KeyDown(KEY_F1))
            {
                break;
            }
            else if (is_KeyDown(KEY_RWIN) || is_KeyDown(KEY_F8) || is_KeyUp(sv_MBind_Menu))
            {
                WinMgr_Open(W_QMenu, 0, NULL);  // Global quick menu
            }

            VDP_waitVSync();
            VBlank();
        }

        if (FrameElapsed(&ticks, speed))
        {
            TELNET_ParseRX(streamdump[p]);
            p++;
        }

        VDP_waitVSync();
        VBlank();
    }

    KDebug_StopTimer();
    snprintf(title, 37, "$%lX / $%lX - Finished", p, len);
    SB_SetStatusTextQ(title);
    kprintf("Stream replay end.");
    #endif
}
