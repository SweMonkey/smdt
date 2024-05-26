#include "Keyboard_PS2.h"
#include "Keyboard.h"
#include "StateCtrl.h"  // bWindowActive
#include "QMenu.h"      // ChangeText() when KB is detected
#include "Terminal.h"   // TTY_PrintChar
#include "Telnet.h"
#include "Input.h"
#include "Buffer.h"
#include "Utils.h"

#define KB_CL 0
#define KB_DT 1

SM_Device DEV_KBPS2;


bool KB_PS2_Init()
{
    u8 ret = 0;
    char FStringTemp[32];

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
        ret = KB_PS2_SendCommand(0xEE);
        #endif

        if ((ret == 0xFE) || (ret == 0xEE)) // FE = Fail+Resend, EE = Successfull echo back
        {
            sprintf(FStringTemp, "Found PS/2 KB @ slot %u:%u (r=$%X)", DEV_FULL(DEV_KBPS2), ret);
            TRM_DrawText(FStringTemp, 1, BootNextLine++, PAL1);

            KB_SetKeyboard(&KB_PS2_Poll);

            return 1;
        }
    }

    return 0;
}

inline void KB_Lock()
{
    SetDevCtrl(DEV_KBPS2, 0x3); // Set pin 0 and 1 as output (smd->kb)
    UnsetDevData(DEV_KBPS2);
    SetDevData(DEV_KBPS2, 0x2); // Set clock low, data high - Stop kb sending data
}

inline void KB_Unlock()
{
    UnsetDevData(DEV_KBPS2);    
    SetDevData(DEV_KBPS2, 0x3); // Set clock high, data high - Allow kb to send data
    UnsetDevCtrl(DEV_KBPS2);    // Set pin 0 and 1 as input (kb->smd)
}

inline u8 KB_PS2_WaitClockLow()
{
    u16 timeout = 0;

    //  PAL/50hz or MD1? works fine with timeout >= 128
    // NTSC/60hz or MD2? requires at least timeout >= 224, 192 may result in some dropped keys according to b1tsh1ft3r (hard to tell)
    while (!GetDevData(DEV_KBPS2, 0x1)){if (timeout++ >= 224)goto timedout;}    // Wait for clock to go high
    while ( GetDevData(DEV_KBPS2, 0x1)){if (timeout++ >= 224)goto timedout;}    // Wait for clock to go low

    return 0;

    timedout:
    return 1;
}

u8 KB_PS2_Poll(u8 *r)
{
    u8 bit = 0;
    u8 data = 0;
    u8 parity = 0;

    KB_Unlock();

    // Start bit
    if (KB_PS2_WaitClockLow()) goto Error;

    for (u8 b = 0; b < 8; b++)  // Recieve byte
    {
        if (KB_PS2_WaitClockLow()) goto Error;

        bit = GetDevData(DEV_KBPS2, 0x2) >> KB_DT;

        data |= bit << b;
        parity += bit;
    }

    // Parity bit
    if (KB_PS2_WaitClockLow()) goto Error;
    
    parity += GetDevData(DEV_KBPS2, 0x2) >> KB_DT;

    // Stop bit
    if (KB_PS2_WaitClockLow()) goto Error;

    u8 stop = GetDevData(DEV_KBPS2, 0x2) >> KB_DT;

    // Check parity
    if ((parity & 1) && (stop == 1))
    {
        KB_Lock();
        *r = data;
        return 0xFF;
    }

    Error:
    KB_Lock();
    *r = 0;
    return 0;
}

//https://www.burtonsys.com/ps2_chapweske.htm
// Todo: Send multi byte commands/receive multi byte responses
u8 KB_PS2_SendCommand(u8 cmd)
{
    u8 bit = 0;
    u8 parity = (cmd % 2) << KB_DT;

    UnsetDevCtrl(DEV_KBPS2);    // (1) Set data(2) and clock(1) as input
    SetDevCtrl(DEV_KBPS2, 0x1); // (1) Set clock as output
    UnsetDevData(DEV_KBPS2);    // (1) Hold clock to low for at least 100 microseconds
    waitMs(1);
    OrDevCtrl(DEV_KBPS2, 0x3);  // (2) Set data(2) and clock(1) as output
    UnsetDevData(DEV_KBPS2);    // (2) Set data(2) and clock(1) low
    AndDevCtrl(DEV_KBPS2, 0x2); // (3) Release clock line (data output - clock input)

    for (u8 b = 0; b < 8; b++)  // Send byte
    {
        if (KB_PS2_WaitClockLow()) goto Error;

        bit = ((cmd >> (b)) & 1) << KB_DT;
        UnsetDevData(DEV_KBPS2);
        OrDevData(DEV_KBPS2, bit);  // Send bits in reverse order (Least significant bit first)
    }

    if (KB_PS2_WaitClockLow()) goto Error;
    UnsetDevData(DEV_KBPS2);
    OrDevData(DEV_KBPS2, parity);    // Send parity bit

    UnsetDevCtrl(DEV_KBPS2);    // (9) Set data(2) and clock(1) as input
    
    // Ack
    if (KB_PS2_WaitClockLow()) goto Error;

    // Release
    if (KB_PS2_WaitClockLow()) goto Error;

    // Get response from keyboard
    u8 ret = 0;
    KB_PS2_Poll(&ret);  // This will also call KB_Lock() on exit
    return ret;

    Error:
    KB_Lock();
    return 0;
}

/*
SendCommand:

1)   Bring the Clock line low for at least 100 microseconds.
2)   Bring the Data line low.
3)   Release the Clock line.
4)   Wait for the device to bring the Clock line low.
5)   Set/reset the Data line to send the first data bit
6)   Wait for the device to bring Clock high.
7)   Wait for the device to bring Clock low.
8)   Repeat steps 5-7 for the other seven data bits and the parity bit
9)   Release the Data line.
10) Wait for the device to bring Data low.
11) Wait for the device to bring Clock  low.
12) Wait for the device to release Data and Clock
*/
