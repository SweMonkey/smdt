#include "Network.h"
#include "Buffer.h"
#include "DevMgr.h"
#include "Terminal.h"           // v_LineMode
#include "Telnet.h"             // LMSM define
#include "Utils.h"              // EMU_BUILD define, TRM

#include "StateCtrl.h"

// Statistics
u32 RXBytes = 0;
u32 TXBytes = 0;
u16 TxUpdate = 0;

Buffer RxBuffer, TxBuffer;
SM_Device DRV_UART;

NET_Connect_CB *ConnectCB = NULL;
NET_Disconnect_CB *DisconnectCB = NULL;
NET_GetIP_CB *GetIPCB = NULL;
NET_PingIP_CB *PingIPCB = NULL;


// Rx IRQ
void NET_RxIRQ()
{
    vu8 RxData;
    vu16 RxFail;

    __asm__ __volatile__
    (
        "moveq   #1, %1            \n\t"  // Set RxFail to true
        "btst    #2, (%[serial])   \n\t"  // RX error?
        "bne.s   1f                \n\t"
        "btst    #1, (%[serial])   \n\t"  // RX ready?
        "beq.s   1f                \n\t"
        "move.b  -2(%[serial]), %0 \n\t"  // Move received byte to RxData variable
        "moveq   #0, %1            \n\t"  // Set RxFail to false to indicate a successful byte was received
        "1:"
        : /* outputs */
            "=d"(RxData), "=d"(RxFail)
        : /* inputs */
            [serial] "a"(DRV_UART.SCtrl)
        : /* clobbered regs */
            "cc"
    );

    if (!RxFail) Buffer_Push_IRQ(&RxBuffer, RxData);
}

void NET_SendChar(const u8 c)
{
    #ifdef EMU_BUILD
    return;
    #endif
    
    // Flush any buffered data before sending new data
    // if (Buffer_IsEmpty(&TxBuffer) == FALSE) NET_TransmitBuffer();

    if (bRLNetwork)
    {
        RLN_SendByte(c);
    }
    else
    {
        __asm__ __volatile__
        (
            "1:                         \n\t"
            "btst #0, (%[serial])       \n\t"   // Tx full?
            "bne.s 1b                   \n\t"   // If so loop back until it is empty
            "move.b %[c], -4(%[serial]) \n\t"   // Send byte 'c' to Tx register
        : /* outputs */
        : /* inputs */
            [c] "d"(c), [serial] "a"(DRV_UART.SCtrl)
        : /* clobbered regs */
            "cc"
        );
    }

    TxUpdate = 1;
    TXBytes++;
}

void NET_BufferChar(const u8 c)
{    
    if ((v_LineMode & LMSM_EDIT) == 0) NET_SendChar(c);
    else Buffer_Push_IRQ(&TxBuffer, c);
}

// Pop and transmit data in TxBuffer
void NET_TransmitBuffer()
{
    u8 data;

    while (Buffer_Pop(&TxBuffer, &data))  NET_SendChar(data);
}

void NET_SendString(const char *str)
{
    while (*str) NET_SendChar(*str++);
}

void NET_SendStringLen(const char *str, u16 len)
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
