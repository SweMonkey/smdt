#include "MegaMouse.h"
#include "Utils.h"      // TRM_
#include "system/PseudoFile.h"

#define TIMEOUT 128
SM_Device DRV_MMOUSE;


bool MM_Mouse_Init()
{
    DRV_MMOUSE.Id.sName = "Mega Mouse";
    DRV_MMOUSE.Id.Bitmask = 0x7F;
    DRV_MMOUSE.Id.Bitshift = 0;
    DRV_MMOUSE.Id.Mode = DEVMODE_PARALLEL;

    DEV_SetCtrl(DRV_MMOUSE, 0x60);
    DEV_SetData(DRV_MMOUSE, 0x60);

    if (DEV_GetData(DRV_MMOUSE, 0xF) == 0)
    {
        Stdout_Push(" [92mMega Mouse initialized[0m\n");
        return TRUE;
    } 
    
    return FALSE;
}

bool MM_Mouse_Poll(s16 *delta_x, s16 *delta_y, u16 *buttons)
{
    u16 timeout = 0;
    u8 nibble[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

    for (u8 i = 0; i < 9; i += 2)
    {
        // Even nibble
        DEV_SetData(DRV_MMOUSE,  0x20);
        while (DEV_GetData(DRV_MMOUSE, 0x10) != 0x10)   // Wait for bit 4 to become 1
        {
            if (timeout++ >= TIMEOUT)
            {
                DEV_SetData(DRV_MMOUSE, 0x60);
                return FALSE;
            }
        }

        timeout = 0;
        nibble[i] = DEV_GetData(DRV_MMOUSE, 0xF);

        if (i == 8) break;  // Last iteration only has a single "even" nibble to read

        // Odd nibble
        DEV_ClrData(DRV_MMOUSE);
        while (DEV_GetData(DRV_MMOUSE, 0x10) != 0)  // Wait for bit 4 to become 0
        {
            if (timeout++ >= TIMEOUT)
            {
                DEV_SetData(DRV_MMOUSE, 0x60);
                return FALSE;
            }
        }

        timeout = 0;
        nibble[i+1] = DEV_GetData(DRV_MMOUSE, 0xF);
    }

    // End transmission
    DEV_SetData(DRV_MMOUSE, 0x60);

    // Check signature
    if (nibble[0] != 0xB) return FALSE;

    // Check overflow
    if (((nibble[3] & 8) == 8) || ((nibble[3] & 4) == 4)) return FALSE;

    // Set delta x/y
    *delta_x = (nibble[5] << 4) | nibble[6];
    *delta_y = (nibble[7] << 4) | nibble[8];
    
    // Sign extend if negative
    if ((*delta_x > 0) && (nibble[3] & 1)){ *delta_x |= 0xFF00; }
    if ((*delta_y > 0) && (nibble[3] & 2)){ *delta_y |= 0xFF00; }  

    *buttons = nibble[4];

    return TRUE;
}
