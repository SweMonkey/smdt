#include "Network.h"
#include "Buffer.h"
#include "DevMgr.h"
#include "Terminal.h"           // vLineMode
#include "Telnet.h"             // LMSM define
#include "Utils.h"              // EMU_BUILD define, TRM
#include "devices/RL_Network.h" // RLN_SendByte

// Statistics
u32 RXBytes = 0;
u32 TXBytes = 0;

Buffer RxBuffer, TxBuffer;
SM_Device DEV_UART;
NET_Connect_CB *ConnectCB = NULL;


// Rx IRQ
void Ext_IRQ()
{
    vu8 *PSCTRL = (vu8*)DEV_UART.SCtrl;
    if ((*PSCTRL & 6) != 2) return;

    SYS_setInterruptMaskLevel(7);

    vu8 *byte = (vu8*)DEV_UART.RxData;
    Buffer_Push(&RxBuffer, *byte);

    SYS_setInterruptMaskLevel(0);
}

void NET_SetConnectFunc(NET_Connect_CB *cb)
{
    ConnectCB = cb;
}

bool NET_Connect(char *str)
{
    if (ConnectCB == NULL) return FALSE;

    return ConnectCB(str);
}

// Send byte to remote machine or buffer it depending on linemode
void NET_SendChar(const u8 c, u8 flags)
{
    #ifdef EMU_BUILD
    #warning EMU_BUILD active, key input from keyboard wont be buffered!
    return;
    #endif

    if ((vLineMode & LMSM_EDIT) && ((flags & TXF_NOBUFFER) == 0))
    {
        Buffer_Push(&TxBuffer, c);
        return;
    }

    TRM_SetStatusIcon(ICO_NET_SEND, ICO_POS_2);

    if (bRLNetwork)
    {
        RLN_SendByte(c);
    }
    else
    {
        vu8 *PTX = (vu8 *)DEV_UART.TxData;
        vu8 *PSCTRL = (vu8 *)DEV_UART.SCtrl;

        while (*PSCTRL & 1) // while Txd full = 1
        {
            PSCTRL = (vu8 *)DEV_UART.SCtrl;
        }

        *PTX = c;
    }

    TXBytes++;

    TRM_SetStatusIcon(ICO_NET_IDLE_SEND, ICO_POS_2);
}

// Pop and transmit data in TxBuffer
void NET_TransmitBuffer()
{
    #ifdef EMU_BUILD
    return;
    #endif

    TRM_SetStatusIcon(ICO_NET_SEND, ICO_POS_2);

    if (bRLNetwork)
    {
        u8 data;

        while (Buffer_Pop(&TxBuffer, &data) != 0xFF)
        {
            RLN_SendByte(data);

            TXBytes++;
        }
    }
    else
    {
        vu8 *PTX = (vu8 *)DEV_UART.TxData;
        vu8 *PSCTRL = (vu8 *)DEV_UART.SCtrl;
        u8 data;

        while (Buffer_Pop(&TxBuffer, &data) != 0xFF)
        {
            while (*PSCTRL & 1) // while Txd full = 1
            {
                PSCTRL = (vu8 *)DEV_UART.SCtrl;
            }

            *PTX = data;

            TXBytes++;
        }
    }

    TRM_SetStatusIcon(ICO_NET_IDLE_SEND, ICO_POS_2);
}

void NET_SendString(const char *str)
{
    for (u16 c = 0; c < strlen(str); c++)
    {
        NET_SendChar(str[c], TXF_NOBUFFER);
    }
}
