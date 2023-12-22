
#include "Keyboard_PS2.h"
#include "StateCtrl.h"  // bWindowActive
#include "QMenu.h"      // ChangeText() when KB is detected
#include "Terminal.h"
#include "Telnet.h"
#include "Input.h"
#include "Buffer.h"
#include "Utils.h"

#define KB_CL 0
#define KB_DT 1

// https://www.win.tue.nl/~aeb/linux/kbd/scancodes-10.html#scancodesets
// Using set 2

u8 KB_Initialized = FALSE;
static u8 bExtKey = FALSE;
static u8 bBreak = FALSE;
static u8 bShift = FALSE;
static u8 bAlt = FALSE;

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
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // 0x60 - 0x6F
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // 0x70 - 0x7F
},
{   // Shift+<KEY>
//  x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xA   xB   xC   xD   xE   xF     
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '~', 0x0, // 0x00 - 0x0F
    0x0, 0x0, 0x0, 0x0, 0x0, 'Q', '!', 0x0, 0x0, 0x0, 'Z', 'S', 'A', 'W', '@', 0x0, // 0x10 - 0x1F
    0x0, 'C', 'X', 'D', 'E', '$', '#', 0x0, 0x0, ' ', 'V', 'F', 'T', 'R', '%', 0x0, // 0x20 - 0x2F
    0x0, 'N', 'B', 'H', 'G', 'Y', '^', 0x0, 0x0, 0x0, 'M', 'J', 'U', '&', '*', 0x0, // 0x30 - 0x3F
    0x0, '<', 'K', 'I', 'O', ')', '(', 0x0, 0x0, '>', '?', 'L', ':', 'P', '_', 0x0, // 0x40 - 0x4F
    0x0, 0x0, '"', 0x0, '{', '+', 0x0, 0x0, 0x0, 0x0, 0x0, '}', 0x0, '|', 0x0, 0x0, // 0x50 - 0x5F
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // 0x60 - 0x6F
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // 0x70 - 0x7F
},
{   // ALT+<KEY>
//  x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xA   xB   xC   xD   xE   xF     
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // 0x00 - 0x0F
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // 0x10 - 0x1F
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, ' ', 0x0, 0x0, 0x0, 0x0, 'E', 0x0, // 0x20 - 0x2F  // E = €
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // 0x30 - 0x3F
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // 0x40 - 0x4F
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // 0x50 - 0x5F
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // 0x60 - 0x6F
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, // 0x70 - 0x7F
}};


void KB_Init()
{
    vu8 *PCTRL;
    vu8 *PDATA;

    PCTRL = (vu8 *)KB_PORT_CTRL;
    *PCTRL = 0;   // All pins are input

    PDATA = (vu8 *)KB_PORT_DATA;
    *PDATA = 0;

    KB_Initialized = TRUE;
    bExtKey = FALSE;
    bBreak = FALSE;
    bShift = FALSE;
    bAlt = FALSE;

    // Writing 0xf0 followed by 1, 2 or 3 to port 0x60 will put the keyboard in scancode mode 1, 2 or 3.
    // Writing 0xf0 followed by 0 queries the mode, resulting in a scancode byte 43, 41 or 3f from the keyboard.
}

inline void KB_Lock()
{
    vu8 *PCTRL = (vu8 *)KB_PORT_CTRL;
    vu8 *PDATA = (vu8 *)KB_PORT_DATA;

    *PCTRL = 0x3;   // Set pin 0 and 1 as output (smd->kb)
    *PDATA = 0x2;   // Set clock low, data high - Stop kb sending data
}

inline void KB_Unlock()
{
    vu8 *PCTRL = (vu8 *)KB_PORT_CTRL;
    vu8 *PDATA = (vu8 *)KB_PORT_DATA;

    *PDATA = 0x3; // Set clock high, data high - Allow kb to send data
    *PCTRL = 0;   // Set pin 0 and 1 as input (kb->smd)
}

u8 KB_Poll(u8 *data)
{
    u32 timeout = 0;
    u16 stream_buffer = 0;
    vu8 *PDATA = (vu8 *)KB_PORT_DATA;

    KB_Unlock();

    while ((*PDATA & 1) == 1)
    {
        PDATA = (vu8 *)KB_PORT_DATA;

        if (timeout >= 3200)   // 32000
        {
            KB_Lock();
            return 0;
        }

        timeout++;
    }

    for (u8 b = 0; b < 11; b++)  // Recieve byte
    {
        while ((*PDATA & 1) == 1){PDATA = (vu8 *)KB_PORT_DATA;} // Wait for clock to go low

        stream_buffer |= ((*PDATA >> KB_DT) & 1) << b;

        while ((*PDATA & 1) == 0){PDATA = (vu8 *)KB_PORT_DATA;} // Wait for clock to go high
    }

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

void KB_Interpret_Scancode(u8 scancode)
{
    if (bBreak)
    {
        set_KeyPress(((bExtKey?0x100:0) | scancode), KEYSTATE_UP);
        bBreak = FALSE;
        bExtKey = FALSE;
        switch (scancode)
        {
            case KEY_LSHIFT:
            case KEY_RSHIFT:
                bShift = 0;
            break;
            case 0x11:  //KEY_RALT
                bAlt = 0;
            break;
            default:
            break;
        }
        return;
    }

    switch (scancode)
    {
        case 0xAA:  // BAT OK
            print_charXY_WP(ICO_KB_OK, STATUS_KB_POS, CHAR_GREEN);
            ChangeText(7, 0, "PORT 1: KEYBOARD");
        break;
        case 0xE0:
            bExtKey = TRUE;
        break;
        case 0xF0:
            bBreak = TRUE;
        break;
        case 0xFC: // BAT FAIL
            print_charXY_WP(ICO_KB_ERROR, STATUS_KB_POS, CHAR_RED);
            ChangeText(7, 0, "PORT 1: <ERROR>");
        break;

        // Hopefully temporary shitcode
        // These keys will not be down/up whenever a character will be printed
        // Temporarily buffer these keys locally...
        case KEY_LSHIFT:
        case KEY_RSHIFT:
            set_KeyPress(((bExtKey?0x100:0) | scancode), KEYSTATE_DOWN);
            bShift = 1;
        break;
        case 0x11:  //KEY_RALT
            set_KeyPress(((bExtKey?0x100:0) | scancode), KEYSTATE_DOWN);
            bAlt = 1;
        break;

        default:
            set_KeyPress(((bExtKey?0x100:0) | scancode), KEYSTATE_DOWN);
        break;
    }

    // More shit that should not be here...
    u8 mod = 0;
    if (bAlt) mod = 2;
    else if (bShift) mod = 1;

    u8 key = SCTable_US[mod][scancode];

    if ((key >= 0x20) && (key <= 0x7E) && (!bWindowActive))
    {
        // Only print characters if ECHO is false
        if (!vDoEcho) TTY_PrintChar(key);

        TTY_SendChar(key, 0);
    }
}
