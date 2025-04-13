#include "DebugStream.h"
#include "Utils.h"
#include "Network.h"
#include "Telnet.h"

#ifdef DEBUG_STREAM
#include "kdebug.h"
asm(".global streamdump\nstreamdump:\n.incbin \"tmp/streams/rx_nethack_lines2.log\"");
extern const unsigned char streamdump[];
static u32 dumpsize = 365971;
#endif


void Run_DebugStream(u32 len)
{
    #ifdef DEBUG_STREAM
    u32 p = 0;

    if (len > 0) dumpsize = len;

    kprintf("Stream replay start.");
    KDebug_StartTimer();

    /*u8 data;
    
    while (p < dumpsize)
    {
        while (Buffer_Push(&RxBuffer, streamdump[p]))
        {
            p++;

            if (p >= dumpsize) break;
        }
        
        while (Buffer_Pop(&RxBuffer, &data))
        {
            TELNET_ParseRX(data);
            //waitMs(4);
        }
    }*/

    while (p < dumpsize)
    {
        TELNET_ParseRX(streamdump[p]);
        p++;
        //waitMs(4);
    }

    KDebug_StopTimer();
    kprintf("Stream replay end.");
    #endif
}
