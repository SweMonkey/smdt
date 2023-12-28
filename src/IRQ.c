
#include "IRQ.h"
#include "DevMgr.h"   // Port definitions
#include "Buffer.h"

static u8 CursorBlink = 0; // Cursor blink counter
extern SM_Device DEV_UART;
u16 Cursor_CL = 0x0E0;

void VB_IRQ()
{
    // Cursor blink
    if (CursorBlink == 20)
    {
        PAL_setColor(31, Cursor_CL);
        CursorBlink++;
    }
    else if (CursorBlink == 40)
    {
        PAL_setColor(31, 0x000);
        CursorBlink = 0;
    }
    else CursorBlink++;
}

void HB_IRQ()
{
}

void Ext_IRQ()
{
    vu8 *byte = (u8*)DEV_UART.RxData;
    Buffer_Push(&RxBuffer, *byte);
}
