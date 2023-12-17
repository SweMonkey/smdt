
#include "StateCtrl.h"
#include "IRC.h"
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
        KB_Handle_Scancode(kbdata);
    }
    
    if (!bOnce)
    {
        print_charXY_WP(ICO_NET_IDLE_RECV, STATUS_NET_RECV_POS, CHAR_WHITE);
        bOnce = !bOnce;
    }
    #endif
}

void Input_IRC()
{
}

const PRG_State IRCState = 
{
    Enter_IRC, ReEnter_IRC, Exit_IRC, Reset_IRC, Run_IRC, Input_IRC
};

