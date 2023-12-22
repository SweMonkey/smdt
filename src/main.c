
#include <genesis.h>
#include "StateCtrl.h"
#include "Input.h"
#include "Terminal.h"
#include "Keyboard_PS2.h"
#include "../res/system.h"
#include "Utils.h"


int main(bool hardReset)
{
    SYS_disableInts();

    Z80_unloadDriver();
    Z80_requestBus(TRUE);
    //Z80_getAndRequestBus(TRUE);

    VDP_setWindowAddress(0x9000);   // 0x9000 - 0x9FFF
    VDP_setHScrollTableAddress(0xA000); // 0xA000 - 0xA3FF
    VDP_setSpriteListAddress(0xAC00);   // 0xAC00 - 0xAFFF
    VDP_setBGAAddress(0xC000);    // 0xC000 - 0xDFFF
    VDP_setBGBAddress(0xE000);    // 0xE000 - 0xFFFF

    bPALSystem = IS_PAL_SYSTEM;

    if (bPALSystem)
    {
        VDP_setScreenHeight240();
    }

    PAL_setPalette(PAL0, palette_black, DMA);
    PAL_setPalette(PAL1, palette_black, DMA);
    PAL_setPalette(PAL2, palette_black, DMA);
    PAL_setPalette(PAL3, palette_black, DMA);
    
    PAL_setColor( 1, 0x00e);    // Icon Red
    PAL_setColor( 2, 0x0e0);    // Icon Green
    PAL_setColor(14, 0x000);    // Icon BG
    PAL_setColor(15, 0xeee);    // Icon Normal
    PAL_setColor(17, 0x000);    // Window text BG Normal / Terminal text BG
    PAL_setColor(18, 0xeee);    // Window text FG Normal
    PAL_setColor(49, 0xeee);    // Window text BG Inverted / Terminal text FG? Was used for something there...
    PAL_setColor(50, 0x000);    // Window text FG Inverted

    VDP_loadTileSet(&GFX_ASCII_MENU, 0x220, DMA);
    VDP_loadTileSet(&GFX_ICONS, 0x18, DMA);

    Input_Init();

    VDP_setWindowVPos(FALSE, 1);
    TRM_clearTextArea(0, 0, 40, 10);
    TRM_drawText(STATUS_TEXT, 1, 0, PAL1);

    print_charXY_WP(ICO_KB_UNKNOWN, STATUS_KB_POS, CHAR_WHITE);
    print_charXY_WP(ICO_NET_IDLE_RECV, STATUS_NET_RECV_POS, CHAR_WHITE);
    print_charXY_WP(ICO_NET_IDLE_SEND, STATUS_NET_SEND_POS, CHAR_WHITE);

    SYS_enableInts();

    SYS_doVBlankProcess();

    //ChangeState(PS_Debug, 0, NULL);
    //ChangeState(PS_Telnet, 0, NULL);
    ChangeState(PS_Entry, 0, NULL);

    while(TRUE)
    {
        StateTick();
        SYS_doVBlankProcess();
    }

    return 0;
}
