
#include "Network.h"
#include "Buffer.h"
#include "DevMgr.h"
#include "Terminal.h"   // vLineMode
#include "Telnet.h"     // LMSM define
#include "Utils.h"      // EMU_BUILD define, TRM

// Statistics
u32 RXBytes = 0;
u32 TXBytes = 0;


Buffer RxBuffer, TxBuffer;
SM_Device DEV_UART;

// Rx IRQ
void Ext_IRQ()
{
    vu8 *PSCTRL = (u8*)DEV_UART.SCtrl;
    if (((*PSCTRL >> 2) & 1) || !((*PSCTRL >> 1) & 1)) return;   // if (Error=1 || Ready=0)

    SYS_setInterruptMaskLevel(7);

    vu8 *byte = (u8*)DEV_UART.RxData;
    Buffer_Push(&RxBuffer, *byte);

    SYS_setInterruptMaskLevel(0);
}

// Send byte to remote machine or buffer it depending on linemode
void NET_SendChar(const u8 c, u8 flags)
{
    #ifdef EMU_BUILD
    return;
    #endif

    if ((vLineMode & LMSM_EDIT) && ((flags & TXF_NOBUFFER) == 0))
    {
        Buffer_Push(&TxBuffer, c);
        return;
    }

    vu8 *PTX = (vu8 *)DEV_UART.TxData;
    vu8 *PSCTRL = (vu8 *)DEV_UART.SCtrl;

    TRM_SetStatusIcon(ICO_NET_SEND, STATUS_NET_SEND_POS, CHAR_RED);

    while (*PSCTRL & 1) // while Txd full = 1
    {
        PSCTRL = (vu8 *)DEV_UART.SCtrl;
    }

    *PTX = c;

    TXBytes++;

    TRM_SetStatusIcon(ICO_NET_IDLE_SEND, STATUS_NET_SEND_POS, CHAR_WHITE);
}

// Pop and transmit data in TxBuffer
void NET_TransmitBuffer()
{
    #ifdef EMU_BUILD
    return;
    #endif

    vu8 *PTX = (vu8 *)DEV_UART.TxData;
    vu8 *PSCTRL = (vu8 *)DEV_UART.SCtrl;
    u8 data;

    TRM_SetStatusIcon(ICO_NET_SEND, STATUS_NET_SEND_POS, CHAR_RED);

    while (Buffer_Pop(&TxBuffer, &data) != 0xFF)
    {
        while (*PSCTRL & 1) // while Txd full = 1
        {
            PSCTRL = (vu8 *)DEV_UART.SCtrl;
        }

        *PTX = data;

        TXBytes++;
    }

    TRM_SetStatusIcon(ICO_NET_IDLE_SEND, STATUS_NET_SEND_POS, CHAR_WHITE);
}

void NET_SendString(const char *str)
{
    u16 len = strlen(str);

    for (u16 c = 0; c < len; c++)
    {
        NET_SendChar(str[c], TXF_NOBUFFER);
    }
}
