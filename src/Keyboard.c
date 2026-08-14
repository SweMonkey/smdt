#include "Keyboard.h"
#include "devices/Keyboard_PS2.h"
#include "devices/Keyboard_Saturn.h"
#include "StateCtrl.h"  // bWindowActive
#include "Terminal.h"
#include "Input.h"
#include "Network.h"
#include "system/PseudoFile.h"
#include "system/StatusBar.h"
#include "WinMgr.h"
#include "Utils.h"

u8 sv_KeyLayout = 0;
u8 vKB_BATStatus = 0;
u8 bKB_ExtKey   = FALSE;
u8 bKB_Break    = FALSE;
u8 bKB_Shift    = FALSE;
u8 bKB_Alt      = FALSE;
u8 bKB_Ctrl     = FALSE;
u8 bKB_CapsLock = FALSE;
u8 bKB_NumLock  = FALSE;
u8 bKB_ScrLock  = FALSE;
KB_Poll_CB *PollCB = NULL;
KB_SetLED_CB *SetLEDCB = NULL;

extern bool bReverseColour;

// https://www.win.tue.nl/~aeb/linux/kbd/scancodes-10.html#scancodesets
// Using set 2

// US Layout
const u8 SCTable_US[3][128] =
{
{   // Lower
//  x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xA   xB   xC   xD   xE   xF     
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '`', 0x0, // 0x00 - 0x0F
    0x0, 0x0, 0x0, 0x0, 0x0, 'q', '1', 0x0, 0x0, 0x0, 'z', 's', 'a', 'w', '2', 0x0, // 0x10 - 0x1F
    0x0, 'c', 'x', 'd', 'e', '4', '3', 0x0, 0x0, ' ', 'v', 'f', 't', 'r', '5', 0x0, // 0x20 - 0x2F
    0x0, 'n', 'b', 'h', 'g', 'y', '6', 0x0, 0x0, 0x0, 'm', 'j', 'u', '7', '8', 0x0, // 0x30 - 0x3F
    0x0, ',', 'k', 'i', 'o', '0', '9', 0x0, 0x0, '.', '/', 'l', ';', 'p', '-', 0x0, // 0x40 - 0x4F
    0x0, 0x0,'\'', 0x0, '[', '=', 0x0, 0x0, 0x0, 0x0, 0x0, ']', 0x0,'\\', 0x0, 0x0, // 0x50 - 0x5F
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '1', 0x0, '4', '7', 0x0, 0x0, 0x0, // 0x60 - 0x6F - Numpad
    '0', ',', '2', '5', '6', '8', 0x0, 0x0, 0x0, '+', '3', '-', '*', '9', 0x0, 0x0, // 0x70 - 0x7F - Numpad
},
{   // Shift+<KEY>
//  x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xA   xB   xC   xD   xE   xF     
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '~', 0x0, // 0x00 - 0x0F
    0x0, 0x0, 0x0, 0x0, 0x0, 'Q', '!', 0x0, 0x0, 0x0, 'Z', 'S', 'A', 'W', '@', 0x0, // 0x10 - 0x1F
    0x0, 'C', 'X', 'D', 'E', '$', '#', 0x0, 0x0, ' ', 'V', 'F', 'T', 'R', '%', 0x0, // 0x20 - 0x2F
    0x0, 'N', 'B', 'H', 'G', 'Y', '^', 0x0, 0x0, 0x0, 'M', 'J', 'U', '&', '*', 0x0, // 0x30 - 0x3F
    0x0, '<', 'K', 'I', 'O', ')', '(', 0x0, 0x0, '>', '?', 'L', ':', 'P', '_', 0x0, // 0x40 - 0x4F
    0x0, 0x0, '"', 0x0, '{', '+', 0x0, 0x0, 0x0, 0x0, 0x0, '}', 0x0, '|', 0x0, 0x0, // 0x50 - 0x5F
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '1', 0x0, 0x0, '7', 0x0, 0x0, 0x0, // 0x60 - 0x6F - Numpad without 2468 directional
    '0', 0x0, 0x0, '5', 0x0, 0x0, 0x0, 0x0, 0x0, '+', '3', '-', '*', '9', 0x0, 0x0, // 0x70 - 0x7F - Numpad without 2468 directional
},
{   // ALT+<KEY>
//  x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xA   xB   xC   xD   xE   xF     
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // 0x00 - 0x0F
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // 0x10 - 0x1F
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, ' ', 0x0, 0x0, 0x0, 0x0,0xEE, 0x0, // 0x20 - 0x2F - 0x2E = €
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // 0x30 - 0x3F
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // 0x40 - 0x4F
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // 0x50 - 0x5F
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '1', 0x0, 0x0, '7', 0x0, 0x0, 0x0, // 0x60 - 0x6F - Numpad without 2468 directional
    '0', 0x0, 0x0, '5', 0x0, 0x0, 0x0, 0x0, 0x0, '+', '3', '-', '*', '9', 0x0, 0x0, // 0x70 - 0x7F - Numpad without 2468 directional
}};

// Swedish Layout
const u8 SCTable_SV[3][128] =
{
{   // Lower
//  x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xA   xB   xC   xD   xE   xF     
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x15, 0x0, // 0x00 - 0x0F
    0x0, 0x0, 0x0, 0x0, 0x0, 'q', '1', 0x0, 0x0, 0x0, 'z', 's', 'a', 'w', '2', 0x0, // 0x10 - 0x1F
    0x0, 'c', 'x', 'd', 'e', '4', '3', 0x0, 0x0, ' ', 'v', 'f', 't', 'r', '5', 0x0, // 0x20 - 0x2F
    0x0, 'n', 'b', 'h', 'g', 'y', '6', 0x0, 0x0, 0x0, 'm', 'j', 'u', '7', '8', 0x0, // 0x30 - 0x3F
    0x0, ',', 'k', 'i', 'o', '0', '9', 0x0, 0x0, '.', '-', 'l',0x94, 'p', '+', 0x0, // 0x40 - 0x4F - 0x4A is used for both '-' and numpad '/' (numpad one using extended $E0)
    0x0, 0x0,0x84, 0x0,0x86,0x60, 0x0, 0x0, 0x0, 0x0, 0x0, '"', 0x0,'\'', 0x0, 0x0, // 0x50 - 0x5F - 0x55 may be the wrong ' (´) - 0x5B is wrong " (should be ¨ but it is not in ascii)
    0x0, '<', 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '1', 0x0, '4', '7', 0x0, 0x0, 0x0, // 0x60 - 0x6F - Numpad
    '0', ',', '2', '5', '6', '8', 0x0, 0x0, 0x0, '+', '3', '-', '*', '9', 0x0, 0x0, // 0x70 - 0x7F - Numpad
},
{   // Shift+<KEY>
//  x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xA   xB   xC   xD   xE   xF     
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0xAB, 0x0, // 0x00 - 0x0F - 0x0E=½
    0x0, 0x0, 0x0, 0x0, 0x0, 'Q', '!', 0x0, 0x0, 0x0, 'Z', 'S', 'A', 'W', '"', 0x0, // 0x10 - 0x1F
    0x0, 'C', 'X', 'D', 'E', 0x9, '#', 0x0, 0x0, ' ', 'V', 'F', 'T', 'R', '%', 0x0, // 0x20 - 0x2F - 0x25 = ¤ (Not in ascii) 
    0x0, 'N', 'B', 'H', 'G', 'Y', '&', 0x0, 0x0, 0x0, 'M', 'J', 'U', '/', '(', 0x0, // 0x30 - 0x3F  
    0x0, ';', 'K', 'I', 'O', '=', ')', 0x0, 0x0, ':', '_', 'L',0x99, 'P', '?', 0x0, // 0x40 - 0x4F - 0x4C = Ö
    0x0, 0x0,0x8E, 0x0,0x8F,0x27, 0x0, 0x0, 0x0, 0x0, 0x0, '^', 0x0, '*', 0x0, 0x0, // 0x50 - 0x5F - 0x55 may be the wrong ' (`)
    0x0, '>', 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '1', 0x0, 0x0, '7', 0x0, 0x0, 0x0, // 0x60 - 0x6F - Numpad without 2468 directional
    '0', 0x0, 0x0, '5', 0x0, 0x0, 0x0, 0x0, 0x0, '+', '3', '-', '*', '9', 0x0, 0x0, // 0x70 - 0x7F - Numpad without 2468 directional
},
{   // ALT+<KEY>
//  x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xA   xB   xC   xD   xE   xF     
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x14, 0x0, // 0x00 - 0x0F  - 0x0E=¶
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0xAE, 0x0, 0x0, 0x0, '@', 0x0, // 0x10 - 0x1F
    0x0, 0x0,0xAF, 0x0, 0x0, '$',0x9C, 0x0, 0x0, ' ', 0x0, 0x0, 0x0, 0x0,0xEE, 0x0, // 0x20 - 0x2F  - 0x2E = €
    0x0, 0x0, 0x0, 0x0, 0x0,0x1B,0x9D, 0x0, 0x0, 0x0,0xE6, 0x0,0x19, '{', '[', 0x0, // 0x30 - 0x3F
    0x0, 0x0, 0x0,0x1A, 0x0, '}', ']', 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0xE3,'\\', 0x0, // 0x40 - 0x4F
    0x0, 0x0, 0x0, 0x0, 0x0,0xF1, 0x0, 0x0, 0x0, 0x0, 0x0,0x7E, 0x0,0x60, 0x0, 0x0, // 0x50 - 0x5F
    0x0, '|', 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '1', 0x0, 0x0, '7', 0x0, 0x0, 0x0, // 0x60 - 0x6F - Numpad without 2468 directional
    '0', 0x0, 0x0, '5', 0x0, 0x0, 0x0, 0x0, 0x0, '+', '3', '-', '*', '9', 0x0, 0x0, // 0x70 - 0x7F - Numpad without 2468 directional
}};

u8 const * const SCTablePtr[2][3] =
{
    {SCTable_US[0], SCTable_US[1], SCTable_US[2]}, 
    {SCTable_SV[0], SCTable_SV[1], SCTable_SV[2]}
};


void KB_Init()
{
    bKB_ExtKey = FALSE;
    bKB_Break = FALSE;
    bKB_Shift = FALSE;
    bKB_Alt = FALSE;
    bKB_Ctrl = FALSE;
    bKB_CapsLock = FALSE;
    bKB_NumLock  = TRUE;
    bKB_ScrLock  = FALSE;
}

void KB_SetPoll_Func(KB_Poll_CB *cb)
{
    PollCB = cb;
}

void KB_SetLED_Func(KB_SetLED_CB *cb)
{
    SetLEDCB = cb;
}

bool KB_Poll(u8 *data)
{
    if (PollCB == NULL) return FALSE;

    return PollCB(data);
}

void KB_SetLED(u8 leds)
{
    if (SetLEDCB == NULL) return;

    SetLEDCB(leds);
    return;
}

void KB_Interpret_Scancode(u8 scancode)
{
    if (bKB_Break && scancode != 0xF0)
    {
        switch (scancode)
        {
            default:
                set_KeyPress(((bKB_ExtKey?0x100:0) | scancode), KEYSTATE_UP);
            break;

            case KEY_LSHIFT:
            case KEY_RSHIFT:
                bKB_Shift = FALSE;
                set_KeyPress(((bKB_ExtKey?0x100:0) | scancode), KEYSTATE_UP);
            break;
            case 0x11:  //KEY_RALT
                bKB_Alt = FALSE;
                set_KeyPress(((bKB_ExtKey?0x100:0) | scancode), KEYSTATE_UP);
            break;
            case KEY_LCONTROL:  // CTRL^ Sequence
                bKB_Ctrl = FALSE;
                set_KeyPress(((bKB_ExtKey?0x100:0) | scancode), KEYSTATE_UP);
            break;

            case KEY_KP_DECIMAL:
            case KEY_KP0_INS:
            case KEY_KP1_END:
            case KEY_KP2_DOWN:
            case KEY_KP3_PGDN:
            case KEY_KP4_LEFT:
            case KEY_KP5:
            case KEY_KP6_RIGHT:
            case KEY_KP7_HOME:
            case KEY_KP8_UP:
            case KEY_KP9_PGUP:
            {
                // If ExtKey is true then this is probably the arrow keys not the numpad keys.
                if (bKB_ExtKey) 
                {
                    set_KeyPress((0x100 | scancode), KEYSTATE_UP);
                    break;
                }

                // Only set the numpad keys if numlock is off
                // OR if numlock is on AND shift is pressed.
                // Do not type out the numbers.
                if (!bKB_NumLock || (bKB_NumLock && bKB_Shift))
                {
                    set_KeyPress(scancode, KEYSTATE_UP);
                }
                break;
            }
        }

        bKB_Break = FALSE;
        bKB_ExtKey = FALSE;
        return;
    }

    switch (scancode)
    {
        default:
            set_KeyPress(((bKB_ExtKey?0x100:0) | scancode), KEYSTATE_DOWN);
        break;

        case 0xAA:  // BAT OK
            vKB_BATStatus = 1;
        return;
        case 0xE0:
            bKB_ExtKey = TRUE;
        return;
        case 0xF0:
            bKB_Break = TRUE;
        return;
        case 0xFC: // BAT FAIL
            vKB_BATStatus = 2;
            SB_SetStatusIcon(ICO_ID_ERROR, ICO_POS_0);
        return;

        // These keys will not be down/up whenever a character will be printed
        // Temporarily buffer these keys locally...
        case KEY_LSHIFT:
        case KEY_RSHIFT:
            set_KeyPress(((bKB_ExtKey?0x100:0) | scancode), KEYSTATE_DOWN);
            bKB_Shift = 1;
        return;
        case 0x11:  //KEY_RALT
            set_KeyPress(((bKB_ExtKey?0x100:0) | scancode), KEYSTATE_DOWN);
            bKB_Alt = 1;
        return;
        case KEY_LCONTROL:  // CTRL^ Sequence
            set_KeyPress(((bKB_ExtKey?0x100:0) | scancode), KEYSTATE_DOWN);
            bKB_Ctrl = TRUE;
        return;

        case KEY_SCROLLOCK:
        {
            set_KeyPress(((bKB_ExtKey?0x100:0) | scancode), KEYSTATE_DOWN);
            bKB_ScrLock = !bKB_ScrLock;

            u8 l = (bKB_CapsLock << 2) | (bKB_NumLock << 1) | (bKB_ScrLock);
            KB_SetLED(l);
            break;
        }
        case KEY_NUMLOCK:
        {
            set_KeyPress(((bKB_ExtKey?0x100:0) | scancode), KEYSTATE_DOWN);
            bKB_NumLock = !bKB_NumLock;

            u8 l = (bKB_CapsLock << 2) | (bKB_NumLock << 1) | (bKB_ScrLock);
            KB_SetLED(l);
            break;
        }
        case KEY_CAPSLOCK:
        {
            set_KeyPress(((bKB_ExtKey?0x100:0) | scancode), KEYSTATE_DOWN);
            bKB_CapsLock = !bKB_CapsLock;

            u8 l = (bKB_CapsLock << 2) | (bKB_NumLock << 1) | (bKB_ScrLock);
            KB_SetLED(l);
            break;
        }

        case KEY_KP_DECIMAL:
        case KEY_KP0_INS:
        case KEY_KP1_END:
        case KEY_KP2_DOWN:
        case KEY_KP3_PGDN:
        case KEY_KP4_LEFT:
        case KEY_KP5:
        case KEY_KP6_RIGHT:
        case KEY_KP7_HOME:
        case KEY_KP8_UP:
        case KEY_KP9_PGUP:
        {
            // If ExtKey is true then this is probably the arrow keys not the numpad keys.
            if (bKB_ExtKey) 
            {
                set_KeyPress((0x100 | scancode), KEYSTATE_DOWN);
                break;
            }

            // Only set the numpad keys if numlock is off
            // OR if numlock is on AND shift is pressed.
            // Do not type out the numbers.
            if (!bKB_NumLock || (bKB_NumLock && bKB_Shift))
            {
                set_KeyPress(scancode, KEYSTATE_DOWN);
                return;
            }
            break;
        }
    }

    // Ctrl^ sequence
    if (bKB_Ctrl)
    {
        u8 key = SCTablePtr[sv_KeyLayout][0][scancode];

        NET_BufferChar(key & 0x1F); // Add control byte to TxBuffer
        //Buffer_Push(&StdinBuffer, key & 0x1F);

        if (!bNoEcho) 
        {
            //TTY_PrintChar('^');     // Print ^ to TTY if ECHO is false
            bReverseColour = !bReverseColour;
            TTY_PrintChar(key);
            bReverseColour = !bReverseColour;
            //Buffer_Push(&StdinBuffer, key);
        }
        
        return;
    }

    // Normal printing
    u8 mod = 0;
    if (bKB_Alt) mod = 2;
    else if (bKB_Shift || bKB_CapsLock) mod = 1;

    u8 key = SCTablePtr[sv_KeyLayout][mod][scancode];

    // Ugly hack to fix $4A and $E04A on Swedish layout. ($4A = '-' and $E04A = '/'
    // This part of the code can't see the $E0, thus the numpad forward slash gets printed as '-')
    // On English layout this is not a problem because both keys map to '/'
    if (scancode == KEY_FSLASH && bKB_ExtKey) key = '/';
    else if (bKB_ExtKey) return; // No other printable characters have extended make codes ($E0), no point in going further.

    if (isPrintable(key) && !bWindowActive)
    {
        NET_BufferChar(key);                // Send key to TxBuffer
        //Buffer_Push(&StdinBuffer, key);

        if (!bNoEcho) TTY_PrintChar(key);   // Only print characters if ECHO is false
    }
    else if (isPrintable(key) && (WinMgr_GetCurrentWinID() == W_FavView))
    {
        Buffer_Push_IRQ(&TxBuffer, key);
        //Buffer_Push(&StdinBuffer, key);
    }
}
