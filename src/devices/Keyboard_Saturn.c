#include "Keyboard_Saturn.h"
#include "Keyboard.h"
#include "Utils.h"      // TRM_

#define TIMEOUT 128
SM_Device DRV_KBSATURN;


bool KB_Saturn_Init()
{
    DRV_KBSATURN.Id.sName = "Saturn KB";
    DRV_KBSATURN.Id.Bitmask = 0x7F;
    DRV_KBSATURN.Id.Bitshift = 0;
    DRV_KBSATURN.Id.Mode = DEVMODE_PARALLEL;

    DEV_SetCtrl(DRV_KBSATURN, 0x60);
    DEV_SetData(DRV_KBSATURN, 0x60);    // Write $20 to data port, if read back of bits 3-0 is $1 then the device is a keyboard

    if (DEV_GetData(DRV_KBSATURN, 0xF) != 1)
    {
        kprintf("Unknown Saturn peripheral found (r = $%X)", DEV_GetData(DRV_KBSATURN, 0xF));

        return 0;
    }
    else
    {        
        DEV_SetData(DRV_KBSATURN, 0x60);

        KB_SetKeyboard(&KB_Saturn_Poll);

        TRM_DrawText("Saturn KB found.", 1, BootNextLine++, PAL1);
    }

    return 1;
}

u8 KB_Saturn_Poll(u8 *data)
{
    u32 timeout = 0;
    u8 nibble[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    /*
    1. Write $00 to the data port
    2. Wait until bit 4 becomes 0
    3. Read bits 3-0 to get a nibble
    4. Write $20 to the data port
    5. Wait until bit 4 becomes 1
    6. Read bits 3-0 to get a nibble 
    */

    for (u8 i = 0; i < 12; i += 2)
    {
        // Even nibble
        DEV_ClrData(DRV_KBSATURN);
        while ((DEV_GetData(DRV_KBSATURN, 0x10) & 0x10) != 0)
        {
            if (timeout++ >= TIMEOUT) return 0;
        }

        timeout = 0;
        nibble[i] = DEV_GetData(DRV_KBSATURN, 0xF);

        // Odd nibble
        DEV_SetData(DRV_KBSATURN,  0x20);
        
        while (DEV_GetData(DRV_KBSATURN, 0x10) != 0x10)
        {
            if (timeout++ >= TIMEOUT) return 0;
        }

        timeout = 0;
        nibble[i+1] = DEV_GetData(DRV_KBSATURN, 0xF);
    }

    // End transmission
    DEV_SetData(DRV_KBSATURN, 0x60);

    bKB_Break = nibble[7] & 1;
    *data = (nibble[8] << 4) | nibble[9];

    // Translate "irregular" scancodes into "normal" extended scancodes
    switch (*data)
    {
        case 0x17:  // RAlt
            bKB_ExtKey = TRUE;
            *data = 0x11;
        break;

        case 0x18:  // RCtrl
            bKB_ExtKey = TRUE;
            *data = 0x14;
        break;

        case 0x19:  // KP Enter
            bKB_ExtKey = TRUE;
            *data = 0x5A;
        break;
        
        case 0x1F:  // LWin
            bKB_ExtKey = TRUE;
        break;

        case 0x27:  // RWin
            bKB_ExtKey = TRUE;
        break;

        case 0x2F:  // Menu
            bKB_ExtKey = TRUE;
        break;

        case 0x80:  // KP /
            bKB_ExtKey = TRUE;
            *data = 0x4A;
        break;

        case 0x81:  // Insert
            bKB_ExtKey = TRUE;
            *data = 0x70;
        break;

        case 0x84:  // PrtScr
            bKB_ExtKey = TRUE;
            *data = 0x7C;
        break;

        case 0x85:  // Delete
            bKB_ExtKey = TRUE;
            *data = 0x71;
        break;

        case 0x86:  // Left arrow
            bKB_ExtKey = TRUE;
            *data = 0x6B;
        break;

        case 0x87:  // Home
            bKB_ExtKey = TRUE;
            *data = 0x6C;
        break;

        case 0x88:  // End
            bKB_ExtKey = TRUE;
            *data = 0x69;
        break;

        case 0x89:  // Up arrow
            bKB_ExtKey = TRUE;
            *data = 0x75;
        break;

        case 0x8A:  // Down arrow
            bKB_ExtKey = TRUE;
            *data = 0x72;
        break;

        case 0x8B:  // PgUp
            bKB_ExtKey = TRUE;
            *data = 0x7D;
        break;

        case 0x8C:  // PgDown
            bKB_ExtKey = TRUE;
            *data = 0x7A;
        break;

        case 0x8D:  // Right arrow
            bKB_ExtKey = TRUE;
            *data = 0x74;
        break;
        
        //case 0x:  // Ctrl+Break
        //case 0x:  // Sleep
        //case 0x:  // Power
        //case 0x:  // Wake
    
        default:
        break;
    }

    if (*data == 0) return 0;

    return 0xFF;
}
