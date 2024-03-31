
#include "StateCtrl.h"
#include "Telnet.h"
#include "Terminal.h"
#include "Buffer.h"
#include "Input.h"
#include "Keyboard_PS2.h"
#include "Utils.h"
#include "Network.h"

#ifndef EMU_BUILD
static u8 rxdata;
static u8 kbdata;
#endif

#ifdef EMU_BUILD
asm(".global logdump\nlogdump:\n.incbin \"tmp/streams/nano.log\"");
extern const unsigned char logdump[];
#endif

static u8 bOnce = FALSE;


void Enter_Terminal(u8 argc, const char *argv[])
{
    TELNET_Init();
    TRM_SetStatusText(STATUS_TEXT);

    // Variable overrides
    vDoEcho = 1;
    vLineMode = 0;
    vNewlineConv = 0;
    bWrapAround = TRUE;


    #ifdef EMU_BUILD
    // nano.log 2835
    u8 data; 
    u32 p = 0;
    u32 s = 2835;
    while (p < s)
    {
        while(Buffer_Push(&RxBuffer, logdump[p]) != 0xFF)
        {
            p++;
            if (bOnce)
            {
                TRM_SetStatusIcon(ICO_NET_RECV, STATUS_NET_RECV_POS, CHAR_GREEN);
                bOnce = !bOnce;
            }

            if (p >= s) break;
        }
        
        while (Buffer_Pop(&RxBuffer, &data) != 0xFF)
        {
            TELNET_ParseRX(data);
            waitMs(10);
            
            if (!bOnce)
            {
                TRM_SetStatusIcon(ICO_NET_IDLE_RECV, STATUS_NET_RECV_POS, CHAR_WHITE);
                bOnce = !bOnce;
            }
        }
    }
    #endif
}

void ReEnter_Terminal()
{
}

void Exit_Terminal()
{
}

void Reset_Terminal()
{
    TTY_Reset(TRUE);
}

void Run_Terminal()
{
    #ifndef EMU_BUILD
    while (Buffer_Pop(&RxBuffer, &rxdata) != 0xFF)
    {
        TELNET_ParseRX(rxdata);

        if (bOnce)
        {
            TRM_SetStatusIcon(ICO_NET_RECV, STATUS_NET_RECV_POS, CHAR_GREEN);
            bOnce = !bOnce;
        }
    }

    while (KB_Poll(&kbdata))
    {
        KB_Interpret_Scancode(kbdata);
    }

    if (!bOnce)
    {
        TRM_SetStatusIcon(ICO_NET_IDLE_RECV, STATUS_NET_RECV_POS, CHAR_WHITE);
        bOnce = !bOnce;
    }
    #endif
}

void Input_Terminal()
{
    if (!bWindowActive)
    {
        if (is_KeyDown(KEY_UP))
        {
            NET_SendChar(0x1B, TXF_NOBUFFER);    // ESC
            NET_SendChar(0x5B, TXF_NOBUFFER);    // [
            NET_SendChar(0x41, TXF_NOBUFFER);    // A
            TTY_MoveCursor(TTY_CURSOR_DUMMY);
        }

        if (is_KeyDown(KEY_DOWN))
        {
            NET_SendChar(0x1B, TXF_NOBUFFER);    // ESC
            NET_SendChar(0x5B, TXF_NOBUFFER);    // [
            NET_SendChar(0x42, TXF_NOBUFFER);    // B
            TTY_MoveCursor(TTY_CURSOR_DUMMY);
        }

        if (is_KeyDown(KEY_LEFT))
        {
            NET_SendChar(0x1B, TXF_NOBUFFER);    // ESC
            NET_SendChar(0x5B, TXF_NOBUFFER);    // [
            NET_SendChar(0x44, TXF_NOBUFFER);    // D
            TTY_MoveCursor(TTY_CURSOR_DUMMY);
        }

        if (is_KeyDown(KEY_RIGHT))
        {
            NET_SendChar(0x1B, TXF_NOBUFFER);    // ESC
            NET_SendChar(0x5B, TXF_NOBUFFER);    // [
            NET_SendChar(0x43, TXF_NOBUFFER);    // C
            TTY_MoveCursor(TTY_CURSOR_DUMMY);
        }

        if (is_KeyDown(KEY_KP4_LEFT))
        {
            if (!FontSize)
            {
                HScroll += 8;
                VDP_setHorizontalScroll(BG_A, HScroll);
                VDP_setHorizontalScroll(BG_B, HScroll);
            }
            else
            {
                HScroll += 4;
                VDP_setHorizontalScroll(BG_A, (HScroll+4));  // -4
                VDP_setHorizontalScroll(BG_B, (HScroll  ));  // -8
            }

            TTY_MoveCursor(TTY_CURSOR_DUMMY);
        }

        if (is_KeyDown(KEY_KP6_RIGHT))
        {
            if (!FontSize)
            {
                HScroll -= 8;
                VDP_setHorizontalScroll(BG_A, HScroll);
                VDP_setHorizontalScroll(BG_B, HScroll);
            }
            else
            {
                HScroll -= 4;
                VDP_setHorizontalScroll(BG_A, (HScroll+4));  // -4
                VDP_setHorizontalScroll(BG_B, (HScroll  ));  // -8
            }
            
            TTY_MoveCursor(TTY_CURSOR_DUMMY);
        }

        if (is_KeyDown(KEY_DELETE))
        {
            NET_SendChar(0x1B, TXF_NOBUFFER);    // ESC
            NET_SendChar(0x5B, TXF_NOBUFFER);    // [
            NET_SendChar(0x7F, TXF_NOBUFFER);    // DEL
        }

        if (is_KeyDown(KEY_F11))
        {
            NET_SendChar(0x03, TXF_NOBUFFER);    // ^C
        }

        if (is_KeyDown(KEY_F12))
        {
            NET_SendChar(0x18, TXF_NOBUFFER);    // ^X
        }

        if (is_KeyDown(KEY_RETURN))
        {
            NET_SendChar(0xD, TXF_NOBUFFER); // Send \r - carridge return
            //NET_SendChar(0xA, TXF_NOBUFFER); // Send \n - line feed

            // Carriage return
            TTY_SetSX(0);
            TTY_MoveCursor(TTY_CURSOR_DUMMY);
        }

        if (is_KeyDown(KEY_BACKSPACE))
        {
            if (!FontSize)
            {
                VDP_setTileMapXY(BG_B, TILE_ATTR_FULL(2, 0, 0, 0, 0), TTY_GetSX(), sy);
            }
            else
            {
                VDP_setTileMapXY(!(TTY_GetSX() % 2), TILE_ATTR_FULL(2, 0, 0, 0, 0), TTY_GetSX()>>1, sy);
            }

            // 0x8  = backspace
            // 0x7F = DEL
            NET_SendChar(0x7F, TXF_NOBUFFER);   // DEL            
        }

        if (is_KeyDown(KEY_ESCAPE))
        {
            NET_SendChar(0x1B, TXF_NOBUFFER); // Send \ESC
        }
    }
}

const PRG_State TerminalState = 
{
    Enter_Terminal, ReEnter_Terminal, Exit_Terminal, Reset_Terminal, Run_Terminal, Input_Terminal, NULL, NULL
};

