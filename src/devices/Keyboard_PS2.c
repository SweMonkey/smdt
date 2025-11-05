#include "Keyboard_PS2.h"
#include "Keyboard.h"
#include "Utils.h"      // TRM_
#include "system/PseudoFile.h"  // Temp, printf

#define KB_CL 0 // Control pin shift
#define KB_DT 1 // Data pin shift

#define TIMEOUT 224
#define TIMEOUT_SLOW 640

SM_Device DRV_KBPS2;

// Checkme: "if a keyboard is interrupted while sending the second byte of a two-byte break code, it will need to retransmit both bytes of that break code, not just the one that was interrupted"


/// @brief Find & initialize keyboard
/// @return TRUE if keyboard was found, FALSE if not
bool KB_PS2_Init(DevPort port)
{
    u8 ret = 0;
    u8 flush = 0;

    DRV_KBPS2.Id.sName = "PS/2 Keyboard";
    DRV_KBPS2.Id.Mode = DEVMODE_PARALLEL;
    SetDevicePort(&DRV_KBPS2, port);
    DRV_KBPS2.Id.Bitmask = 0x3;

    #ifdef EMU_BUILD
    return FALSE;
    #endif

    // Iterate through pins 1+2 and 3+4 in search for a keyboard
    for (u8 s = 0; s < 2; s++)
    {
        DRV_KBPS2.Id.Bitshift = s<<1;    // Pin 1+2 (0*2 = 0) - Pin 3+4 (1*2 = 2)

        while (KB_PS2_Poll(&flush));    // Attempt to flush the internal keyboard buffer
        
        ret = KB_PS2_SendCommand(0xEE); // Send echo command to keyboard

        // Did we receive an echo back from the keyboard?
        if ((ret == 0xFE) || (ret == 0xEE)) // FE = Fail+Resend, EE = Successfull echo back
        {
            printf(" [92mFound PS/2 keyboard @ %u:%u ($%X)[0m\n", DRV_KBPS2.PAssign, s, ret);

            KB_SetPoll_Func(&KB_PS2_Poll);
            KB_SetLED_Func(&KB_PS2_SetLED);

            return TRUE;
        }
    }

    return FALSE;
}

/// @brief Stop keyboard from transmitting data
static inline void KB_Lock()
{
    DEV_SetCtrl(DRV_KBPS2, 0x3);    // Set pin 0 and 1 as output (smd->kb)
    DEV_SetData(DRV_KBPS2, 0x2);    // Set clock low, data high - Stop kb sending data
}

/// @brief Allow keyboard to transmit data
static inline void KB_Unlock()
{
    DEV_SetData(DRV_KBPS2, 0x3);// Set clock high, data high - Allow kb to send data
    DEV_ClrCtrl(DRV_KBPS2);     // Set pin 0 and 1 as input (kb->smd)
}

/// @brief Wait for transmit clock cycle
/// @return TRUE if clock was NOT cycled HIGH->LOW within a time limit <TIMEOUT>. FALSE on Successfull HIGH->LOW cycle
static inline u8 KB_WaitClockLow()
{
    u16 timeout_h = 0;
    u16 timeout_l = 0;

    //  PAL/50hz or MD1? works fine with timeout >= 128
    // NTSC/60hz or MD2? requires at least timeout >= 224, 192 may result in some dropped keys according to b1tsh1ft3r (hard to tell)
    while (!DEV_GetData(DRV_KBPS2, 0x1)){if (timeout_h++ >= TIMEOUT) return 1;}    // Wait for clock to go high
    while ( DEV_GetData(DRV_KBPS2, 0x1)){if (timeout_l++ >= TIMEOUT) return 1;}    // Wait for clock to go low

    return 0;
}

/// @brief Wait for quite some time for transmit clock cycle (Not for hot code!)
/// @return TRUE if clock was NOT cycled HIGH->LOW within a time limit <TIMEOUT_SLOW>. FALSE on Successfull HIGH->LOW cycle
static inline u8 KB_WaitLong_ClockLow()
{
    u16 timeout_h = 0;
    u16 timeout_l = 0;

    while (!DEV_GetData(DRV_KBPS2, 0x1)){if (timeout_h++ >= TIMEOUT_SLOW) return 1;}    // Wait for clock to go high
    while ( DEV_GetData(DRV_KBPS2, 0x1)){if (timeout_l++ >= TIMEOUT_SLOW) return 1;}    // Wait for clock to go low

    return 0;
}

/// @brief Check and read a byte from the keyboard if there is a byte available
/// @param r Pointer to a byte which will receive the value sent from the keyboard
/// @return TRUE if a valid byte was received, FALSE if not
bool KB_PS2_Poll(u8 *r)
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
        return TRUE;
    }

    Error:
    KB_Lock();
    *r = 0;
    return FALSE;
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

    // Send bit 0 - Need slow clock wait for the first bit...
    bit = (cmd & 1) << KB_DT;
    if (KB_WaitLong_ClockLow()){goto Error;}
    DEV_SetData(DRV_KBPS2, bit);

    // Send remaining 7 data bits (5-7)
    for (u8 b = 1; b < 8; b++)
    {
        // Prepare bit to send
        bit = ((cmd >> (b)) & 1) << KB_DT;

        if (KB_WaitLong_ClockLow()) goto Error;

        // Send bits in reverse order (Least significant bit first)
        DEV_SetData(DRV_KBPS2, bit);
    }

    // Wait for clock to cycle once
    if (KB_WaitLong_ClockLow()) goto Error;  

    // (8) Send parity bit
    DEV_SetData(DRV_KBPS2, parity); 

    // (9) Set data and clock as input
    DEV_ClrCtrl(DRV_KBPS2);

    // Ack
    if (KB_WaitLong_ClockLow()) goto Error;

    // Release
    if (KB_WaitLong_ClockLow()) goto Error;

    KB_Lock();

    // Get response from keyboard
    u8 ret = 0;
    waitMs(1);
    KB_PS2_Poll(&ret);  // This will also call KB_Lock() on exit
    return ret;

    Error:
    KB_Lock();
    return 0;
}

void KB_PS2_QueueCommand(u8 *bytes, u8 num)
{
    u8 i = 0;
    u8 r = 0;
    u8 tries = 0;

    while (i < num)
    {
        r = KB_PS2_SendCommand(bytes[i]);

        if (r == 0xFA)
        {
            i++;
            tries = 0;
        }
        else if (r == 0xFE)
        {
            tries++;
        }
        else break;

        if (tries > 10)
        {
            KB_PS2_SendCommand(0xEE);   // Dummy in case of kb being stuck waiting for a valid command
            break;
        }

        waitMs(5);
    }
}

void KB_PS2_SetLED(u8 leds)
{
    //u8 bytes[2] = {0xED, leds};
    //KB_PS2_QueueCommand(bytes, 2);
    return;
}

/*
https://www.burtonsys.com/ps2_chapweske.htm

SendCommand:

1)  Bring the Clock line low for at least 100 microseconds.
2)  Bring the Data line low.
3)  Release the Clock line.
4)  Wait for the device to bring the Clock line low.
5)  Set/reset the Data line to send the first data bit
6)  Wait for the device to bring Clock high.
7)  Wait for the device to bring Clock low.
8)  Repeat steps 5-7 for the other seven data bits and the parity bit
9)  Release the Data line.
10) Wait for the device to bring Data low.
11) Wait for the device to bring Clock  low.
12) Wait for the device to release Data and Clock
*/
