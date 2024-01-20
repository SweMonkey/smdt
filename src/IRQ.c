
#include "IRQ.h"
#include "DevMgr.h"   // Port definitions
#include "Buffer.h"

static u8 CursorBlink = 0; // Cursor blink counter
extern SM_Device DEV_UART;
extern u8 FontSize;
extern bool bPALSystem;
u16 Cursor_CL = 0x0E0;

void VB_IRQ()
{
    // Cursor blink
    if (CursorBlink == (bPALSystem?12:15))
    {
        *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_CRAM_ADDR((u32)4);
        *((vu16*) VDP_DATA_PORT) = 0x0E0;
        *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_CRAM_ADDR((u32)62);
        *((vu16*) VDP_DATA_PORT) = Cursor_CL;
        CursorBlink++;
    }
    else if (CursorBlink == (bPALSystem?24:30))
    {
        *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_CRAM_ADDR((u32)4);
        *((vu16*) VDP_DATA_PORT) = 0;
        *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_CRAM_ADDR((u32)62);
        *((vu16*) VDP_DATA_PORT) = 0;
        CursorBlink = 0;
    }
    else CursorBlink++;
}
