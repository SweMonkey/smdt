
#include "StateCtrl.h"
#include "IRC.h"
#include "Terminal.h"
#include "Buffer.h"
#include "Input.h"
#include "Keyboard_PS2.h"
#include "Utils.h"

#ifndef EMU_BUILD
static u8 rxdata;
static u8 kbdata;
#endif
static u8 bOnce = FALSE;

#ifdef EMU_BUILD
asm(".global ircdump\nircdump:\n.incbin \"tmp/streams/rx_irc_cmd.log\"");
extern const unsigned char ircdump[];
#endif

void Enter_IRC(u8 argc, const char *argv[])
{
    IRC_Init();

    #ifdef EMU_BUILD
    fix32 start = getTimeAsFix32(0);

    // putty_irc.log 405
    // freenode_nodate.log 4479
    // rx_irc.log 423
    // irc_linetest.log 64
    // rx_irc_cmd.log 5682
    u8 data; 
    u32 p = 0;
    u32 s = 5682;
    while (p < s)
    {
        while(Buffer_Push(&RxBuffer, ircdump[p]) != 0xFF)
        {
            p++;
            if (bOnce)
            {
                print_charXY_WP(ICO_NET_RECV, STATUS_NET_RECV_POS, CHAR_GREEN);
                bOnce = !bOnce;
            }

            if (p >= s) break;
        }
        
        while (Buffer_Pop(&RxBuffer, &data) != 0xFF)
        {
            IRC_ParseRX(data);
            
            if (!bOnce)
            {
                print_charXY_WP(ICO_NET_IDLE_RECV, STATUS_NET_RECV_POS, CHAR_WHITE);
                bOnce = !bOnce;
            }
        }
    }

    fix32 end = getTimeAsFix32(0);
    char buf[16];
    fix32ToStr(end-start, buf, 4);
    kprintf("Time elapsed: %s", buf);    
    #endif
}

void ReEnter_IRC()
{
}

void Exit_IRC()
{
}

void Reset_IRC()
{
}

void Run_IRC()
{
    #ifndef EMU_BUILD
    while (Buffer_Pop(&RxBuffer, &rxdata) != 0xFF)
    {
        IRC_ParseRX(rxdata);
        if (PrintDelay) waitMs(PrintDelay);

        if (bOnce)
        {
            print_charXY_WP(ICO_NET_RECV, STATUS_NET_RECV_POS, CHAR_GREEN);
            bOnce = !bOnce;
        }
    }

    while (KB_Poll(&kbdata))
    {
        KB_Interpret_Scancode(kbdata);
    }
    
    if (!bOnce)
    {
        print_charXY_WP(ICO_NET_IDLE_RECV, STATUS_NET_RECV_POS, CHAR_WHITE);
        bOnce = !bOnce;
    }
    #endif
}

void ParseTx()
{
    u8 buf[256];
    u16 i = 0, j = 0;
    u8 data;

    // Pop the TxBuffer back into buf
    while ((Buffer_Pop(&TxBuffer, &data) != 0xFF) && (i++ < 255))
    {
        buf[i] = data;
    }

    // parse data

    // Push buf back into TxBuffer
    j = 0;
    while (j++ < (i-1))
    {
        Buffer_Push(&TxBuffer, buf[j]);
    }
}

void Input_IRC()
{
    if (!bWindowActive)
    {
        if (is_KeyDown(KEY_UP))
        {
        }

        if (is_KeyDown(KEY_DOWN))
        {
        }

        if (is_KeyDown(KEY_LEFT))
        {
            if (!FontSize)
            {
                HScroll += 8;
                VDP_setHorizontalScrollVSync(BG_A, HScroll % 1024);
                VDP_setHorizontalScrollVSync(BG_B, HScroll % 1024);
            }
            else
            {
                HScroll += 4;
                VDP_setHorizontalScroll(BG_A, (HScroll-4) % 1024);
                VDP_setHorizontalScroll(BG_B, (HScroll-8) % 1024);
            }

            TTY_MoveCursor(TTY_CURSOR_DUMMY);
        }

        if (is_KeyDown(KEY_RIGHT))
        {
            if (!FontSize)
            {
                HScroll -= 8;
                VDP_setHorizontalScrollVSync(BG_A, HScroll % 1024);
                VDP_setHorizontalScrollVSync(BG_B, HScroll % 1024);
            }
            else
            {
                HScroll -= 4;
                VDP_setHorizontalScroll(BG_A, (HScroll-4) % 1024);
                VDP_setHorizontalScroll(BG_B, (HScroll-8) % 1024);
            }

            TTY_MoveCursor(TTY_CURSOR_DUMMY);
        }

        if (is_KeyDown(KEY_RETURN))
        {
            //ParseTx();  // Mess with the TxBuffer a bit...
            TTY_TransmitBuffer();
            //TTY_SendChar(0xD, 0); // Send \r - carridge return
            //TTY_SendChar(0xA, 0); // Send \n - line feed

            //line feed, new line
            TTY_MoveCursor(TTY_CURSOR_DOWN, 1);
            TTY_ClearLine(sy % 32, 4);

            //carriage return
            sx = 0;
            TTY_MoveCursor(TTY_CURSOR_DUMMY);   // Dummy
        }

        if (is_KeyDown(KEY_BACKSPACE))
        {
            TTY_MoveCursor(TTY_CURSOR_LEFT, 1);
            VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(1, 0, 0, 0, 0), sx, sy);
            VDP_setTileMapXY(BG_B, TILE_ATTR_FULL(2, 0, 0, 0, 0), sx, sy);

            Buffer_ReversePop(&TxBuffer);
        }
    }
}

const PRG_State IRCState = 
{
    Enter_IRC, ReEnter_IRC, Exit_IRC, Reset_IRC, Run_IRC, Input_IRC
};

