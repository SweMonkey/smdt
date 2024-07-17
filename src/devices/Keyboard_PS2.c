#include "Keyboard_PS2.h"
#include "Keyboard.h"
#include "Utils.h"      // TRM_

#define KB_CL 0 // Control pin shift
#define KB_DT 1 // Data pin shift

SM_Device DRV_KBPS2;


/// @brief Find & initialize keyboard
/// @return TRUE if keyboard was found, FALSE if not
bool KB_PS2_Init()
{
    u8 ret = 0;
    char FStringTemp[32];

    DRV_KBPS2.Id.sName = "PS/2 Keyboard";
    DRV_KBPS2.Id.Mode = DEVMODE_PARALLEL;

    // Iterate through all ports/pins in search for a keyboard. First found keyboard will be used
    for (u8 s = 0; s < 6; s++)
    {
        switch (s)
        {
            case 0: // Pin 1 and 2 @ Port 1
                SetDevicePort(&DRV_KBPS2, DP_Port1);
                DRV_KBPS2.Id.Bitmask = 0x3;
                DRV_KBPS2.Id.Bitshift = 0;
            break;
            case 1: // Pin 3 and 4 @ Port 1
                SetDevicePort(&DRV_KBPS2, DP_Port1);
                DRV_KBPS2.Id.Bitmask = 0x3;
                DRV_KBPS2.Id.Bitshift = 2;
            break;

            case 2: // Pin 1 and 2 @ Port 2
                SetDevicePort(&DRV_KBPS2, DP_Port2);
                DRV_KBPS2.Id.Bitmask = 0x3;
                DRV_KBPS2.Id.Bitshift = 0;
            break;
            case 3: // Pin 3 and 4 @ Port 2
                SetDevicePort(&DRV_KBPS2, DP_Port2);
                DRV_KBPS2.Id.Bitmask = 0x3;
                DRV_KBPS2.Id.Bitshift = 2;
            break;

            case 4: // Pin 1 and 2 @ Port 3
                SetDevicePort(&DRV_KBPS2, DP_Port3);
                DRV_KBPS2.Id.Bitmask = 0x3;
                DRV_KBPS2.Id.Bitshift = 0;
            break;
            case 5: // Pin 3 and 4 @ Port 3
                SetDevicePort(&DRV_KBPS2, DP_Port3);
                DRV_KBPS2.Id.Bitmask = 0x3;
                DRV_KBPS2.Id.Bitshift = 2;
            break;
        
            default:
            break;
        }
        
        #ifndef EMU_BUILD        
        ret = KB_PS2_SendCommand(0xEE); // Send echo command to keyboard
        #endif

        // Did we receive an echo back from the keyboard?
        if ((ret == 0xFE) || (ret == 0xEE)) // FE = Fail+Resend, EE = Successfull echo back
        {
            sprintf(FStringTemp, "Found PS/2 KB @ slot %u:%u (r=$%X)", DEV_FULL(DRV_KBPS2), ret);
            TRM_DrawText(FStringTemp, 1, BootNextLine++, PAL1);

            KB_SetKeyboard(&KB_PS2_Poll);

            return TRUE;
        }
    }

    return FALSE;
}

/// @brief Stop keyboard from transmitting data
inline void KB_Lock()
{
    DEV_SetCtrl(DRV_KBPS2, 0x3);    // Set pin 0 and 1 as output (smd->kb)
    DEV_SetData(DRV_KBPS2, 0x2);    // Set clock low, data high - Stop kb sending data
}

/// @brief Allow keyboard to transmit data
inline void KB_Unlock()
{
    DEV_SetData(DRV_KBPS2, 0x3);// Set clock high, data high - Allow kb to send data
    DEV_ClrCtrl(DRV_KBPS2);     // Set pin 0 and 1 as input (kb->smd)
}

/// @brief Wait for transmit clock cycle
/// @return TRUE if clock was NOT cycled HIGH->LOW within a time limit. FALSE on Successfull HIGH->LOW cycle
inline u8 KB_WaitClockLow()
{
    u16 timeout = 0;

    //  PAL/50hz or MD1? works fine with timeout >= 128
    // NTSC/60hz or MD2? requires at least timeout >= 224, 192 may result in some dropped keys according to b1tsh1ft3r (hard to tell)
    while (!DEV_GetData(DRV_KBPS2, 0x1)){if (timeout++ >= 224)goto timedout;}    // Wait for clock to go high
    while ( DEV_GetData(DRV_KBPS2, 0x1)){if (timeout++ >= 224)goto timedout;}    // Wait for clock to go low

    return 0;

    timedout:
    return 1;
}

/// @brief Check and read a byte from the keyboard if there is a byte available
/// @param r Pointer to a byte which will receive the value sent from the keyboard
/// @return TRUE if a valid byte was received, FALSE if not
u8 KB_PS2_Poll(u8 *r)
{
    u8 bit = 0;
    u8 data = 0;
    u8 parity = 0;

    KB_Unlock();

    // Start bit
    if (KB_WaitClockLow()) goto Error;

    for (u8 b = 0; b < 8; b++)  // Receive 8 data bits (bit 1-8)
    {
        if (KB_WaitClockLow()) goto Error;

        bit = DEV_GetData(DRV_KBPS2, 0x2) >> KB_DT;  // Read bit from keyboard

        data |= bit << b;   // Shift bit into data buffer
        parity += bit;
    }

    // Parity bit (bit 9)
    if (KB_WaitClockLow()) goto Error;
    
    parity += DEV_GetData(DRV_KBPS2, 0x2) >> KB_DT;  // Read parity bit

    // Stop bit (bit 10)
    if (KB_WaitClockLow()) goto Error;

    u8 stop = DEV_GetData(DRV_KBPS2, 0x2) >> KB_DT;  // Read stop bit

    // Check parity (Odd)
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

/// @brief Send a command byte to the keyboard and read the response
/// \todo Ability to send multi byte commands/receive multi byte responses
/// @param cmd Command to send to keyboard
/// @return Received response from keyboard
u8 KB_PS2_SendCommand(u8 cmd)
{
    u8 bit = 0;
    u8 parity = (cmd % 2) << KB_DT;

    DEV_SetCtrl(DRV_KBPS2, 0x1);    // (1) Set data as input and clock as output
    DEV_ClrData(DRV_KBPS2);         // (1) Hold clock low for at least 100 microseconds
    waitMs(1);
    DEV_SetCtrl(DRV_KBPS2, 0x3);    // (2) Set data and clock as output
    DEV_ClrData(DRV_KBPS2);         // (2) Set data and clock low
    DEV_SetCtrl(DRV_KBPS2, 0x2);    // (3) Release clock line (data output - clock input)

    // Send 8 data bits (steps 4 + 5-7)
    for (u8 b = 0; b < 8; b++)  
    {
        // Prepare bit to send
        bit = ((cmd >> (b)) & 1) << KB_DT;

        if (KB_WaitClockLow()) goto Error;

        // Send bits in reverse order (Least significant bit first)
        DEV_SetData(DRV_KBPS2, bit);
    }

    // Wait for clock to cycle once
    if (KB_WaitClockLow()) goto Error;  

    // (8) Send parity bit
    DEV_SetData(DRV_KBPS2, parity); 

    // (9) Set data and clock as input
    DEV_ClrCtrl(DRV_KBPS2);
    
    // Ack
    if (KB_WaitClockLow()) goto Error;

    // Release
    if (KB_WaitClockLow()) goto Error;

    // Get response from keyboard
    u8 ret = 0;
    KB_PS2_Poll(&ret);  // This will also call KB_Lock() on exit
    return ret;

    Error:
    KB_Lock();
    return 0;
}

/*
https://www.burtonsys.com/ps2_chapweske.htm

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
