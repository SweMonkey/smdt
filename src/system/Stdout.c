#include "Stdout.h"
#include "Telnet.h"
#include "Terminal.h"   // TTY_
#include "Utils.h"      // bPALSystem
#include "Input.h"      // is_AnyKey()
#include "StateCtrl.h"  // StateTick()
#include "Keyboard.h"

Buffer StdoutBuffer;
extern Buffer TxBuffer;
bool bAutoFlushStdout = FALSE;

void TickClock();
void ScreensaverTick();
void CR_Blink();

SM_File *rxbuf = NULL;
SM_File *txbuf = NULL;
SM_File *stdout = NULL;
SM_File *stdin = NULL;
SM_File *stderr = NULL;


// Hacky function to pause printing when screen has been filled
void MoreFunc(s16 *start)
{
    u8 kbdata = 0;
    
    if (((TTY_GetSY()) - *start) >= (bPALSystem?27:25))
    {
        TELNET_ParseRX('\n');
        TELNET_ParseRX('');
        TELNET_ParseRX('[');
        TELNET_ParseRX('7');
        TELNET_ParseRX('m');
        TELNET_ParseRX('<');
        TELNET_ParseRX('M');
        TELNET_ParseRX('o');
        TELNET_ParseRX('r');
        TELNET_ParseRX('e');
        TELNET_ParseRX('>');
        TELNET_ParseRX(' ');
        TELNET_ParseRX('');
        TELNET_ParseRX('[');
        TELNET_ParseRX('0');
        TELNET_ParseRX('m');

        while (1)
        {
            while (KB_Poll(&kbdata))
            {
                KB_Interpret_Scancode(kbdata);
            }

            if (is_AnyKey())
            {
                TTY_SetSX(0);
                TTY_MoveCursor(TTY_CURSOR_UP, 1); // Only move up in case the initial \n is printed above
                Buffer_Flush(&TxBuffer);
                break;
            }

            #ifdef ENABLE_CLOCK
            TickClock();        // Clock will drift when interrupts are disabled!
            #endif
            ScreensaverTick();  // Screensaver counter/animation
            CR_Blink();         // Cursor blink
            VDP_waitVSync();
        }

        *start = TTY_GetSY();
        InputTick();    // Flush input queue to prevent inputs from above "leaking" out into stdout
    }
}

void Stdout_Push(const char *str)
{
    bool r = TRUE;
    
    while (*str) // Loop until the null terminator
    {
        if (bAutoFlushStdout) TELNET_ParseRX((u8)*str);
        else r = Buffer_Push(&StdoutBuffer, (u8)*str);

        // Check if stdout is full, if it is then flush it
        if (r == FALSE)
        {
            Stdout_Flush();
            Buffer_Push(&StdoutBuffer, (u8)*str);   // Push character again since it was previously dropped
        }

        str++; // Move to the next character
    }
}

void Stdout_PushByte(u8 byte)
{
    Buffer_Push(&StdoutBuffer, byte);
}

void Stdout_Flush()
{
    u8 data = 0;
    s16 start = TTY_GetSY();

    while (Buffer_Pop(&StdoutBuffer, &data))
    {
        TELNET_ParseRX(data);
        MoreFunc(&start);
    }
}
