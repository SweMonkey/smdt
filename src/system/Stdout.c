#include "Stdout.h"
#include "Telnet.h"
#include "Terminal.h"   // TTY_
#include "Utils.h"      // bPALSystem
#include "Input.h"      // is_AnyKey()
#include "StateCtrl.h"  // StateTick()
#include "Keyboard.h"

Buffer stdout;
bool bAutoFlushStdout = FALSE;


// Hacky function to pause printing when screen has been filled
void MoreFunc(s32 *start)
{
    u8 kbdata;
    
    if (((TTY_GetSY()) - *start) >= (bPALSystem?27:25))
    {
        TELNET_ParseRX('\n');
        TELNET_ParseRX('');
        TELNET_ParseRX('[');
        TELNET_ParseRX('7');
        TELNET_ParseRX('m');
        TELNET_ParseRX('M');
        TELNET_ParseRX('o');
        TELNET_ParseRX('r');
        TELNET_ParseRX('e');
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
                break;
            }

            SYS_doVBlankProcess();
        }

        *start = TTY_GetSY();
        InputTick();    // Flush input queue to prevent inputs from above "leaking" out into stdout
    }
}

void Stdout_Push(const char *str)
{
    u8 r = 0;
    for (u16 c = 0; c < strlen(str); c++)
    {
        if (bAutoFlushStdout) TELNET_ParseRX((u8)str[c]);
        else r = Buffer_Push(&stdout, (u8)str[c]);

        // Check if stdout is full, if it is then flush it
        if (r)
        {
            Stdout_Flush();
            Buffer_Push(&stdout, (u8)str[c]);   // Push character again since it was previously dropped
        }
    }
}

void Stdout_PushByte(u8 byte)
{
    Buffer_Push(&stdout, byte);
}

void Stdout_Flush()
{
    u8 data = 0;
    //s32 start = TTY_GetSY();

    while (Buffer_Pop(&stdout, &data) != 0xFF)
    {
        TELNET_ParseRX(data);
        //MoreFunc(&start);
    }
}
