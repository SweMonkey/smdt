#include <genesis.h>
#include "StateCtrl.h"
#include "DevMgr.h"
#include "Input.h"
#include "Terminal.h"
#include "devices/Keyboard_PS2.h"
#include "../res/system.h"
#include "Utils.h"
#include "HexView.h"    // bShowHexView
#include "QMenu.h"      // bShowQMenu
#include "Network.h"
#include "SRAM.h"

u8 BootNextLine = 0;    // Bootscreen text y position


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

    DMA_setBufferSize(DMA_BUFFER_SIZE_MIN);

    VDP_setHScrollTableAddress(AVR_HSCROLL); 
    VDP_setSpriteListAddress(AVR_SAT);   
    VDP_setWindowAddress(AVR_WINDOW);       
    VDP_setBGAAddress(AVR_PLANE_A);          
    VDP_setBGBAddress(AVR_PLANE_B);          

    // Make sure that previous State.Exit() is called if this is a soft reset
    if (!hardReset)
    {
        ChangeState(PS_Dummy, 0, NULL); 
    }

    PAL_setPalette(PAL0, palette_black, DMA);
    PAL_setPalette(PAL1, palette_black, DMA);
    PAL_setPalette(PAL2, palette_black, DMA);
    PAL_setPalette(PAL3, palette_black, DMA);

    PSG_init();
    PSG_setEnvelope(0, PSG_ENVELOPE_MAX);
    PSG_setTone(0, 200);
    waitMs(200);
    PSG_setEnvelope(0, PSG_ENVELOPE_MIN);
    
    PAL_setColor( 1, 0x00e);    // Icon Red
    PAL_setColor( 3, 0x0e0);    // Icon Green
    PAL_setColor( 4, 0x000);    // Cursor (Blink)
    PAL_setColor( 5, 0x000);    // Icon BG
    PAL_setColor( 6, 0xeee);    // Icon Normal
    PAL_setColor(17, 0x000);    // Window text BG Normal / Terminal text BG
    PAL_setColor(18, 0xeee);    // Window text FG Normal
    PAL_setColor(49, 0xeee);    // Window text BG Inverted / Terminal text FG? Was used for something there...
    PAL_setColor(50, 0x000);    // Window text FG Inverted

    VDP_loadTileSet(&GFX_BGBLOCKS,   AVR_BGBLOCK, DMA);
    VDP_loadTileSet(&GFX_CURSOR,     AVR_CURSOR,  DMA);
    VDP_loadTileSet(&GFX_ICONS,      AVR_ICONS,   DMA);
    VDP_loadTileSet(&GFX_ASCII_MENU, AVR_UI,      DMA);

    BootNextLine = 0;
    TRM_SetWinParam(FALSE, FALSE, 0, 1);   // Setup default window parameters
    TRM_SetWinHeight(13);                   // Change window height for boot menu
    TRM_ClearTextArea(0, 0, 40, 13);
    TRM_DrawText("Initializing system...", 1, BootNextLine++, PAL1);

    bPALSystem = IS_PAL_SYSTEM;

    if (bPALSystem)
    {
        VDP_setScreenHeight240();
    }

    VDP_setReg(0xB, 0x8);   // Enable VDP ext interrupt (Enable: 8 - Disable: 0)
    SYS_setInterruptMaskLevel(0);
    SYS_setExtIntCallback(Ext_IRQ);

    Input_Init();

    TRM_SetStatusIcon(ICO_ID_UNKNOWN, ICO_POS_0);
    TRM_SetStatusIcon(ICO_NET_IDLE_RECV, ICO_POS_1);
    TRM_SetStatusIcon(ICO_NET_IDLE_SEND, ICO_POS_2);

    TRM_DrawText("Loading config...", 1, BootNextLine++, PAL1);
    if (SRAM_LoadData()) 
    {
        TRM_DrawText("Failed to load config from SRAM...", 1, BootNextLine++, PAL1);
        SRAM_SaveData();
    }
    else TRM_DrawText("Successfully loaded config from SRAM", 1, BootNextLine++, PAL1);
    
    TRM_DrawText("Configuring devices...", 1, BootNextLine++, PAL1);
    ConfigureDevices();

    bShowHexView = FALSE;
    bShowQMenu = FALSE;

    #if (HALT_Z80_ON_IO != 0)
    BootNextLine++;
    TRM_DrawText("Warning: HALT_Z80_ON_IO is enabled", 1, BootNextLine++, PAL1);
    TRM_DrawText("in SGDK! This may cause issues!", 1, BootNextLine++, PAL1);
    #endif 
    #if (HALT_Z80_ON_DMA != 0)
    BootNextLine++;
    TRM_DrawText("Warning: HALT_Z80_ON_DMA is enabled", 1, BootNextLine++, PAL1);
    TRM_DrawText("in SGDK! This may cause issues!", 1, BootNextLine++, PAL1);
    #endif

    #if ((HALT_Z80_ON_DMA != 0) || (HALT_Z80_ON_IO != 0))
    BootNextLine++;
    char cntbuf[16];
    for (u8 i = 0; i < 10; i++)
    {
        sprintf(cntbuf, "Resuming boot in %u seconds", 9-i);
        TRM_DrawText(cntbuf, 1, BootNextLine, PAL1);
        
        waitMs(1000);
    }
    BootNextLine++;
    #endif

    SYS_enableInts();

    SYS_doVBlankProcess();

    // Show "boot" screen for a second
    #ifndef EMU_BUILD
    waitMs(2000);
    #endif

    TRM_ResetWinParam();
    
    //ChangeState(PS_Debug, 0, NULL);
    //ChangeState(PS_Telnet, 0, NULL);
    //ChangeState(PS_Entry, 0, NULL);
    ChangeState(PS_Terminal, 0, NULL);

    while(TRUE)
    {
        StateTick();
        SYS_doVBlankProcess();
    }

    return 0;
}
