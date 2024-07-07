#include "XP_Network.h"
#include "Buffer.h"
#include "Utils.h"      // TRM
#include "Network.h"    // RxBuffer/TxBuffer
#include "system/Stdout.h"

#define CTIME 500000
#define RTIME 300000
#define DTIME 200

u32 sv_ConnTimeout = CTIME;    // Connection timeout (waiting for connection to remote server)
u32 sv_ReadTimeout = RTIME;    // Readback timeout (waiting for response from xpico)
u32 sv_DelayTime   = DTIME;    // Milliseconds of delay between sending a command and reading the response
static u8 bIsInMonitorMode = FALSE;
static u8 rxdata = 0;


u8 XPN_Initialize()
{
    #ifdef EMU_BUILD
    return 0;
    #endif

    // Todo: Call XPN_Reset() and listen for 'D' instead?

    // Reset these variables in case of a clean SRAM
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

bool XPN_RXReady() 
{
    return !Buffer_IsEmpty(&RxBuffer);
}

void XPN_SendByte(u8 data) 
{
    NET_SendChar(data, TXF_NOBUFFER);
}

u8 XPN_ReadByte()
{
    if (Buffer_Pop(&RxBuffer, &rxdata) == 0) return rxdata;

    return 0;
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

    if (bIsInMonitorMode) return TRUE;

    XPN_FlushBuffers();    
    XPN_SendMessage("C0.0.0.0/0\n");

    waitMs(sv_DelayTime);
    
    while (!XPN_RXReady() || XPN_ReadByte() != '>')
    {
        if (timeout++ >= sv_ReadTimeout) return FALSE;
    }

    bIsInMonitorMode = TRUE;

    return TRUE;
}

// Exit Monitor Mode
bool XPN_ExitMonitorMode()
{
    u32 timeout = 0;

    if (!bIsInMonitorMode) return TRUE;

    XPN_FlushBuffers();
    XPN_SendMessage("QU\n");

    waitMs(sv_DelayTime);
    
    while (!XPN_RXReady() || XPN_ReadByte() != '>')
    {
        if ((RxBuffer.data[0] == 'Q') && (RxBuffer.data[1] == 'U') && (RxBuffer.data[2] == 0xD) && (RxBuffer.data[3] == 0xA)) break;

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
    bool r = FALSE;

    XPN_FlushBuffers();  

    XPN_SendByte('C');
    XPN_SendMessage(str);
    XPN_SendByte(0x0A);

    while ((RxBuffer.data[0] != 'C') || (RxBuffer.data[0] != 'N'))
    {
        if (timeout++ >= sv_ReadTimeout) goto Exit;
    }

    if (RxBuffer.data[0] == 'C')
    {
        r = TRUE;
        Buffer_Pop(&RxBuffer, &byte);   // Discard the 'C' byte from buffer
    }
    else if (RxBuffer.data[0] == 'N')
    {
        Buffer_Pop(&RxBuffer, &byte);   // Discard the 'N' byte from buffer
    }
    
    Exit:
    return r;
}

void XPN_Disconnect()
{
    u32 timeout = 0;
    u8 byte = 0;

    XPN_FlushBuffers();

    // 1. Real xPico
    // Set CP3 pin to tell the xPico to disconnect from the remote server
    OrDevData(DEV_UART, 0x20);  // Set pin 7 high
    waitMs(400);
    UnsetDevData(DEV_UART);     // Set pin 7 low
    waitMs(sv_DelayTime);
    waitMs(sv_DelayTime);

    while (RxBuffer.data[0] != 'D')
    {
        if (timeout++ >= sv_ReadTimeout) return;
    }

    if (RxBuffer.data[0] == 'D')
    {
        Buffer_Pop(&RxBuffer, &byte);   // Discard the 'D' byte from buffer
    }
    
    /*// 2. Emulated xPico
    // Enter/Exit monitor to tell the xPico emulator to disconnect/close the socket
    XPN_EnterMonitorMode();
    waitMs(sv_DelayTime);
    XPN_ExitMonitorMode();*/
}

u8 XPN_GetIP(char *ret)
{
    u32 timeout = 0;
    u8 byte = 0;
    u8 i = 0;
    u8 r = 0;

    if (ret == NULL) return 1;

    if (XPN_EnterMonitorMode() == FALSE) return 1;

    XPN_FlushBuffers();
    XPN_SendMessage("NC\n"); // Send command to get network information

    waitMs(sv_DelayTime);

    while (byte != 'G')
    {
        if (XPN_RXReady())
        {
            byte = XPN_ReadByte();

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
    int ping_counter = 0;
    int byte_count = 0;
    int ping_count = 4;

    XPN_EnterMonitorMode();
    XPN_SendMessage("PI ");
    XPN_SendMessage(ip);
    XPN_SendMessage("\n");

    while (1)
    {
        while (!XPN_RXReady())
        {
            if (timeout++ >= sv_ReadTimeout) { break; }
        }

        u8 byte = XPN_ReadByte();
        byte_count++;

        if (byte_count > 2)
        {
            Stdout_PushByte(byte);

            if (byte == '\n'){ Stdout_Push("\n"); Stdout_Flush(); ping_counter++; }
            if (ping_counter >= ping_count+1){ break; }
        }

        timeout = 0;
    }

    XPN_ExitMonitorMode();
}