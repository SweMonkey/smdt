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

Modified for use in SMDTC by smds
*/

#include "RL_Network.h"
#include "Buffer.h"
#include "Utils.h"      // TRM
#include "Network.h"    // RxBuffer/TxBuffer


// Initialize Network Adapter
// Detects adapter and returns TRUE or FALSE boolean value
bool RLN_Initialize()
{
    UART_LCR = 0x80;                // Setup registers so we can read device ID from UART
    UART_DLM = 0x00;                // to detect presence of hardware
    UART_DLL = 0x00;                // ..

    if (UART_DVID == 0x10) // Init UART to 921600 Baud 8-N-1 no flow control
    {
        UART_LCR = 0x83;    // 8-N-1
        UART_DLM = 0x00;    // 921600 Baud
        UART_DLL = 0x01;    // 921600 Baud
        UART_LCR = 0x03;    //
        UART_MCR = 0x08;    // Block all incoming connections
        UART_FCR = 0x07;    // Enable & reset fifos and buffer indexes
        Buffer_Flush(&RxBuffer);  // Flush software buffer
        return TRUE;
    }
    else
    {
        return FALSE;
    }
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
}

// Network Send
// Sends a single byte
void RLN_SendByte(u8 data) 
{
    while (!RLN_TXReady());
    UART_RHR = data;
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
    while(RLN_RXReady())
    { 
        if (Buffer_IsFull(&RxBuffer)) break;

        u8 byte = RLN_ReadByte();
        Buffer_Push(&RxBuffer, byte);
    }
}

// Enter Monitor Mode
void RLN_EnterMonitorMode(void)
{
    RLN_FlushBuffers();
    RLN_SendMessage("C0.0.0.0/0\n");
    while (!RLN_RXReady() || RLN_ReadByte() != '>') {}
}

// Exit Monitor Mode
void RLN_ExitMonitorMode(void)
{
    RLN_SendMessage("QU\n");
    while (!RLN_RXReady() || RLN_ReadByte() != '>') {}
    RLN_FlushBuffers();
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
    RLN_SendByte('C');RLN_SendMessage(str); RLN_SendByte(0x0A);

    while (!RLN_RXReady());
    u8 byte = RLN_ReadByte();

    switch(byte)
    {
        case 'C': // Connected
            return TRUE;
        case 'N': // Host Unreachable
            RLN_FlushBuffers();
            return FALSE;
        default:
            RLN_FlushBuffers();
            return FALSE;
    }
}

// Prints IP address of cartridge adapter
void RLN_PrintIP(int x, int y)
{
    RLN_EnterMonitorMode();
    RLN_SendMessage("NC\n"); // Send command to get network information

    while (1)
    {
        while (!RLN_RXReady());
        u8 byte = RLN_ReadByte();
        if (byte == 'G') { break; }
        if ((byte >= '0' && byte <= '9') || byte == '.' || byte == '1')
        {
            TRM_DrawChar(byte, x, y, PAL1); x++;
        }
    }

    RLN_ExitMonitorMode();
}

// Prints MAC address of cartridge hardware (Xpico)
void RLN_PrintMAC(int x, int y)
{
    RLN_EnterMonitorMode();
    RLN_SendMessage("GM\n");

    for (int i=1; i<22;i++)
    {
        while (!RLN_RXReady());
        u8 byte = RLN_ReadByte();
        if (i>4) { TRM_DrawChar(byte, x, y, PAL1); x++; }
    }

    RLN_ExitMonitorMode();
}

// Ping IP Address
// Only accepts an IP address to ping
void RLN_PingIP(int x, int y, int ping_count, char *ip)
{
    int ping_counter = 0;
    int byte_count = 0;
    int tmp = x;

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
            TRM_DrawChar(byte, x, y, PAL1); x++;

            if (byte == '\n') { x=tmp; y++; ping_counter++; }
            if (ping_counter >= ping_count+1) { break; }
        }
    }

    RLN_ExitMonitorMode();
}
