// Note: This clusterfuck is a prototype and may be slightly rewritten in the future

#include "DevMgr.h"
#include "devices/Keyboard_PS2.h"   // KB_SendCommand() and KB_Poll()
#include "devices/Keyboard_Saturn.h"
#include "Keyboard.h"
#include "devices/RL_Network.h"
#include "devices/XP_Network.h"
#include "Network.h"
#include "QMenu.h"                  // ChangeText() when KB is detected
#include "Utils.h"                  // Definitions
#include "Input.h"                  // Input_JP

/*
ID	        Peripheral
1111 ($0F)	(undetectable)
1101 ($0D)	Mega Drive controller
1100 ($0C)	Mega Drive controller (see note)
1011 ($0B)	Saturn controller
1010 ($0A)	Printer
0111 ($07)	Sega multitap
0101 ($05)	Other Saturn peripherals
0011 ($03)	Mouse
0001 ($01)	Justifier
0000 ($00)	Menacer 
*/

#define DEVICE_UNKNOWN 0xF
#define DEVICE_MD_CTRL0 0xD
#define DEVICE_MD_CTRL1 0xC
#define DEVICE_SATURN_CTRL 0xB
#define DEVICE_PRINTER 0xA
#define DEVICE_MULTITAP 0x7
#define DEVICE_SATURN_PERIPHERAL 0x5
#define DEVICE_MOUSE 0x3
#define DEVICE_JUSTIFIER 0x1
#define DEVICE_MENACER 0

SM_Device DEV_Joypad;               // Joypad device
SM_Device DEV_Detector;             // Detector device
SM_Device *DevList[DEV_MAX];        // Device list
u8 DevSeq = 0;                      // Number of devices
bool bRLNetwork = FALSE;            // Use RetroLink cartridge instead of built-in UART
bool bXPNetwork = FALSE;            // Use XPort network adapter
DevPort sv_ListenPort = DP_Port2;   // Default UART port to listen on

/// @brief Get four bit device identifier (Sega devices only)
/// @param p Port to check (DP_Port1, DP_Port2, DP_Port3)
/// @return Four bit device identifier
u8 GetDeviceID(DevPort p)
{
    u8 r = 0;
    u8 dH = 0;
    u8 dL = 0;

    // Setup dummy device to test ports with
    DEV_Detector.Id.Bitmask = 0x7F;
    DEV_Detector.Id.Bitshift = 0;
    DEV_Detector.Id.Mode = DEVMODE_PARALLEL;

    SetDevicePort(&DEV_Detector, p);

    // Get device ID
    UnsetDevCtrl(DEV_Detector);
    OrDevCtrl(DEV_Detector, 0x40);
    UnsetDevData(DEV_Detector);

    OrDevData(DEV_Detector, 0x40);
    dH = GetDevData(DEV_Detector, 0xF);

    UnsetDevData(DEV_Detector);
    dL = GetDevData(DEV_Detector, 0xF);

    r |= (((dH & 8) >> 3) | ((dH & 4) >> 2)) << 3;
    r |= (((dH & 2) >> 1) | ((dH & 1)     )) << 2;
    r |= (((dL & 8) >> 3) | ((dL & 4) >> 2)) << 1;
    r |= (((dL & 2) >> 1) | ((dL & 1)     ));

    //kprintf("dH: $%X - dL: $%X", dH, dL);
    //kprintf("Device ID: $%X", r);

    return r;
}

/// @brief Set device to desired port
/// @param d Device pointer
/// @param p Desired port
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

/// @brief Detect and initialize found devices
void DetectDevices()
{
    u8 bNoKeyboard = TRUE;
    u8 DevId0 = 0;
    u8 DevId1 = 0;
    u8 DevId2 = 0;

    DevId0 = GetDeviceID(DP_Port1);
    DevId1 = GetDeviceID(DP_Port2);
    DevId2 = GetDeviceID(DP_Port3);

    // -- PS/2 Keyboard setup --------------------------
    if (KB_PS2_Init())
    {
        DevList[DevSeq++] = &DEV_KBPS2;
        TRM_SetStatusIcon(ICO_KB_OK, ICO_POS_0);
        bNoKeyboard = FALSE;
    }
    else
    {
        bNoKeyboard = TRUE;
    }

    // -- Saturn Keyboard setup ------------------------
    if (bNoKeyboard && ((DevId0 == DEVICE_SATURN_PERIPHERAL) || (DevId1 == DEVICE_SATURN_PERIPHERAL) || (DevId2 == DEVICE_SATURN_PERIPHERAL)))
    {
             if (DevId0 == DEVICE_SATURN_PERIPHERAL) SetDevicePort(&DEV_KBSATURN, DP_Port1);
        else if (DevId1 == DEVICE_SATURN_PERIPHERAL) SetDevicePort(&DEV_KBSATURN, DP_Port2);
        else if (DevId2 == DEVICE_SATURN_PERIPHERAL) SetDevicePort(&DEV_KBSATURN, DP_Port3);

        if (KB_Saturn_Init())
        {
            DevList[DevSeq++] = &DEV_KBSATURN;
            TRM_SetStatusIcon(ICO_KB_OK, ICO_POS_0);
            bNoKeyboard = FALSE;
        }
        else
        {
            bNoKeyboard = TRUE;
        }
    }

    if (bNoKeyboard)
    {
        TRM_DrawText("No keyboard found.", 1, BootNextLine++, PAL1);
        kprintf("No KB found - Press F1 to continue");
    }

    // -- Joypad setup ---------------------------------
    if (bNoKeyboard || ((DEV_KBPS2.PAssign != DP_Port1) || (DEV_KBSATURN.PAssign != DP_Port1))) // Only enable joypad if there is no keyboard detected, or if port 1 is free
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

        if (bNoKeyboard && (vKB_BATStatus == 0))
        {
            TRM_SetStatusIcon(ICO_JP_OK, ICO_POS_0);
        }
    }

    // -- UART setup --------------------------
    DEV_UART.Id.sName = "UART DEVICE";
    DEV_UART.Id.Bitmask = 0x20; // TH pin
    DEV_UART.Id.Bitshift = 0;
    DEV_UART.Id.Mode = DEVMODE_SERIAL | DEVMODE_PARALLEL;
    
    DevList[DevSeq++] = &DEV_UART;
    SetDevicePort(&DEV_UART, sv_ListenPort);
    vu8 *SCtrl;
    SCtrl = (vu8 *)DEV_UART.SCtrl;
    *SCtrl = 0x38;

    u8 xpn_r = XPN_Initialize();

    switch (xpn_r)
    {
        /*case 0:
            TRM_DrawText("XPN: Device not found", 1, BootNextLine++, PAL1);
        break;*/
        case 1:
            TRM_DrawText("XPN: xPico module OK", 1, BootNextLine++, PAL1);
        break;
        case 2:
            TRM_DrawText("XPN: Error", 1, BootNextLine++, PAL1);
        break;
    
        default:
        break;
    }

    // RetroLink Network
    if (RLN_Initialize())
    {
        bRLNetwork = TRUE;

        VDP_setReg(0xB, 0);   // Disable VDP ext interrupt (Enable: 8 - Disable: 0)

        NET_SetConnectFunc(RLN_Connect);

        TRM_DrawText("RetroLink IP: ", 1, BootNextLine++, PAL1);
        RLN_PrintIP(1, BootNextLine++);
        TRM_DrawText("RetroLink MAC: ", 1, BootNextLine++, PAL1);
        RLN_PrintMAC(1, BootNextLine++);
    }
    else if (xpn_r)
    {
        DEV_UART.Id.sName = "XPICO UART";

        SetDevCtrl(DEV_UART, 0x20);
        UnsetDevData(DEV_UART);

        bXPNetwork = TRUE;

        NET_SetConnectFunc(XPN_Connect);

        //TRM_DrawText("xPico IP: <not implemented>", 1, BootNextLine++, PAL1);
        //XPN_PrintIP(1, BootNextLine++);
        //TRM_DrawText("xPico MAC: <not implemented>", 1, BootNextLine++, PAL1);
        //XPN_PrintMAC(1, BootNextLine++);
    }
    else
    {
        bRLNetwork = FALSE;
        bXPNetwork = FALSE;

        //TRM_DrawText("RetroLink Network Adapter not found", 1, BootNextLine++, PAL1);
        TRM_DrawText("No network adapters found", 1, BootNextLine++, PAL1);
        TRM_DrawText("Listening on built in UART", 1, BootNextLine++, PAL1);
    }

}

/// @brief Initialize device manager and find/init devices
void DeviceManager_Init()
{
    for (u8 s = 0; s < DEV_MAX; s++)
    {
        DevList[s] = 0;
    }

    DevSeq = 0;
    
    bRLNetwork = FALSE;
    bXPNetwork = FALSE;

    DetectDevices();    // Detect and setup
}
