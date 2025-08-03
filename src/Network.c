#include "Network.h"
#include "Buffer.h"
#include "DevMgr.h"
#include "Terminal.h"           // vLineMode
#include "Telnet.h"             // LMSM define
#include "Utils.h"              // EMU_BUILD define, TRM

// Statistics
u32 RXBytes = 0;
u32 TXBytes = 0;

Buffer RxBuffer, TxBuffer;
SM_Device DRV_UART;

SM_File *rxbuf; // RxBuffer as an IO file
SM_File *txbuf; // TxBuffer as an IO file

NET_Connect_CB *ConnectCB = NULL;
NET_Disconnect_CB *DisconnectCB = NULL;
NET_GetIP_CB *GetIPCB = NULL;
NET_PingIP_CB *PingIPCB = NULL;


// Rx IRQ
void NET_RxIRQ()
{
    if ((*(vu8*)DRV_UART.SCtrl & 6) != 2) return;   // Check Ready/RxError flag in serial control register, Bail if no byte is ready or if there was an Rx error

    SYS_setInterruptMaskLevel(7);
    Buffer_Push(&RxBuffer, *(vu8*)DRV_UART.RxData);
    SYS_setInterruptMaskLevel(0);

    //RXBytes++;
}

inline void NET_SendChar(const u8 c)
{
    #ifdef EMU_BUILD
    return;
    #endif

    if (bRLNetwork)
    {
        RLN_SendByte(c);
    }
    else
    {
        __asm__ __volatile__
        (
            "1:                         \n\t"
            "btst #0, (%[serial])       \n\t"   // Test serial control register bit 0
            "bne.s 1b                   \n\t"   // and wait until Tx is not full
            "move.b %[c], -4(%[serial]) \n\t"   // Send byte 'c' to Tx register
        : /* outputs */
        : /* inputs */
            [c] "d"(c), [serial] "a"((vu8*)DRV_UART.SCtrl)
        : /* clobbered regs */
        );
    }

    TXBytes++;
}

inline void NET_BufferChar(const u8 c)
{    
    if ((vLineMode & LMSM_EDIT) == 0) NET_SendChar(c);
    else Buffer_Push(&TxBuffer, c);
}

// Pop and transmit data in TxBuffer
inline void NET_TransmitBuffer()
{
    u8 data;

    while (Buffer_Pop(&TxBuffer, &data)) NET_SendChar(data);
}

inline void NET_SendString(const char *str)
{
    while (*str) NET_SendChar(*str++);
}

inline void NET_SendStringLen(const char *str, u16 len)
{
    for (u16 i = 0; i < len; i++)
    {
        NET_SendChar(str[i]);
    }
}


// Network callback functions

// Connect
void NET_SetConnectFunc(NET_Connect_CB *cb)
{
    ConnectCB = cb;
}

bool NET_Connect(char *str)
{
    if (ConnectCB == NULL) return FALSE;

    return ConnectCB(str);
}

// Disconnect
void NET_SetDisconnectFunc(NET_Disconnect_CB *cb)
{
    DisconnectCB = cb;
}

void NET_Disconnect()
{
    if (DisconnectCB == NULL) return;

    DisconnectCB();
    return;
}

// Get IP address string
void NET_SetGetIPFunc(NET_GetIP_CB *cb)
{
    GetIPCB = cb;
}

u8 NET_GetIP(char *str)
{
    if (GetIPCB == NULL) return 1;

    return GetIPCB(str);
}

// Ping IP address
void NET_SetPingFunc(NET_PingIP_CB *cb)
{
    PingIPCB = cb;
}

u8 NET_PingIP(char *ip)
{
    if (PingIPCB == NULL) return 2;

    return PingIPCB(ip);
}
