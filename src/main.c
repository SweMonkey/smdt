#include <genesis.h>
#include "StateCtrl.h"
#include "DevMgr.h"
#include "Input.h"
#include "../res/system.h"
#include "Utils.h"
#include "UI.h"         // UI_ApplyTheme
#include "Network.h"
#include "Terminal.h"
#include "Telnet.h"
#include "WinMgr.h"
#include "Palette.h"

#include "misc/ConfigFile.h"
#include "misc/Exception.h"

#include "system/PseudoFile.h"
#include "system/File.h"
#include "system/Filesystem.h"


int main(bool hardReset)
{
    VDP_setEnable(FALSE);
    SYS_disableInts();

    SetupExceptions();

    sv_CBrightness = 0; // Set full brightness during boot

    SetPalette(PAL0, palette_black);
    SetPalette(PAL1, palette_black);
    SetPalette(PAL2, palette_black);
    SetPalette(PAL3, palette_black);
    UploadPalette();

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
        RXBytes = 0;
        TXBytes = 0;
    }

    bPALSystem = IS_PAL_SYSTEM;

    if (bPALSystem)
    {
        VDP_setScreenHeight240();
    }

    PSG_init();
    PSG_setEnvelope(0, PSG_ENVELOPE_MAX);
    PSG_setTone(0, 200);
    waitMs(200);    
    PSG_setEnvelope(0, PSG_ENVELOPE_MIN);
    
    // Setup initial colour values needed for boot
    SetColor(17, 0x000);    // Window text BG Normal / Terminal text BG
    SetColor(20, 0x444);    // Window title BG
    SetColor(49, 0xEEE);    // Inverted cursor outline
    SetColor(50, 0x000);    // Window text FG Inverted / Inverted cursor inner
    SetColor(51, 0xEEE);    // Window text BG Inverted
    SetColor(54, 0x444);    // Screensaver colour 0
    SetColor(55, 0xEEE);    // Screensaver colour 1

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
    //sv_BoldFont = TRUE;
    TELNET_Init(TF_Everything);
    vNewlineConv = 1;
    bAutoFlushStdout = TRUE;
    SetColor(4, 0);    // Set cursor colour back to black to hide it
    UploadPalette();

    // Setup permanent sprite links
    SetSprite_SIZELINK(SPRITE_ID_CURSOR, SPR_SIZE_1x1, SPRITE_ID_SCRSAV);
    SetSprite_SIZELINK(SPRITE_ID_SCRSAV, SPR_WIDTH_4x1 | SPR_HEIGHT_1x4, SPRITE_ID_POINTER);
    SetSprite_SIZELINK(SPRITE_ID_POINTER, SPR_SIZE_1x1, 0);
    
    VDP_setEnable(TRUE);
 
    Stdout_Push(" [97mInitializing system...[0m\n");

    WinMgr_Init();

    TRM_SetStatusIcon(ICO_ID_UNKNOWN,    ICO_POS_0);
    TRM_SetStatusIcon(ICO_NET_IDLE_RECV, ICO_POS_1);
    TRM_SetStatusIcon(ICO_NET_IDLE_SEND, ICO_POS_2);
    TRM_SetStatusIcon(ICO_NONE,          ICO_POS_3);

    Stdout_Push(" [97mMounting filesystems...[0m\n");
    FS_Init();

    Stdout_Push(" [97mLoading system configuration...[0m\n");
    if (CFG_LoadData()) 
    {
        Stdout_Push(" [91mFailed to load config file![0m\n");
        CFG_SaveData();
    }
    else Stdout_Push(" [92mSuccessfully loaded config file[0m\n");

    Input_Init();
    
    SYS_setExtIntCallback(NET_RxIRQ);   // Set external IRQ callback

    // Enable interrupts during driver init, certain devices will need ExtIRQ working for detection
    VDP_setReg(0xB, 0x8);               // Enable VDP ext interrupt (Enable: 8 - Disable: 0)
    SYS_enableInts();
    SYS_setInterruptMaskLevel(0);       // Enable all interrupts

    Stdout_Push(" [97mConfiguring devices...[0m\n");
    DeviceManager_Init();

    SYS_disableInts();

    #if (HALT_Z80_ON_IO != 0)
    Stdout_Push("\n [91mWarning: HALT_Z80_ON_IO is enabled");
    Stdout_Push("\n in SGDK! This may cause issues![0m\n\n");
    #endif 
    #if (HALT_Z80_ON_DMA != 0)
    Stdout_Push("\n [91mWarning: HALT_Z80_ON_DMA is enabled");
    Stdout_Push("\n in SGDK! This may cause issues![0m\n\n");
    #endif

    #if ((HALT_Z80_ON_DMA != 0) || (HALT_Z80_ON_IO != 0))
    for (u8 i = 0; i < 10; i++)
    {
        printf(" Resuming boot in %u seconds\r", 9-i);
        Stdout_Flush();
        
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

    // Setup icon and default window colours
    SetColor( 1, 0x00E);    // Icon Red
    SetColor( 4, 0x0E0);    // Cursor                - This is set to black during boot, revert it back (Not really necessary since starting a terminal will init this itself...)
    SetColor( 6, 0xEEE);    // Icon Normal
    SetColor( 7, 0x0C0);    // Icon Green (Previously in slot 3)
    SetColor(18, 0xEEE);    // Window text FG Normal - This is set to black during boot, revert it back
    UI_ApplyTheme();

    // Set VBlank IRQ callback - Do not set it earlier in boot process!
    SYS_setVBlankCallback(VBlank);
    
    //ChangeState(PS_Debug, 0, NULL);
    //ChangeState(PS_Telnet, 0, NULL);
    //ChangeState(PS_IRC, 0, NULL);
    //ChangeState(PS_Gopher, 0, NULL);
    ChangeState(PS_Terminal, 0, NULL);

    while(TRUE)
    {
        StateTick();
        SYS_doVBlankProcess();
    }

    return 0;
}
