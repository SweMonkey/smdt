#include "PseudoFile.h"
#include "Telnet.h"
#include "Terminal.h"   // TTY_
#include "Utils.h"      // bPALSystem
#include "Input.h"      // is_AnyKey()
#include "StateCtrl.h"  // StateTick()
#include "Network.h"
#include "Keyboard.h"

Buffer StdoutBuffer;
Buffer StdinBuffer;
bool bAutoFlushStdout = FALSE;

void ShellDrawClockUpdate();

SM_File *FILE_INTERNAL_tty_in  = NULL;
SM_File *FILE_INTERNAL_tty_out = NULL;
SM_File *FILE_INTERNAL_stdin   = NULL;
SM_File *FILE_INTERNAL_stdout  = NULL;
SM_File *FILE_INTERNAL_stderr  = NULL;

SM_File *tty_in  = NULL;
SM_File *tty_out = NULL;
SM_File *stdin   = NULL;
SM_File *stdout  = NULL;
SM_File *stderr  = NULL;

void IO_CreatePseudoFiles()
{
    FILE_INTERNAL_tty_in  = tty_in  = NULL;
    FILE_INTERNAL_tty_out = tty_out = NULL;
    FILE_INTERNAL_stdin   = stdin   = NULL;
    FILE_INTERNAL_stdout  = stdout  = NULL;
    FILE_INTERNAL_stderr  = stderr  = NULL;

    FILE_INTERNAL_tty_in  = F_Open("/sram/system/tty_in.io", LFS_O_CREAT | LFS_O_RDONLY | LFS_O_IO);
    FILE_INTERNAL_tty_in->io_buf = &RxBuffer;
    tty_in = FILE_INTERNAL_tty_in;

    FILE_INTERNAL_tty_out = F_Open("/sram/system/tty_out.io", LFS_O_CREAT | LFS_O_TRUNC | LFS_O_WRONLY | LFS_O_IO);
    FILE_INTERNAL_tty_out->io_buf = &TxBuffer;
    tty_out = FILE_INTERNAL_tty_out;
    
    FILE_INTERNAL_stdin = F_Open("/sram/system/stdin.io", LFS_O_CREAT | LFS_O_RDONLY | LFS_O_IO);
    FILE_INTERNAL_stdin->io_buf = &StdinBuffer;
    stdin = FILE_INTERNAL_stdin;
    
    FILE_INTERNAL_stdout = F_Open("/sram/system/stdout.io", LFS_O_CREAT | LFS_O_TRUNC | LFS_O_WRONLY | LFS_O_IO); 
    FILE_INTERNAL_stdout->io_buf = &StdoutBuffer;
    stdout = FILE_INTERNAL_stdout;

    FILE_INTERNAL_stderr = F_Open("/sram/system/stderr.io", LFS_O_CREAT | LFS_O_TRUNC | LFS_O_WRONLY | LFS_O_IO);
    FILE_INTERNAL_stderr->io_buf = &StdoutBuffer;
    stderr = FILE_INTERNAL_stderr;
}

void IO_ForceRestorePseudoFiles()
{
    tty_in  = FILE_INTERNAL_tty_in;
    tty_out = FILE_INTERNAL_tty_out;
    stdin   = FILE_INTERNAL_stdin;
    stdout  = FILE_INTERNAL_stdout;
    stderr  = FILE_INTERNAL_stderr;
}

// Hacky function to pause printing when screen has been filled
static void MoreFunc()
{
    u8 kbdata = 0;

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

        VDP_waitVSync();
        VBlank();
        ShellDrawClockUpdate();
    }

    InputTick();    // Flush input queue to prevent inputs from above "leaking" out into stdout
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
    s16 start = TTY_GetSY();
    u8 data = 0;

    while (Buffer_Pop(&StdoutBuffer, &data))
    {
        TELNET_ParseRX(data);

        if ((TTY_GetSY() - start) >= (bPALSystem?27:25))
        {
            start = TTY_GetSY();
            MoreFunc();
        }
    }
}
