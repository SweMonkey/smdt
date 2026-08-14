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

SM_File *FILE_INTERNAL_tty    = NULL;
SM_File *FILE_INTERNAL_stdin  = NULL;
SM_File *FILE_INTERNAL_stdout = NULL;
SM_File *FILE_INTERNAL_stderr = NULL;

SM_File *tty    = NULL;
SM_File *stdin  = NULL;
SM_File *stdout = NULL;
SM_File *stderr = NULL;

void IO_CreatePseudoFiles()
{
    FILE_INTERNAL_tty    = tty    = NULL;
    FILE_INTERNAL_stdin  = stdin  = NULL;
    FILE_INTERNAL_stdout = stdout = NULL;
    FILE_INTERNAL_stderr = stderr = NULL;

    FILE_INTERNAL_tty = F_Open("/sram/system/tty.io", LFS_O_CREAT | LFS_O_TRUNC | LFS_O_RDWR | LFS_O_IO);
    if (FILE_INTERNAL_tty)
    {
        FILE_INTERNAL_tty->in_buf = &RxBuffer;
        FILE_INTERNAL_tty->out_buf = &TxBuffer;
    }
    tty = FILE_INTERNAL_tty;
    
    FILE_INTERNAL_stdin = F_Open("/sram/system/stdin.io", LFS_O_CREAT | LFS_O_RDONLY | LFS_O_IO);
    if (FILE_INTERNAL_stdin) FILE_INTERNAL_stdin->in_buf = &StdinBuffer;
    stdin = FILE_INTERNAL_stdin;
    
    FILE_INTERNAL_stdout = F_Open("/sram/system/stdout.io", LFS_O_CREAT | LFS_O_TRUNC | LFS_O_WRONLY | LFS_O_IO); 
    if (FILE_INTERNAL_stdout) FILE_INTERNAL_stdout->out_buf = &StdoutBuffer;
    stdout = FILE_INTERNAL_stdout;

    FILE_INTERNAL_stderr = F_Open("/sram/system/stderr.io", LFS_O_CREAT | LFS_O_TRUNC | LFS_O_WRONLY | LFS_O_IO);
    if (FILE_INTERNAL_stderr) FILE_INTERNAL_stderr->out_buf = &StdoutBuffer;
    stderr = FILE_INTERNAL_stderr;
}

void IO_ForceRestorePseudoFiles()
{
    tty    = FILE_INTERNAL_tty;
    stdin  = FILE_INTERNAL_stdin;
    stdout = FILE_INTERNAL_stdout;
    stderr = FILE_INTERNAL_stderr;
}

// Hacky function to pause printing when screen has been filled
void MoreFunc()
{
    u8 kbdata = 0;

    TELNET_ParseRX('\e');
    TELNET_ParseRX('[');
    TELNET_ParseRX('7');
    TELNET_ParseRX('m');
    TELNET_ParseRX('<');
    TELNET_ParseRX('M');
    TELNET_ParseRX('o');
    TELNET_ParseRX('r');
    TELNET_ParseRX('e');
    TELNET_ParseRX('>');
    TELNET_ParseRX('\e');
    TELNET_ParseRX('[');
    TELNET_ParseRX('0');
    TELNET_ParseRX('m');
    TELNET_ParseRX(' ');

    while (KB_Poll(&kbdata));   // Flush any data that the keyboard may have in its buffer

    while (1)
    {
        while (KB_Poll(&kbdata))
        {
            KB_Interpret_Scancode(kbdata);
        }

        if (is_AnyKey())
        {
            TTY_MoveCursor(TTY_CURSOR_LEFT, TTY_GetSX());
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
        }

        str++; // Move to the next character
    }
}

void Stdout_PushS16(const char *str, s16 num, const char *endstr)
{
    char buf[128];

    snprintf(buf, 128, "%s %d %s", str, num, endstr);
    Stdout_Push(buf);
}

bool Stdout_PushByte(u8 byte)
{
    return Buffer_Push(&StdoutBuffer, byte);
}

void Stdout_Flush()
{
    s16 start = TTY_GetSY();
    u8 data = 0;

    while (Buffer_Pop(&StdoutBuffer, &data))
    {
        TELNET_ParseRX(data);

        if ((TTY_GetSY() - start) > (bPALSystem?27:25))
        {
            start = TTY_GetSY();
            MoreFunc();
        }
    }
}
