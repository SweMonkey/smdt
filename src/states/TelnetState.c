
#include "StateCtrl.h"
#include "Telnet.h"
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
asm(".global telnetdump\ntelnetdump:\n.incbin \"tmp/streams/telnetdump.log\"");
extern const unsigned char telnetdump[];
#endif

void Enter_Telnet(u8 argc, const char *argv[])
{
    TELNET_Init();

    #ifdef EMU_BUILD
    fix32 start = getTimeAsFix32(0);

    // out.log 7357
    // out3.log 9139
    // putty1.log 85527
    // putty3.log 24919
    // telnetdump.log 140676
    // enigma_hw_stream_utf8_wireshark.log 7405
    // enigma_hw_stream_utf8_wireshark_no_iac.log 7235
    // testnano_eng.log 701
    // testnano_eng_s.log 514
    // putty5.log 4056
    // matrix.log 9139
    // putty_irc.log 405
    // ncurses.log 6803
    // IAC.log 74
    // rx.log 32440
    // rx_enigma.log 8029
    // rx_sw.log 3122726
    // rx_arcadia.log 11156
    u8 data; 
    u32 p = 0;
    u32 s = 140676;
    while (p < s)
    {
        while(Buffer_Push(&RxBuffer, telnetdump[p]) != 0xFF)
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
            TELNET_ParseRX(data);
            
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

void ReEnter_Telnet()
{
}

void Exit_Telnet()
{
}

void Reset_Telnet()
{
    TTY_Reset(TRUE);
}

void Run_Telnet()
{
    #ifndef EMU_BUILD
    while (Buffer_Pop(&RxBuffer, &rxdata) != 0xFF)
    {
        TELNET_ParseRX(rxdata);
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

void Input_Telnet()
{
    if (!bWindowActive)
    {
        if (is_KeyDown(KEY_UP))
        {
            TTY_SendChar(0x1B, TXF_NOBUFFER);    // ESC
            TTY_SendChar(0x5B, TXF_NOBUFFER);    // [
            TTY_SendChar(0x41, TXF_NOBUFFER);    // A
        }

        if (is_KeyDown(KEY_DOWN))
        {
            TTY_SendChar(0x1B, TXF_NOBUFFER);    // ESC
            TTY_SendChar(0x5B, TXF_NOBUFFER);    // [
            TTY_SendChar(0x42, TXF_NOBUFFER);    // B
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

            TTY_SendChar(0x1B, TXF_NOBUFFER);    // ESC
            TTY_SendChar(0x5B, TXF_NOBUFFER);    // [
            TTY_SendChar(0x44, TXF_NOBUFFER);    // D
            
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

            TTY_SendChar(0x1B, TXF_NOBUFFER);    // ESC
            TTY_SendChar(0x5B, TXF_NOBUFFER);    // [
            TTY_SendChar(0x43, TXF_NOBUFFER);    // C
            
            TTY_MoveCursor(TTY_CURSOR_DUMMY);
        }

        if (is_KeyDown(KEY_DELETE))
        {
            TTY_SendChar(0x1B, TXF_NOBUFFER);    // ESC
            TTY_SendChar(0x5B, TXF_NOBUFFER);    // [
            TTY_SendChar(0x7F, TXF_NOBUFFER);    // DEL
        }

        if (is_KeyDown(KEY_RETURN))
        {
            /*When LINEMODE is turned on, and when in EDIT mode, when any normal
            line terminator on the client side operating system is typed, the
            line should be transmitted with "CR LF" as the line terminator.  When
            EDIT mode is turned off, a carriage return should be sent as "CR
            NUL", a line feed should be sent as LF, and any other key that cannot
            be mapped into an ASCII character, but means the line is complete
            (like a DOIT or ENTER key), should be sent as "CR LF".*/
            if (vLineMode & LMSM_EDIT)
            {
                TTY_TransmitBuffer();
                TTY_SendChar(0xD, 0); // Send \r - carridge return
                TTY_SendChar(0xA, 0); // Send \n - line feed
            }
            else
            {
                TTY_SendChar(0xD, TXF_NOBUFFER); // Send \r - carridge return
                //TTY_SendChar(0, TXF_NOBUFFER); // Send NUL
                TTY_SendChar(0xA, TXF_NOBUFFER); // Send \n - line feeds
            }

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

            // 0x8  = backspace
            // 0x7F = delete
            if (vLineMode & LMSM_EDIT)
            {
                Buffer_ReversePop(&TxBuffer);
            }
            else
            {
                TTY_SendChar(0x8, TXF_NOBUFFER); // send backspace
            }
        }

        if (is_KeyDown(KEY_ESCAPE))
        {
            TTY_SendChar(0x1B, 0); // Send \ESC
        }
    }
}


const PRG_State TelnetState = 
{
    Enter_Telnet, ReEnter_Telnet, Exit_Telnet, Reset_Telnet, Run_Telnet, Input_Telnet
};

