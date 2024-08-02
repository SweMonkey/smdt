#include <genesis.h>
#include "StateCtrl.h"
#include "DevMgr.h"
#include "Input.h"
#include "../res/system.h"
#include "Utils.h"
#include "HexView.h"    // bShowHexView
#include "QMenu.h"      // bShowQMenu
#include "UI.h"         // UI_ApplyTheme
#include "Network.h"
#include "SRAM.h"
#include "Terminal.h"
#include "Telnet.h"
#include "system/Stdout.h"


int main(bool hardReset)
{
    VDP_setEnable(FALSE);
    SYS_disableInts();

    Z80_unloadDriver();
    Z80_requestBus(TRUE);   // Make sure SGDK library is built with HALT_Z80_ON_IO and HALT_Z80_ON_DMA set to 0, to make sure the bus never gets released again

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

    bHardReset = hardReset;

    // Make sure that previous State.Exit() is called if this is a soft reset
    if (!bHardReset)
    {
        ChangeState(PS_Dummy, 0, NULL); 
        SetSprite_Y(SPRITE_ID_CURSOR, 0);
    }

    bPALSystem = IS_PAL_SYSTEM;

    if (bPALSystem)
    {
        VDP_setScreenHeight240();
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
    
    // Setup initial colour values needed for boot
    PAL_setColor(17, 0x000);    // Window text BG Normal / Terminal text BG
    PAL_setColor(20, 0x444);    // Window title BG
    PAL_setColor(50, 0x000);    // Window text FG Inverted
    PAL_setColor(51, 0xEEE);    // Window text BG Inverted
    PAL_setColor(54, 0x444);    // Screensaver colour 0
    PAL_setColor(55, 0xEEE);    // Screensaver colour 1

    // Reset window plane to be fully transparent
    TRM_ClearArea(0, 0, 40, (bPALSystem ? 30 : 28), PAL1, TRM_CLEAR_BG);

    // Setup default window parameters
    TRM_SetWinParam(FALSE, FALSE, 0, 1);

    // Upload initial tilesets to VRAM
    VDP_loadTileSet(&GFX_BGBLOCKS,   AVR_BGBLOCK, DMA);
    VDP_loadTileSet(&GFX_POINTER,    AVR_POINTER, DMA);
    VDP_loadTileSet(&GFX_CURSOR,     AVR_CURSOR,  DMA);
    VDP_loadTileSet(&GFX_ICONS,      AVR_ICONS,   DMA);
    VDP_loadTileSet(&GFX_ASCII_MENU, AVR_UI,      DMA);
    VDP_loadTileSet(&GFX_SCRSAV,     AVR_SCRSAV,  DMA);

    // Initialize terminal for boot output text
    sv_Font = FONT_8x8_16;
    sv_HSOffset = 8;
    TELNET_Init();
    vNewlineConv = 1;
    bAutoFlushStdout = TRUE;
    
    VDP_setEnable(TRUE);
 
    Stdout_Push("Initializing system...\n");

    Input_Init();

    TRM_SetStatusIcon(ICO_ID_UNKNOWN,    ICO_POS_0);
    TRM_SetStatusIcon(ICO_NET_IDLE_RECV, ICO_POS_1);
    TRM_SetStatusIcon(ICO_NET_IDLE_SEND, ICO_POS_2);
    TRM_SetStatusIcon(ICO_NONE,          ICO_POS_3);

    Stdout_Push("Loading config...\n");
    if (SRAM_LoadData()) 
    {
        Stdout_Push("Failed to load config from SRAM...\n");
        SRAM_SaveData();
    }
    else Stdout_Push("Successfully loaded config from SRAM\n");
        
    VDP_setReg(0xB, 0x8);               // Enable VDP ext interrupt (Enable: 8 - Disable: 0)
    SYS_setInterruptMaskLevel(0);       // Enable all interrupts
    SYS_setExtIntCallback(NET_RxIRQ);   // Set external IRQ callback

    // Enable interrupts during driver init, certain devices will need ExtIRQ working for detection
    SYS_enableInts();

    Stdout_Push("Configuring devices...\n");
    DeviceManager_Init();

    SYS_disableInts();
    
    bShowHexView = FALSE;
    bShowQMenu = FALSE;

    #if (HALT_Z80_ON_IO != 0)
    Stdout_Push("\n[91mWarning: HALT_Z80_ON_IO is enabled\n");
    Stdout_Push("in SGDK! This may cause issues![0m\n\n");
    #endif 
    #if (HALT_Z80_ON_DMA != 0)
    Stdout_Push("\n[91mWarning: HALT_Z80_ON_DMA is enabled\n");
    Stdout_Push("in SGDK! This may cause issues![0m\n\n");
    #endif

    #if ((HALT_Z80_ON_DMA != 0) || (HALT_Z80_ON_IO != 0))
    for (u8 i = 0; i < 10; i++)
    {
        stdout_printf(" Resuming boot in %u seconds\r", 9-i);
        
        waitMs(1000);
    }
    Stdout_Push("\n");
    #endif

    bAutoFlushStdout = FALSE;

    SYS_enableInts();

    SYS_doVBlankProcess();

    // Show "boot" screen for a few more seconds
    #ifndef EMU_BUILD
    waitMs(1000);
    #endif
    
    //waitMs(1000);

    // Setup icon and default window colours
    PAL_setColor( 1, 0x00E);    // Icon Red
    PAL_setColor( 2, 0xEEE);    // Window title FG
    PAL_setColor( 3, 0x444);    // Window title BG
    PAL_setColor( 4, 0x0E0);    // Cursor
    PAL_setColor( 5, 0x222);    // Icon BG
    PAL_setColor( 6, 0xEEE);    // Icon Normal
    PAL_setColor( 7, 0x0E0);    // Icon Green (Previously in slot 3)
    PAL_setColor(18, 0xEEE);    // Window text FG Normal - This is set to black during boot, revert it back
    PAL_setColor(19, 0x222);    // Window inner BG       - This is set to black during boot, revert it back
    UI_ApplyTheme();

    // Set VBlank IRQ callback - Do not set it earlier in boot process!
    SYS_setVBlankCallback(VBlank);
    
    //ChangeState(PS_Debug, 0, NULL);
    //ChangeState(PS_Telnet, 0, NULL);
    //ChangeState(PS_Entry, 0, NULL);
    //ChangeState(PS_IRC, 0, NULL);
    ChangeState(PS_Terminal, 0, NULL);

    while(TRUE)
    {
        StateTick();
        SYS_doVBlankProcess();
    }

    return 0;
}
