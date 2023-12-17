
#include "Keyboard_PS2.h"
#include "StateCtrl.h"  // bWindowActivev
#include "QMenu.h"      // ChangeText() when KB is detected
#include "Terminal.h"
#include "Telnet.h"
#include "Input.h"
#include "IRQ.h"    // Buffer functions

#define KB_CL 0
#define KB_DT 1

u8 KB_Initialized = FALSE;
static u8 bExtKey = FALSE;
static u8 bBreak = FALSE;
static u8 bCaps = FALSE;
static u8 bShift = FALSE;
static u8 bAlt = FALSE;
static u8 bCtrl = FALSE;

static const u8 ScancodeTable[256] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '`', 0, 0, 0, 0, 0, 0, 'q', '1', 0, 0, 0, 'z', 's', 'a', 'w', '2', 0, 0, 'c', 'x', 'd', 'e', '4', '3', 0, //   0-39
    0, ' ', 'v', 'f', 't', 'r', '5', 0, 0, 'n', 'b', 'h', 'g', 'y', '6', 0, 0, 0, 'm', 'j', 'u', '7', '8', 0, 0, ',', 'k', 'i', 'o', '0', '9', 0, 0, '.', '/', 'l', ';', 'p', '-', 0, //  40-79
    0, 0, '\'', 0, '[', '=', 0, 0, 0, 0, 0, ']', 0, '\\', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '1', 0, '4', '7', 0, 0, 0, '0', '.', '2', '5', '6', '8', 0, 0, //  80-119
    0, '+', '3', '-', '*', '9', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 120-159
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 160-199
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 200-239
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0                                                                          // 240-255
};

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
    bCaps = FALSE;
    bShift = FALSE;
    bAlt = FALSE;
    bCtrl = FALSE;
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

// https://www.win.tue.nl/~aeb/linux/kbd/scancodes-10.html#scancodesets
// Using set 2
void KB_Handle_Scancode(u8 scancode)
{
    if (scancode == 0) return;

    if (bExtKey)
    {
        KB_Handle_EXT_Scancode(scancode);
        bExtKey = FALSE;

        return;
    }

    if (bBreak)
    {
        bBreak = FALSE;

        switch (scancode)
        {
            case 0:
                return;
            break;

            case 0x11:  // LAlt
                bAlt = FALSE;    // was TRUE, copypaste error?
                set_KeyPress(KEY_LALT, KEYSTATE_UP);
                return;
            break;

            case 0x12:  // LShift
                bShift = FALSE;
                set_KeyPress(KEY_LSHIFT, KEYSTATE_UP);
                return;
            break;

            case 0x59:  // RShift
                bShift = FALSE;
                set_KeyPress(KEY_RSHIFT, KEYSTATE_UP);
                return;
            break;

            default:
            break;
        }

        return;
    }

    switch (scancode)
    {
        case 0:
            return;
        break;

        case 0x11:  // LAlt
            bAlt = TRUE;
            set_KeyPress(KEY_LALT, KEYSTATE_DOWN);
            return;
        break;

        case 0x12:  // LShift
            bShift = TRUE;
            set_KeyPress(KEY_LSHIFT, KEYSTATE_DOWN);
            return;
        break;

        case 0x59:  // RShift
            bShift = TRUE;
            set_KeyPress(KEY_RSHIFT, KEYSTATE_DOWN);
            return;
        break;

        case 0x58:  // Capslock
            bCaps = !bCaps;
            set_KeyPress(KEY_CAPITAL, bCaps);
            return;
        break;

        case 0x5A:  // Return
            set_KeyPress(KEY_RETURN, KEYSTATE_DOWN);

            if (!bWindowActive)
            {
              /*When LINEMODE is turned on, and when in EDIT mode, when any normal
                line terminator on the client side operating system is typed, the
                line should be transmitted with "CR LF" as the line terminator.  When
                EDIT mode is turned off, a carriage return should be sent as "CR
                NUL", a line feed should be sent as LF, and any other key that cannot
                be mapped into an ASCII character, but means the line is complete
                (like a DOIT or ENTER key), should be sent as "CR LF".*/
                if (vLineMode & LMSM_EDIT)
                {
                    TTY_SendChar(0xD, 0); // Send \r - carridge return
                    TTY_SendChar(0xA, 0); // Send \n - line feed
                    TTY_TransmitBuffer();
                }
                else
                {
                    TTY_SendChar(0xD, TXF_NOBUFFER); // Send \r - carridge return
                    //TTY_SendChar(0, TXF_NOBUFFER); // Send NUL
                    TTY_SendChar(0xA, TXF_NOBUFFER); // Send \n - line feeds
                }

                //TTY_TransmitBuffer();

                // -- CheckMe: Should cursor actually be moved here? or be echoed back from server?

                //line feed, new line
                TTY_MoveCursor(TTY_CURSOR_DOWN, 1);
                TTY_ClearLine(sy % 32, 4);

                //carriage return
                sx = 0;
                TTY_MoveCursor(TTY_CURSOR_DUMMY);   // Dummy
            }

            return;
        break;

        case 0x66:  // Backspace
            set_KeyPress(KEY_BACKSPACE, KEYSTATE_DOWN);

            if (!bWindowActive)
            {
                // -- CheckMe: Should cursor actually be moved here? or be echoed back from server?

                TTY_MoveCursor(TTY_CURSOR_LEFT, 1);
                VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(1, 0, 0, 0, 0), sx, sy);
                VDP_setTileMapXY(BG_B, TILE_ATTR_FULL(2, 0, 0, 0, 0), sx, sy);

                // 0x8  = backspace
                // 0x7F = delete
                if (vLineMode & LMSM_EDIT)
                {
                    Buffer_ReversePop(&TxBuffer);
                }
                else
                {
                    TTY_SendChar(0x8, TXF_NOBUFFER); // send backspace
                }
            }

            return;
        break;

        case 0x76:  // Escape
            set_KeyPress(KEY_ESCAPE, KEYSTATE_DOWN);

            if (!bWindowActive)
            {
                TTY_SendChar(0x1B, 0); // Send \ESC
            }

            return;
        break;

        case 0xAA:  // BAT OK
            print_charXY_WP(ICO_KB_OK, STATUS_KB_POS, CHAR_GREEN);
            ChangeText(7, 0, "PORT 1: KEYBOARD");

            return;
        break;

        case 0xE0:
            bExtKey = TRUE;

            return;
        break;

        case 0xF0:
            bBreak = TRUE;

            return;
        break;

        case 0xFC: // BAT FAIL
            print_charXY_WP(ICO_KB_ERROR, STATUS_KB_POS, CHAR_RED);
            ChangeText(7, 0, "PORT 1: <ERROR>");

            return;
        break;

        default:
        break;
    }

    u8 key = ScancodeTable[scancode];

    if ((key >= 0x20) && (key <= 0x7E) && (!bWindowActive))
    {
        char chr = ScancodeTable[scancode] - ((bCaps || bShift) ? 32 : 0);
        
        // Only print characters if ECHO is false
        if (!vDoEcho) TTY_PrintChar(chr);

        TTY_SendChar(chr, 0);
    }
}

void KB_Handle_EXT_Scancode(u8 scancode)
{
    switch (scancode)
    {
        case 0:
            return;
        break;

        /*case 0x1F:  // Left GUI
            TTY_Reset(TRUE);
            return;
        break;*/

        case 0x11:  // RAlt
            bAlt = TRUE;
            set_KeyPress(KEY_RALT, KEYSTATE_DOWN);
            return;
        break;

        case 0x1F:  // LWIN
            set_KeyPress(KEY_LWIN, KEYSTATE_DOWN);
            return;
        break;

        case 0x27:  // RWIN
            set_KeyPress(KEY_RWIN, KEYSTATE_DOWN);
            return;
        break;

        case 0x37:  // Power button
            set_KeyPress(KEY_SLEEP, KEYSTATE_DOWN);
            TTY_Reset(TRUE);
            return;
        break;

        case 0x6B:  // Left arrow
            set_KeyPress(KEY_LEFT, KEYSTATE_DOWN);

            if (!bWindowActive)
            {
                if (!FontSize)
                {
                    HScroll += 8;
                    VDP_setHorizontalScrollVSync(BG_A, HScroll % 1024);
                    VDP_setHorizontalScrollVSync(BG_B, HScroll % 1024);
                }

                TTY_SendChar(0x1B, TXF_NOBUFFER);    // ESC
                TTY_SendChar(0x5B, TXF_NOBUFFER);    // [
                TTY_SendChar(0x44, TXF_NOBUFFER);    // D
            }

            #ifndef NO_LOGGING
                KLog("KB; Extended key Left arrow");
            #endif

            #ifdef KB_DEBUG
                TTY_PrintChar('L');
            #endif

            return;
        break;

        case 0x71:  // Delete
            set_KeyPress(KEY_DELETE, KEYSTATE_DOWN);

            TTY_SendChar(0x1B, TXF_NOBUFFER);    // ESC
            TTY_SendChar(0x5B, TXF_NOBUFFER);    // [
            TTY_SendChar(0x7F, TXF_NOBUFFER);    // DEL

            return;
        break;

        case 0x72:  // Down arrow
            set_KeyPress(KEY_DOWN, KEYSTATE_DOWN);

            if (!bWindowActive)
            {
                TTY_SendChar(0x1B, TXF_NOBUFFER);    // ESC
                TTY_SendChar(0x5B, TXF_NOBUFFER);    // [
                TTY_SendChar(0x42, TXF_NOBUFFER);    // B
            }

            #ifndef NO_LOGGING
                KLog("KB; Extended key Down arrow");
            #endif

            #ifdef KB_DEBUG
                TTY_PrintChar('D');
            #endif

            return;
        break;

        case 0x74:  // Right arrow
            set_KeyPress(KEY_RIGHT, KEYSTATE_DOWN);

            if (!bWindowActive)
            {
                if (!FontSize)
                {
                    HScroll -= 8;
                    VDP_setHorizontalScrollVSync(BG_A, HScroll % 1024);
                    VDP_setHorizontalScrollVSync(BG_B, HScroll % 1024);
                }

                TTY_SendChar(0x1B, TXF_NOBUFFER);    // ESC
                TTY_SendChar(0x5B, TXF_NOBUFFER);    // [
                TTY_SendChar(0x43, TXF_NOBUFFER);    // C
            }

            #ifndef NO_LOGGING
                KLog("KB; Extended key Right arrow");
            #endif

            #ifdef KB_DEBUG
                TTY_PrintChar('R');
            #endif

            return;
        break;

        case 0x75:  // Up arrow
            set_KeyPress(KEY_UP, KEYSTATE_DOWN);

            if (!bWindowActive)
            {
                TTY_SendChar(0x1B, TXF_NOBUFFER);    // ESC
                TTY_SendChar(0x5B, TXF_NOBUFFER);    // [
                TTY_SendChar(0x41, TXF_NOBUFFER);    // A
            }

            #ifndef NO_LOGGING
                KLog("KB; Extended key Up arrow");
            #endif

            #ifdef KB_DEBUG
                TTY_PrintChar('U');
            #endif

            return;
        break;

        case 0xF0:
            bBreak = TRUE;

            #ifndef NO_LOGGING
                KLog("KB; Extended key break");
            #endif

            return;
        break;

        default:
        break;
    }
}
