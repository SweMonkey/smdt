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
#include "system/StatusBar.h"

#ifdef DEBUG_STREAM
#include "kdebug.h"
asm(".global streamdump\nstreamdump:\n.incbin \"tmp/streams/error/PCJR.ANS\"");
extern const unsigned char streamdump[];
static const u32 dumpsize = 2155;
static const u32 dumpstart = 0;
#endif


void Run_DebugStream(u32 len)
{
    #ifdef DEBUG_STREAM
    u32 p = dumpstart;
    u8 kbdata;
    u8 speed = 4;
    bool bStepping = FALSE;
    char title[38];

    if (len == 0) len = dumpsize;

    kprintf("Stream replay start.");
    KDebug_StartTimer();

    while (p < len)
    {
        while (KB_Poll(&kbdata))
        {
            KB_Interpret_Scancode(kbdata);
        }

        if (is_KeyDown(KEY_F1))// || (p == 0x91D))
        {
            bStepping = TRUE;
            InputTick();    // Flush input queue
        }
        if (is_KeyDown(KEY_F2))
        {
            bStepping = FALSE;
            InputTick();    // Flush input queue
        }
        if (is_KeyDown(KEY_F3))
        {
            speed = 96;
        }
        if (is_KeyDown(KEY_F4))
        {
            speed = 4;
        }
        if (is_KeyDown(KEY_F5))
        {
            speed = 0;
        }

        snprintf(title, 37, "$%lX / $%lX - %s", p, len, bStepping ? "Stepping" : "Running ");
        SB_SetStatusText(title);

        while (bStepping)
        {
            while (KB_Poll(&kbdata))
            {
                KB_Interpret_Scancode(kbdata);
            }

            if (WinMgr_isWindowOpen())
            {
                //WinMgr_Input();
            }

            if (is_KeyDown(KEY_F1))
            {
                break;
            }
            else if (is_KeyDown(KEY_RWIN) || is_KeyDown(KEY_F8) || is_KeyUp(sv_MBind_Menu))
            {
                WinMgr_Open(W_QMenu, 0, NULL);  // Global quick menu
            }
            else if (is_AnyKey() && !WinMgr_isWindowOpen())
            {
                bStepping = FALSE;
                break;
            }

            VDP_waitVSync();
            VBlank();
        }

        //kprintf("Pos: $%lX (%ld)", p, p);
        TELNET_ParseRX(streamdump[p]);
        p++;
        //waitMs(speed);
    }

    KDebug_StopTimer();
    snprintf(title, 37, "$%lX / $%lX - Finished", p, len);
    SB_SetStatusText(title);
    kprintf("Stream replay end.");
    #endif
}
