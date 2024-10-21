/*
MIT License

Copyright (c) 2023 B1tsh1ft3r

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Modified for use in SMDTC by smds.
Originial source at: https://github.com/b1tsh1ft3r/retro.link/tree/main/sega_genesis/sgdk_example
*/

#include "RL_Network.h"
#include "Buffer.h"
#include "Utils.h"      // TRM
#include "Network.h"    // RxBuffer/TxBuffer
#include "XP_Network.h"
#include "system/Stdout.h"


#define RL_DLM 0x00
#define RL_DLL 0x01

u8 sv_DLM = RL_DLM;
u8 sv_DLL = RL_DLL;
static u8 bIsInMonitorMode = FALSE;


// Initialize Network Adapter
// Detects adapter and returns TRUE or FALSE boolean value
bool RLN_Initialize()
{
    // Reset these variables in case of a clean SRAM
    if (sv_DLM == 0) sv_DLM = RL_DLM;
    if (sv_DLL == 0) sv_DLL = RL_DLL;

    UART_LCR = 0x80;                // Setup registers so we can read device ID from UART
    UART_DLM = 0x00;                // to detect presence of hardware
    UART_DLL = 0x00;                // ..

    if (UART_DVID == 0x10) // Init UART to 921600 Baud 8-N-1 no flow control
    {
        UART_LCR = 0x83;         // 8-N-1
        UART_DLM = 0x00;//sv_DLM;// 921600 Baud
        UART_DLL = 0x01;//sv_DLL;// 921600 Baud
        UART_LCR = 0x03;         //
        UART_MCR = 0x08;         // Block all incoming connections
        UART_FCR = 0x07;         // Enable & reset fifos and buffer indexes
        Buffer_Flush(&RxBuffer); // Flush software buffer
        return TRUE;
    }
    else
    {
        return FALSE;
    }

    bIsInMonitorMode = FALSE;
}

// Flush Buffer
void RLN_FlushBuffers(void)
{
    UART_FCR = 0x07;          // Reset UART TX/RX hardware fifos
    Buffer_Flush(&RxBuffer);  // Flush software buffer
    return;
}

// Network TX Ready
// Returns boolean value if transmit fifo is clear to send data
bool RLN_TXReady() 
{
    return (UART_LSR & 0x20);
}

// Network RX Ready
// Returns boolean value if there is data in hardware receive fifo
bool RLN_RXReady() 
{
    return (UART_LSR & 0x01);
    //return ((UART_LSR & 0x9F) == 1);
}

// Network Send
// Sends a single byte
void RLN_SendByte(u8 data) 
{
    while (!RLN_TXReady());
    UART_THR = data;
    return;
}

// Network Read Byte
// Returns a single byte from the hardware UART receive buffer directly
u8 RLN_ReadByte(void)
{
    return UART_RHR;
}

// Network Send Message
// Sends a string of ascii 
void RLN_SendMessage(char *str) 
{
    int i=0;
    while (str[i] != '\0') { RLN_SendByte(str[i]); i++; }
}

// Network Update
// Check for data in hardware receive buffer and store it into 
// the software receive buffer. Designed to be called from Vblank 
void RLN_Update(void)
{
    u32 timeout = 0;
    //u8 n = 0;

    SYS_setInterruptMaskLevel(7);
    //if (Buffer_IsEmpty(&RxBuffer) != 0xFF) return;

    while (Buffer_IsFull(&RxBuffer) != 0xFF)
    {
        if (RLN_RXReady())
        {
            Buffer_Push(&RxBuffer, RLN_ReadByte());
        }
        else if (timeout++ >= 128)
        {
            break;
        }

        //if (n++ >= 96) break;
    }
    SYS_setInterruptMaskLevel(0);

    /*while ((RLN_RXReady()) && (!Buffer_IsFull(&RxBuffer)))
    {
        u8 byte = RLN_ReadByte();
        Buffer_Push(&RxBuffer, byte);

        //break;  // !!!
    }*/

    //if (UART_LSR & 0x01) Buffer_Push(&RxBuffer, UART_RHR);  // If RxReady then push a byte from Rx register into RxBuffer
}

// Enter Monitor Mode
void RLN_EnterMonitorMode(void)
{
    u32 timeout = 0;

    if (bIsInMonitorMode) return;

    RLN_FlushBuffers();
    RLN_SendMessage("C0.0.0.0/0\n");
    while (!RLN_RXReady() || RLN_ReadByte() != '>')
    {
        if (timeout++ >= sv_ReadTimeout) return;
    }

    bIsInMonitorMode = TRUE;
}

// Exit Monitor Mode
void RLN_ExitMonitorMode(void)
{
    u32 timeout = 0;

    if (!bIsInMonitorMode) return;

    RLN_SendMessage("QU\n");
    while (!RLN_RXReady() || RLN_ReadByte() != '>')
    {
        if (timeout++ >= sv_ReadTimeout)
        {
            RLN_FlushBuffers();
            return;
        }
    }

    RLN_FlushBuffers();
    bIsInMonitorMode = FALSE;
}

// Allows inbound TCP connections on port 5364
void RLN_AllowConnections(void)
{
    UART_MCR = 0x08;
    while (UART_MCR != 0x08);
    return;
}

// Drops any current connection and blocks future inbound connections
void RLN_BlockConnections(void)
{
    UART_MCR = 0x00;
    while (UART_MCR != 0x00);
    return;
}

// Reboot Adapter
// Reboots Xpico and waits until its back up before returning
void RLN_ResetAdapter(void)
{
    RLN_EnterMonitorMode();
    RLN_SendMessage("RS\n");

    while (1)
    { 
        while (!RLN_RXReady());
        u8 byte = RLN_ReadByte();
        if (byte == 'D') { break; }
    }

    return;
}

// Make an outbound TCP connection to supplied DNS/IP
bool RLN_Connect(char *str)
{
    RLN_SendByte('C');
    RLN_SendMessage(str);
    RLN_SendByte(0x0A);

    while (!RLN_RXReady());
    u8 byte = RLN_ReadByte();

    switch(byte)
    {
        case 'C': // Connected
            return TRUE;
        case 'N': // Host Unreachable
            //RLN_FlushBuffers();
            return FALSE;
        default:
            //RLN_FlushBuffers();
            return FALSE;
    }
}

u8 RLN_GetIP(char *ret)
{
    u32 timeout = 0;
    u8 byte = 0;
    u8 i = 0;
    u8 r = 0;

    if (ret == NULL) return 1;

    RLN_EnterMonitorMode();
    RLN_SendMessage("NC\n"); // Send command to get network information

    while (byte != 'G')
    {
        while (!RLN_RXReady())
        {
            if (timeout++ >= sv_ReadTimeout) 
            {
                r = 2;
                goto Exit;
            }
        }

        byte = RLN_ReadByte();

        if ((byte >= '0' && byte <= '9') || byte == '.' || byte == '1')
        {
            ret[i++] = byte;
            timeout = 0;

            if (i >= 30) break;
        }

        /*if (timeout++ >= sv_ReadTimeout) 
        {
            r = 2;
            break;
        }*/
    }

    Exit:
    RLN_ExitMonitorMode();

    ret[i++] = '\0';
    return r;
}

// Prints IP address of cartridge adapter
void RLN_PrintIP(int x, int y)
{
    u32 timeout = 0;

    RLN_EnterMonitorMode();
    RLN_SendMessage("NC\n"); // Send command to get network information

    while (1)
    {
        while (!RLN_RXReady()){if (timeout++ >= sv_ReadTimeout) goto Exit;}
        timeout = 0;

        u8 byte = RLN_ReadByte();
        if (byte == 'G') { break; }
        if ((byte >= '0' && byte <= '9') || byte == '.' || byte == '1')
        {
            TRM_DrawChar(byte, x, y, PAL1); x++;
        }

        if (timeout++ >= sv_ReadTimeout) 
        {
            TRM_DrawChar('T', x, y, PAL1); x++;
            TRM_DrawChar('O', x, y, PAL1); x++;
            break;
        }
    }

    Exit:
    RLN_ExitMonitorMode();
}

// Prints MAC address of cartridge hardware (Xpico)
void RLN_PrintMAC(int x, int y)
{
    u32 timeout = 0;

    RLN_EnterMonitorMode();
    RLN_SendMessage("GM\n");

    for (int i=1; i<22;i++)
    {
        while (!RLN_RXReady()){if (timeout++ >= sv_ReadTimeout) goto Exit;}
        timeout = 0;

        u8 byte = RLN_ReadByte();
        if (i>4) { TRM_DrawChar(byte, x, y, PAL1); x++; }

        if (timeout++ >= sv_ReadTimeout) 
        {
            TRM_DrawChar('T', x, y, PAL1); x++;
            TRM_DrawChar('O', x, y, PAL1); x++;
            break;
        }
    }

    Exit:
    RLN_ExitMonitorMode();
}

// Ping IP Address
// Only accepts an IP address to ping
void RLN_PingIP(char *ip)
{
    int ping_counter = 0;
    int byte_count = 0;
    int ping_count = 4;

    RLN_EnterMonitorMode();
    RLN_SendMessage("PI ");
    RLN_SendMessage(ip);
    RLN_SendMessage("\n");

    while (1)
    {
        while (!RLN_RXReady());
        u8 byte = RLN_ReadByte();
        byte_count++;

        if (byte_count > 2)
        {
            Stdout_PushByte(byte);

            if (byte == '\n') { Stdout_Push("\n"); Stdout_Flush(); ping_counter++; }
            if (ping_counter >= ping_count+1) { break; }
        }
    }

    RLN_ExitMonitorMode();
}
