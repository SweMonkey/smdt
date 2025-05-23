#include "XP_Network.h"
#include "Buffer.h"
#include "Utils.h"      // TRM
#include "Network.h"    // RxBuffer/TxBuffer
#include "system/Stdout.h"

#define CTIME 500000
#define RTIME 300000
#define DTIME 500

u32 sv_ConnTimeout = CTIME;    // Connection timeout (waiting for connection to remote server)
u32 sv_ReadTimeout = RTIME;    // Readback timeout (waiting for response from xport)
u32 sv_DelayTime   = DTIME;    // Milliseconds of delay between sending a command and reading the response
static u8 bIsInMonitorMode = FALSE;


u8 XPN_Initialize()
{
    #ifdef EMU_BUILD
    return 0;
    #endif

    // Force reset these variables in case they have become set to 0
    if (sv_ConnTimeout == 0) sv_ConnTimeout = CTIME;
    if (sv_ReadTimeout == 0) sv_ReadTimeout = RTIME;
    if (sv_DelayTime == 0) sv_DelayTime = DTIME;

    //bIsInMonitorMode = FALSE;
    if (bHardReset) waitMs(1000);

    if (XPN_EnterMonitorMode())
    {
        waitMs(sv_DelayTime);

        if (XPN_ExitMonitorMode() == FALSE)
        {
            return 2;
        }

        return 1;
    }

    return 0;
}

void XPN_SendByte(u8 data) 
{
    NET_SendChar(data);
}

bool XPN_ReadByte(u8 *data)
{
    return Buffer_Pop(&RxBuffer, data);
}

void XPN_SendMessage(char *str) 
{
    NET_SendString(str);
}

void XPN_FlushBuffers()
{
    Buffer_Flush0(&TxBuffer);
    Buffer_Flush0(&RxBuffer);
    return;
}

// Enter Monitor Mode
bool XPN_EnterMonitorMode()
{
    u32 timeout = 0;
    u8 data;

    //if (bIsInMonitorMode) return TRUE;

    XPN_FlushBuffers();    
    XPN_SendMessage("C0.0.0.0/0\n");

    waitMs(sv_DelayTime);
    
    while (1)
    {
        if (XPN_ReadByte(&data))
        {
            if (data == '>') break;
        }

        if (timeout++ >= sv_ReadTimeout)
        {
            return FALSE;
        }
    }

    bIsInMonitorMode = TRUE;

    return TRUE;
}

// Exit Monitor Mode
bool XPN_ExitMonitorMode()
{
    u32 timeout = 0;
    u8 data;

    //if (!bIsInMonitorMode) return TRUE;

    XPN_FlushBuffers();
    XPN_SendMessage("QU\n");

    waitMs(sv_DelayTime);
    
    while (1)
    {
        if (XPN_ReadByte(&data))
        {
            if (data == '>') break;
        }

        // Check for a specific sequence in RxBuffer
        if ((RxBuffer.data[0] == 'Q') &&
            (RxBuffer.data[1] == 'U') &&
            (RxBuffer.data[2] == 0xD) &&
            (RxBuffer.data[3] == 0xA))
        {
            break;  // Exit loop if the specific sequence is detected
        }

        if (timeout++ >= sv_ReadTimeout)
        {
            XPN_FlushBuffers();

            return FALSE;
        }
    }

    XPN_FlushBuffers();

    bIsInMonitorMode = FALSE;

    return TRUE;
}

bool XPN_Connect(char *str)
{
    u32 timeout = 0;
    u8 byte = 0;

    XPN_FlushBuffers();  

    XPN_SendByte('C');
    XPN_SendMessage(str);
    XPN_SendByte(0x0A);

    waitMs(sv_DelayTime);

    while ((RxBuffer.data[0] != 'C') && (RxBuffer.data[0] != 'N'))
    {
        if (timeout++ >= sv_ReadTimeout) return FALSE;
    }

    // Discard the 'C' or 'N' byte from buffer
    Buffer_Pop(&RxBuffer, &byte);
    return TRUE;
}

void XPN_Disconnect()
{
    u32 timeout = 0;
    u8 byte = 0;

    XPN_FlushBuffers();

    // Set CP3 pin to tell the xPort to disconnect from the remote server
    DEV_SetData(DRV_UART, 0x40);// Set pin 7 high
    waitMs(500);
    DEV_ClrData(DRV_UART);      // Set pin 7 low

    waitMs(sv_DelayTime);

    while (RxBuffer.data[0] != 'D')
    {
        if (timeout++ >= sv_ReadTimeout) goto Exit;
    }

    // Discard the 'D' byte from buffer
    if (RxBuffer.data[0] == 'D') Buffer_Pop(&RxBuffer, &byte);

    Exit:

    // This is not needed on a real xport. Its only here to tell a fake xport emulator to disconnect, since it can't read the CP3 pin for obvious reasons
    XPN_EnterMonitorMode();
    XPN_ExitMonitorMode();
    // -----------------------------

    return;
}

u8 XPN_GetIP(char *ret)
{
    u32 timeout = 0;
    u8 byte = 0;
    u8 i = 0;
    u8 r = 0;

    if (ret == NULL) return 1;

    XPN_EnterMonitorMode();

    XPN_FlushBuffers();
    XPN_SendMessage("NC\n"); // Send command to get network information

    waitMs(sv_DelayTime);

    while (1)
    {
        if (XPN_ReadByte(&byte))
        {
            // Exit the loop if byte is 'G'
            if (byte == 'G') break;

            // Store the byte if its valid
            if ((byte >= '0' && byte <= '9') || byte == '.' || byte == '1')
            {
                ret[i++] = byte;
            }

            timeout = 0;
        }

        if (timeout++ >= sv_ReadTimeout)
        {
            r = 2;
            break;
        }
    }

    XPN_ExitMonitorMode();

    ret[i++] = '\0';
    return r;
}

// Ping IP Address
// Only accepts an IP address to ping
void XPN_PingIP(char *ip)
{
    u32 timeout = 0;
    u16 ping_counter = 0;
    u16 byte_count = 0;
    u8 byte = 0;

    XPN_EnterMonitorMode();

    XPN_SendMessage("PI ");
    XPN_SendMessage(ip);
    XPN_SendMessage("\n");

    while (ping_counter < 5)
    {
        if (XPN_ReadByte(&byte))
        {
            if (byte == 0) continue;

            byte_count++;

            // Skip the first 2 bytes
            if (byte_count > 2)
            {
                Stdout_PushByte(byte);

                if (byte == '\n')
                {
                    ping_counter++;
                }

                // Flush stdout after pushing byte
                Stdout_Flush();
            }
        }

        if (timeout++ >= sv_ReadTimeout)
        {
            Stdout_Push("Ping response timeout!\n");
            break;
        }
    }

    XPN_FlushBuffers();
    XPN_SendMessage("QU\n");    // Pre-emptive manual exit monitor mode to halt pinging
    waitMs(sv_DelayTime);
    XPN_ExitMonitorMode();

    return;
}
