
#include "StateCtrl.h"
#include "Telnet.h"
#include "Terminal.h"
#include "IRQ.h"
#include "Keyboard_PS2.h"
#include "main.h"

#ifndef EMU_BUILD
static u8 rxdata;
static u8 kbdata;
#endif
static u8 bOnce = FALSE;

#ifdef EMU_BUILD
asm(".global telnetdump\ntelnetdump:\n.incbin \"tmp/streams/rx_sw.log\"");
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
    u8 data; 
    u32 p = 0;
    u32 s = 3122726;
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

        
        //if (p >= s) break;
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
        KB_Handle_Scancode(kbdata);
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
}


const PRG_State TelnetState = 
{
    Enter_Telnet, ReEnter_Telnet, Exit_Telnet, Reset_Telnet, Run_Telnet, Input_Telnet
};

