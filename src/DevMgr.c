// Note: This clusterfuck is a prototype and may be slightly rewritten in the future

#include "DevMgr.h"
#include "devices/Keyboard_PS2.h"   // KB_SendCommand() and KB_Poll()
#include "devices/RL_Network.h"
#include "Network.h"
#include "QMenu.h"                  // ChangeText() when KB is detected
#include "Utils.h"                  // Definitions
#include "Input.h"                  // Input_JP

SM_Device DEV_Joypad;               // Joypad device
SM_Device *DevList[DEV_MAX];        // Device list
u8 DevSeq = 0;                      // Number of devices
bool bRLNetwork = FALSE;            // Use RetroLink cartridge instead of built-in UART
DevPort DEV_UART_PORT = DP_Port2;   // Default UART port - Read only!


void SetDevicePort(SM_Device *d, DevPort p)
{
    switch (p)
    {
        case DP_Port1:
            d->Ctrl   = (vu8*)PORT1_CTRL;
            d->Data   = (vu8*)PORT1_DATA;
            d->SCtrl  = (vu8*)PORT1_SCTRL;
            d->RxData = (vu8*)PORT1_SRx;
            d->TxData = (vu8*)PORT1_STx;
        break;

        case DP_Port2:
            d->Ctrl   = (vu8*)PORT2_CTRL;
            d->Data   = (vu8*)PORT2_DATA;
            d->SCtrl  = (vu8*)PORT2_SCTRL;
            d->RxData = (vu8*)PORT2_SRx;
            d->TxData = (vu8*)PORT2_STx;
        break;

        case DP_Port3:
            d->Ctrl   = (vu8*)PORT3_CTRL;
            d->Data   = (vu8*)PORT3_DATA;
            d->SCtrl  = (vu8*)PORT3_SCTRL;
            d->RxData = (vu8*)PORT3_SRx;
            d->TxData = (vu8*)PORT3_STx;
        break;

        default:
        break;
    }

    d->PAssign = p;
}

void DetectDevices()
{
    char FStringTemp[32];
    u8 bNoKeyboard = FALSE;
    u8 ret = 0;

    // -- PS/2 Keyboard setup --------------------------
    DEV_KBPS2.Id.sName = "PS/2 KEYBOARD";
    DEV_KBPS2.Id.Mode = DEVMODE_PARALLEL;

    for (u8 s = 0; s < 6; s++)
    {
        switch (s)
        {
            case 0: // Pin 1 and 2 @ Port 1
                SetDevicePort(&DEV_KBPS2, DP_Port1);
                DEV_KBPS2.Id.Bitmask = 0x3;
                DEV_KBPS2.Id.Bitshift = 0;
            break;
            case 1: // Pin 3 and 4 @ Port 1
                SetDevicePort(&DEV_KBPS2, DP_Port1);
                DEV_KBPS2.Id.Bitmask = 0x3;
                DEV_KBPS2.Id.Bitshift = 2;
            break;

            case 2: // Pin 1 and 2 @ Port 2
                SetDevicePort(&DEV_KBPS2, DP_Port2);
                DEV_KBPS2.Id.Bitmask = 0x3;
                DEV_KBPS2.Id.Bitshift = 0;
            break;
            case 3: // Pin 3 and 4 @ Port 2
                SetDevicePort(&DEV_KBPS2, DP_Port2);
                DEV_KBPS2.Id.Bitmask = 0x3;
                DEV_KBPS2.Id.Bitshift = 2;
            break;

            case 4: // Pin 1 and 2 @ Port 3
                SetDevicePort(&DEV_KBPS2, DP_Port3);
                DEV_KBPS2.Id.Bitmask = 0x3;
                DEV_KBPS2.Id.Bitshift = 0;
            break;
            case 5: // Pin 3 and 4 @ Port 3
                SetDevicePort(&DEV_KBPS2, DP_Port3);
                DEV_KBPS2.Id.Bitmask = 0x3;
                DEV_KBPS2.Id.Bitshift = 2;
            break;
        
            default:
            break;
        }
        
        #ifndef EMU_BUILD
        KB_SendCommand(0xEE);
        waitMs(1);
        KB_Poll(&ret);
        #endif

        if ((ret == 0xFE) || (ret == 0xEE)) // FE = Fail+Resend, EE = Successfull echo back
        {
            sprintf(FStringTemp, "Found KB @ slot %u:%u (r=$%X)", DEV_FULL(DEV_KBPS2), ret);
            TRM_SetStatusIcon(ICO_KB_OK, ICO_POS_0);

            DevList[DevSeq++] = &DEV_KBPS2;
            
            UnsetDevCtrl(DEV_KBPS2);
            UnsetDevData(DEV_KBPS2);

            break;
        }
        else if (s >= 5)
        {
            sprintf(FStringTemp, "No KB found. (r=$%X)", ret);
            bNoKeyboard = TRUE;
        }
    }

    TRM_DrawText(FStringTemp, 1, BootNextLine++, PAL1);

    // -- Joypad setup --------------------------
    if (bNoKeyboard || (DEV_KBPS2.PAssign == DP_Port2))
    {
        DEV_Joypad.Id.sName = "JOYPAD";
        DEV_Joypad.Id.Bitmask = 0x40;
        DEV_Joypad.Id.Bitshift = 0;
        DEV_Joypad.Id.Mode = DEVMODE_PARALLEL;

        DevList[DevSeq++] = &DEV_Joypad;

        SetDevicePort(&DEV_Joypad, DP_Port1);        

        UnsetDevCtrl(DEV_Joypad);
        OrDevCtrl(DEV_Joypad, 0x40);
        UnsetDevData(DEV_Joypad);
        OrDevData(DEV_Joypad, 0x40);

        JOY_setSupport(PORT_1, JOY_SUPPORT_6BTN);
        JOY_setEventHandler(Input_JP);
        kprintf("No KB found - Press F1 to continue");

        if (bNoKeyboard) TRM_SetStatusIcon(ICO_JP_OK, ICO_POS_0);
    }

    // -- UART setup --------------------------
    DEV_UART.Id.sName = "UART DEVICE";
    DEV_UART.Id.Bitmask = 0;
    DEV_UART.Id.Bitshift = 0;
    DEV_UART.Id.Mode = DEVMODE_SERIAL;
    
    DevList[DevSeq++] = &DEV_UART;
    SetDevicePort(&DEV_UART, DEV_UART_PORT);
    vu8 *SCtrl;
    SCtrl = (vu8 *)DEV_UART.SCtrl;
    *SCtrl = 0x38;

    // RetroLink Network
    if (RLN_Initialize())
    {
        bRLNetwork = TRUE;

        VDP_setReg(0xB, 0);   // Disable VDP ext interrupt (Enable: 8 - Disable: 0)

        TRM_DrawText("RetroLink IP: ", 1, BootNextLine++, PAL1);
        RLN_PrintIP(1, BootNextLine++);
        TRM_DrawText("RetroLink MAC: ", 1, BootNextLine++, PAL1);
        RLN_PrintMAC(1, BootNextLine++);
    }
    else
    {
        bRLNetwork = FALSE;

        TRM_DrawText("RetroLink Network Adapter not found", 1, BootNextLine++, PAL1);
        TRM_DrawText("Defaulting to built in UART", 1, BootNextLine++, PAL1);
    }

}

void ConfigureDevices()
{
    for (u8 s = 0; s < DEV_MAX; s++)
    {
        DevList[s] = 0;
    }

    DevSeq = 0;

    DetectDevices();    // Detect and setup
}
