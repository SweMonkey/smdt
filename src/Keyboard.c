#include "Keyboard.h"
#include "devices/Keyboard_PS2.h"
#include "devices/Keyboard_Saturn.h"
#include "StateCtrl.h"  // bWindowActive
#include "Terminal.h"
#include "Input.h"
#include "Network.h"
#include "Utils.h"

u8 sv_KeyLayout = 0;
u8 vKB_BATStatus = 0;
u8 bKB_ExtKey = FALSE;
u8 bKB_Break = FALSE;
u8 bKB_Shift = FALSE;
u8 bKB_Alt = FALSE;
u8 bKB_Ctrl = FALSE;
KB_Poll_CB *PollCB = NULL;

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
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '1', 0x0, 0x0, '7', 0x0, 0x0, 0x0, // 0x60 - 0x6F - Numpad without 2468 directional
    '0', 0x0, 0x0, '5', 0x0, 0x0, 0x0, 0x0, 0x0, '+', '3', '-', '*', '9', 0x0, 0x0, // 0x70 - 0x7F - Numpad without 2468 directional
    //0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '1', 0x0, '4', '7', 0x0, 0x0, 0x0, // 0x60 - 0x6F - Numpad
    //'0', 0x0, '2', '5', '6', '8', 0x0, 0x0, 0x0, '+', '3', '-', '*', '9', 0x0, 0x0, // 0x70 - 0x7F - Numpad
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
    //0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '1', 0x0, '4', '7', 0x0, 0x0, 0x0, // 0x60 - 0x6F - Numpad
    //'0', 0x0, '2', '5', '6', '8', 0x0, 0x0, 0x0, '+', '3', '-', '*', '9', 0x0, 0x0, // 0x70 - 0x7F - Numpad
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
    //0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '1', 0x0, '4', '7', 0x0, 0x0, 0x0, // 0x60 - 0x6F - Numpad
    //'0', 0x0, '2', '5', '6', '8', 0x0, 0x0, 0x0, '+', '3', '-', '*', '9', 0x0, 0x0, // 0x70 - 0x7F - Numpad
}};

// Swedish Layout
// TODO: Add the "<>|" key
const u8 SCTable_SV[3][128] =
{
{   // Lower
//  x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xA   xB   xC   xD   xE   xF     
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x15, 0x0, // 0x00 - 0x0F
    0x0, 0x0, 0x0, 0x0, 0x0, 'q', '1', 0x0, 0x0, 0x0, 'z', 's', 'a', 'w', '2', 0x0, // 0x10 - 0x1F
    0x0, 'c', 'x', 'd', 'e', '4', '3', 0x0, 0x0, ' ', 'v', 'f', 't', 'r', '5', 0x0, // 0x20 - 0x2F
    0x0, 'n', 'b', 'h', 'g', 'y', '6', 0x0, 0x0, 0x0, 'm', 'j', 'u', '7', '8', 0x0, // 0x30 - 0x3F
    0x0, ',', 'k', 'i', 'o', '0', '9', 0x0, 0x0, '.', '-', 'l',0x94, 'p', '+', 0x0, // 0x40 - 0x4F
    0x0, 0x0,0x84, 0x0,0x86,0x60, 0x0, 0x0, 0x0, 0x0, 0x0, '"', 0x0,'\'', 0x0, 0x0, // 0x50 - 0x5F - 0x55 may be the wrong ' (´) - 0x5B is wrong " (should be ¨ but it is not in ascii)
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '1', 0x0, 0x0, '7', 0x0, 0x0, 0x0, // 0x60 - 0x6F - Numpad without 2468 directional
    '0', 0x0, 0x0, '5', 0x0, 0x0, 0x0, 0x0, 0x0, '+', '3', '-', '*', '9', 0x0, 0x0, // 0x70 - 0x7F - Numpad without 2468 directional
    //0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '1', 0x0, '4', '7', 0x0, 0x0, 0x0, // 0x60 - 0x6F - Numpad
    //'0', 0x0, '2', '5', '6', '8', 0x0, 0x0, 0x0, '+', '3', '-', '*', '9', 0x0, 0x0, // 0x70 - 0x7F - Numpad
},
{   // Shift+<KEY>
//  x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xA   xB   xC   xD   xE   xF     
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0xAB, 0x0, // 0x00 - 0x0F - 0x14=½
    0x0, 0x0, 0x0, 0x0, 0x0, 'Q', '!', 0x0, 0x0, 0x0, 'Z', 'S', 'A', 'W', '"', 0x0, // 0x10 - 0x1F
    0x0, 'C', 'X', 'D', 'E', 0x9, '#', 0x0, 0x0, ' ', 'V', 'F', 'T', 'R', '%', 0x0, // 0x20 - 0x2F - 0x25 = ¤ (Not in ascii) 
    0x0, 'N', 'B', 'H', 'G', 'Y', '&', 0x0, 0x0, 0x0, 'M', 'J', 'U', '/', '(', 0x0, // 0x30 - 0x3F  
    0x0, ';', 'K', 'I', 'O', '=', ')', 0x0, 0x0, ':', '_', 'L',0x99, 'P', '?', 0x0, // 0x40 - 0x4F - 0x4C = Ö
    0x0, 0x0,0x8E, 0x0,0x8F,0x27, 0x0, 0x0, 0x0, 0x0, 0x0, '^', 0x0, '*', 0x0, 0x0, // 0x50 - 0x5F - 0x55 may be the wrong ' (`)
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '1', 0x0, 0x0, '7', 0x0, 0x0, 0x0, // 0x60 - 0x6F - Numpad without 2468 directional
    '0', 0x0, 0x0, '5', 0x0, 0x0, 0x0, 0x0, 0x0, '+', '3', '-', '*', '9', 0x0, 0x0, // 0x70 - 0x7F - Numpad without 2468 directional
    //0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '1', 0x0, '4', '7', 0x0, 0x0, 0x0, // 0x60 - 0x6F - Numpad
    //'0', 0x0, '2', '5', '6', '8', 0x0, 0x0, 0x0, '+', '3', '-', '*', '9', 0x0, 0x0, // 0x70 - 0x7F - Numpad
},
{   // ALT+<KEY>
//  x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xA   xB   xC   xD   xE   xF     
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0x14, 0x0, // 0x00 - 0x0F  - 0x14=¶
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0xAE, 0x0, 0x0, 0x0, '@', 0x0, // 0x10 - 0x1F
    0x0, 0x0,0xAF, 0x0, 0x0, '$',0x9C, 0x0, 0x0, ' ', 0x0, 0x0, 0x0, 0x0,0xEE, 0x0, // 0x20 - 0x2F  - 0x2E = €
    0x0, 0x0, 0x0, 0x0, 0x0,0x1B,0x9D, 0x0, 0x0, 0x0,0xE6, 0x0,0x19, '{', '[', 0x0, // 0x30 - 0x3F
    0x0, 0x0, 0x0,0x1A, 0x0, '}', ']', 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,0xE3,'\\', 0x0, // 0x40 - 0x4F
    0x0, 0x0, 0x0, 0x0, 0x0,0xF1, 0x0, 0x0, 0x0, 0x0, 0x0,0x7E, 0x0,0x60, 0x0, 0x0, // 0x50 - 0x5F
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '1', 0x0, 0x0, '7', 0x0, 0x0, 0x0, // 0x60 - 0x6F - Numpad without 2468 directional
    '0', 0x0, 0x0, '5', 0x0, 0x0, 0x0, 0x0, 0x0, '+', '3', '-', '*', '9', 0x0, 0x0, // 0x70 - 0x7F - Numpad without 2468 directional
    //0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '1', 0x0, '4', '7', 0x0, 0x0, 0x0, // 0x60 - 0x6F - Numpad
    //'0', 0x0, '2', '5', '6', '8', 0x0, 0x0, 0x0, '+', '3', '-', '*', '9', 0x0, 0x0, // 0x70 - 0x7F - Numpad
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
}

void KB_SetKeyboard(KB_Poll_CB *cb)
{
    PollCB = cb;
}

bool KB_Poll(u8 *data)
{
    if (PollCB == NULL) return FALSE;

    return PollCB(data);
}

void KB_Interpret_Scancode(u8 scancode)
{
    if (bKB_Break)
    {
        set_KeyPress(((bKB_ExtKey?0x100:0) | scancode), KEYSTATE_UP);
        bKB_Break = FALSE;
        bKB_ExtKey = FALSE;
        switch (scancode)
        {
            case KEY_LSHIFT:
            case KEY_RSHIFT:
                bKB_Shift = 0;
            break;
            case 0x11:  //KEY_RALT
                bKB_Alt = 0;
            break;
            case KEY_LCONTROL:  // CTRL^ Sequence
                bKB_Ctrl = 0;
            break;
            default:
            break;
        }
        return;
    }

    switch (scancode)
    {
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
            TRM_SetStatusIcon(ICO_ID_ERROR, ICO_POS_0);
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

        default:
            set_KeyPress(((bKB_ExtKey?0x100:0) | scancode), KEYSTATE_DOWN);
        break;
    }

    // Ctrl^ sequence
    if (bKB_Ctrl)
    {
        u8 key = SCTablePtr[sv_KeyLayout][0][scancode];

        NET_BufferChar(key & 0x1F); // Add control byte to TxBuffer

        if (!vDoEcho) 
        {
            TTY_PrintChar('^');     // Print ^ to TTY if ECHO is false
            TTY_PrintChar(key);
        }
        
        return;
    }

    // Normal printing
    u8 mod = 0;
    if (bKB_Alt) mod = 2;
    else if (bKB_Shift) mod = 1;

    u8 key = SCTablePtr[sv_KeyLayout][mod][scancode];

    if (isPrintable(key) && !bWindowActive)
    {
        NET_BufferChar(key);                // Send key to TxBuffer
        if (!vDoEcho) TTY_PrintChar(key);   // Only print characters if ECHO is false
    }
}
