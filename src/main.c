
#include <genesis.h>
#include "StateCtrl.h"
#include "DevMgr.h"
#include "Input.h"
#include "Terminal.h"
#include "Keyboard_PS2.h"
#include "../res/system.h"
#include "Utils.h"
#include "HexView.h"    // bShowHexView
#include "QMenu.h"    // bShowQMenu
#include "IRQ.h"


int main(bool hardReset)
{
    SYS_disableInts();

    Z80_unloadDriver();
    Z80_requestBus(TRUE);   // Make sure SGDK library is built with HALT_Z80_ON_IO and HALT_Z80_ON_DMA set to 0, to make sure the bus never gets released again
    //Z80_getAndRequestBus(TRUE);

    #if (HALT_Z80_ON_IO != 0)
    kprintf("Warning: HALT_Z80_ON_IO is enabled!");
    #endif 
    #if (HALT_Z80_ON_DMA != 0)
    kprintf("Warning: HALT_Z80_ON_DMA is enabled!");
    #endif 

    VDP_setHScrollTableAddress(0xA000); // 0xA000 - 0xA3FF  HScroll
    VDP_setSpriteListAddress(0xAC00);   // 0xAC00 - 0xAFFF  SAT
    VDP_setWindowAddress(0xB000);       // 0xB000 - 0xBFFF  Window
    VDP_setBGAAddress(0xC000);          // 0xC000 - 0xDFFF  BG A
    VDP_setBGBAddress(0xE000);          // 0xE000 - 0xFFFF  BG B

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

    VDP_setWindowVPos(FALSE, 4);
    TRM_clearTextArea(0, 0, 40, 10);
    TRM_drawText("Initializing system...", 1, 0, PAL1);

    bPALSystem = IS_PAL_SYSTEM;

    if (bPALSystem)
    {
        VDP_setScreenHeight240();
    }

    VDP_setReg(0xB, 0x8);   // VDP ext interrupt
    SYS_setInterruptMaskLevel(0);
    SYS_setExtIntCallback(Ext_IRQ);

    Input_Init();

    print_charXY_WP(ICO_KB_UNKNOWN, STATUS_KB_POS, CHAR_WHITE);
    print_charXY_WP(ICO_NET_IDLE_RECV, STATUS_NET_RECV_POS, CHAR_WHITE);
    print_charXY_WP(ICO_NET_IDLE_SEND, STATUS_NET_SEND_POS, CHAR_WHITE);

    TRM_drawText("Configuring devices...", 1, 1, PAL1);
    ConfigureDevices();

    bShowHexView = FALSE;
    bShowQMenu = FALSE;

    SYS_enableInts();

    SYS_doVBlankProcess();

    // Show "boot" screen for a second
    waitMs(1000);

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
