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
        KB_PS2_SendCommand(0xEE);
        waitMs(1);
        KB_PS2_Poll(&ret);
        #endif

        if ((ret == 0xFE) || (ret == 0xEE)) // FE = Fail+Resend, EE = Successfull echo back
        {
            sprintf(FStringTemp, "Found PS/2 KB @ slot %u:%u (r=$%X)", DEV_FULL(DEV_KBPS2), ret);
            TRM_DrawText(FStringTemp, 1, BootNextLine++, PAL1);

            UnsetDevCtrl(DEV_KBPS2);
            UnsetDevData(DEV_KBPS2);

            KB_SetKeyboard(&KB_PS2_Poll);

            return 1;
        }
    }

    // Writing 0xf0 followed by 1, 2 or 3 to port 0x60 will put the keyboard in scancode mode 1, 2 or 3.
    // Writing 0xf0 followed by 0 queries the mode, resulting in a scancode byte 43, 41 or 3f from the keyboard.

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

u8 KB_PS2_Poll(u8 *data)
{
    u32 timeout = 0;
    u16 stream_buffer = 0;

    KB_Unlock();

    while (GetDevData(DEV_KBPS2, 0x1))
    {
        if (timeout++ >= 128)   // 3200 32000
        {
            KB_Lock();
            return 0;
        }
    }

    for (u8 b = 0; b < 11; b++)  // Recieve byte
    {
        timeout = 0;
        while (GetDevData(DEV_KBPS2, 0x1)){if (timeout++ >= 128)goto timedout;}  // Wait for clock to go low

        stream_buffer |= (GetDevData(DEV_KBPS2, 0x2) >> KB_DT) << b;

        timeout = 0;
        while (!GetDevData(DEV_KBPS2, 0x1)){if (timeout++ >= 128)goto timedout;} // Wait for clock to go high
    }

    timedout:

    KB_Lock();

    if ((stream_buffer & 0x400) && ((stream_buffer & 1) == 0)) 
    {
        *data = ((stream_buffer & 0x1FE) >> 1);
        return 0xFF;
    }
    else 
    {
        *data = 0; // Tx fail - ask kb to resend
        return 0;
    }
}

//https://www.burtonsys.com/ps2_chapweske.htm
void KB_PS2_SendCommand(u8 cmd)
{
    u8 p, c, b = 7;
    u8 bc[11];

    // bits: xxxxx0dd ddddddp1 - where d= data, p= parity
    bc[0] = 0;  // Start
    for (u8 i = 1; i < 9; i++)
    {
        c = (cmd >> b) & 1;
        if (c) p++;

        bc[i] = c << KB_DT;
        b--;
    }

    bc[9] = ((p % 2) == 0 ? 1<<KB_DT:0);  // Parity
    bc[10] = 1 << KB_DT;  // Stop

    //kprintf("<%u> %u %u %u %u %u %u %u %u <%u> <%u>", bc[0], bc[1], bc[2], bc[3], bc[4], bc[5], bc[6], bc[7], bc[8], bc[9], bc[10]);
    //return;

    /*
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

    u16 timeout = 0;
    
    UnsetDevCtrl(DEV_KBPS2);    // (1) Set data(2) and clock(1) as input
    SetDevCtrl(DEV_KBPS2, 0x1); // (1) Set clock as output
    UnsetDevData(DEV_KBPS2);    // (1) Hold clock to low for at least 100 microseconds
    // wait here for 100 µS
    waitMs(1);
    OrDevCtrl(DEV_KBPS2, 0x3);  // (2) Set data(2) and clock(1) as output
    UnsetDevData(DEV_KBPS2);    // (2) Set data(2) and clock(1) low
    AndDevCtrl(DEV_KBPS2, 0x2); // (3) Release clock line (data output - clock input)

    for (u8 b = 0; b < 11; b++)  // Recieve byte
    {
        timeout = 0;
        while (GetDevData(DEV_KBPS2, 0x1)){if (timeout++ >= 128)goto timedout;}  // (4) Wait for clock to go low

        UnsetDevData(DEV_KBPS2);
        OrDevData(DEV_KBPS2, bc[b]);

        while (!GetDevData(DEV_KBPS2, 0x1)){if (timeout++ >= 128)goto timedout;} // (6) Wait for clock to go high
    }

    UnsetDevCtrl(DEV_KBPS2);    // (9) Set data(2) and clock(1) as input
    
    timeout = 0;

    // Ack
    while (GetDevData(DEV_KBPS2, 0x2)){if (timeout++ >= 128)goto timedout;} // (10) Wait for data to go low
    while (GetDevData(DEV_KBPS2, 0x1)){if (timeout++ >= 128)goto timedout;} // (11) Wait for clock to go low

    // Release
    //while (!GetDevData(DEV_KBPS2, 0x2)){if (timeout++ >= 128)goto timedout;} // (10) Wait for data to go high
    //while (!GetDevData(DEV_KBPS2, 0x1)){if (timeout++ >= 128)goto timedout;} // (11) Wait for clock to go high

    //waitMs(1);
    timedout:
    OrDevCtrl(DEV_KBPS2, 0x3); // Set pin 0 and 1 as output (smd->kb)
    UnsetDevData(DEV_KBPS2);
    OrDevData(DEV_KBPS2, 0x2); // Set clock low, data high - Stop kb sending data

    // Call KB_Poll() after this to recieve response/data
}
