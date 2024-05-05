
#include "XP_Network.h"
#include "Buffer.h"
#include "Utils.h"      // TRM
#include "Network.h"    // RxBuffer/TxBuffer

// Time out and wait values are prelimiary and needs more calibration.

u32 vConn_time = 500000; // Connection timeout (waiting for connection to remote server)


u8 XPN_Initialize()
{
    // Todo: Call XPN_Reset() and listen for 'D' instead?

    if (XPN_EnterMonitorMode())
    {
        waitMs(200);

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
    NET_SendChar(data, TXF_NOBUFFER);
}

u8 XPN_ReadByte()
{
    u8 rxdata = 0;

    if (Buffer_Pop(&RxBuffer, &rxdata) == 0) return rxdata;

    return 0;
}

void XPN_SendMessage(char *str) 
{
    NET_SendString(str);
}

void XPN_FlushBuffers()
{
    Buffer_Flush(&TxBuffer);
    Buffer_Flush(&RxBuffer);
    return;
}

// Enter Monitor Mode
bool XPN_EnterMonitorMode()
{
    u32 timeout = 0;

    XPN_FlushBuffers();    
    XPN_SendMessage("C0.0.0.0/0\n");

    waitMs(200);

    while (XPN_ReadByte() != '>')
    {
        if (timeout++ >= 50000) return FALSE;
    }

    return TRUE;
}

// Exit Monitor Mode
bool XPN_ExitMonitorMode()
{
    u32 timeout = 0;

    XPN_FlushBuffers();    
    XPN_SendMessage("QU\n");

    waitMs(200);

    while (XPN_ReadByte() != '>')
    {
        if (timeout++ >= 50000) return FALSE;
    }

    return TRUE;
}

bool XPN_Connect(char *str)
{
    char tmp[64];
    u32 timeout = 0;
    u8 byte = 0;

    snprintf(tmp, 36, "Connecting to %s", str);
    TRM_SetStatusText(tmp);

    XPN_FlushBuffers();  

    XPN_SendByte('C');
    XPN_SendMessage(str);
    XPN_SendByte(0x0A);

    waitMs(500);

    while (timeout < vConn_time)
    {
        if (Buffer_Pop(&RxBuffer, &byte) == 0)
        {
            switch(byte)
            {
                case 'C': // Connected
                {
                    TRM_SetStatusText(STATUS_TEXT);
                    XPN_FlushBuffers();
                    return TRUE;
                }
                case 'N': // Host Unreachable
                {
                    TRM_SetStatusText(STATUS_TEXT);
                    XPN_FlushBuffers();
                    return FALSE;
                }
            }
        }

        timeout++;
    }
    
    XPN_FlushBuffers();
    TRM_SetStatusText(STATUS_TEXT);
    return FALSE;
}

void XPN_Disconnect()
{
    u32 timeout = 0;

    // 1. Real xPico
    // Set CP3 pin to tell the xPico to disconnect from the remote server
    OrDevData(DEV_UART, 0x20);  // Set pin 7 high
    waitMs(200);
    UnsetDevData(DEV_UART);     // Set pin 7 low
    waitMs(200);

    while (XPN_ReadByte() != 'D')
    {
        if (timeout++ >= 50000) break;
    }

    // 2. Emulated xPico
    // Enter/Exit monitor to tell the xPico emulator to disconnect/close the socket
    XPN_EnterMonitorMode();
    waitMs(200);
    XPN_ExitMonitorMode();
}

void XPN_PrintIP(int x, int y)
{
    /*
    XPN_EnterMonitorMode();
    XPN_FlushBuffers();
    XPN_SendMessage("NC\n"); // Send command to get network information

    waitMs(200);
    
    while (1)
    {
        //while (!RLN_RXReady());
        u8 byte = RLN_ReadByte();
        if (byte == 'G') { break; }
        if ((byte >= '0' && byte <= '9') || byte == '.' || byte == '1')
        {
            TRM_DrawChar(byte, x, y, PAL1); x++;
        }
    }

    XPN_ExitMonitorMode();
    */
}