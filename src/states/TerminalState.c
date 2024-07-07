#include "StateCtrl.h"
#include "Telnet.h"
#include "Terminal.h"
#include "Buffer.h"
#include "Input.h"
#include "Utils.h"
#include "Network.h"
#include "misc/CMDFunc.h"

#include "system/Stdout.h"
#include "system/Time.h"

#ifdef KERNEL_BUILD
#include "system/File.h"
#include "system/Filesystem.h"
#endif

#define INPUT_SIZE 128
#define INPUT_SIZE_ARGV 64

u8 sv_TerminalFont = FONT_8x8_16;//FONT_4x8_8;//
static char LastCommand[INPUT_SIZE] = {'\0'};
static char TimeString[9];
static s32 LastTime = 666;


u8 ParseInputString()
{
    u8 inbuf[INPUT_SIZE] = {0};     // Input buffer string
    char *argv[INPUT_SIZE_ARGV];    // Argument list
    int argc = 0;   // Argument count
    u8 data;        // Byte buffer
    u16 i = 0;      // Buffer iterator
    u16 l = 0;      // List position

    memset(inbuf, 0, INPUT_SIZE);

    // Pop the TxBuffer back into inbuf
    while ((Buffer_Pop(&TxBuffer, &data) != 0xFF) && (i < INPUT_SIZE-1))
    {
        inbuf[i] = data;
        i++;
    }

    // Clear TxBuffer input
    Buffer_Flush0(&TxBuffer);

    // Copy the current input string into last command string
    if (strlen((char*)inbuf) > 0) strncpy(LastCommand, (char*)inbuf, INPUT_SIZE);

    // Extract argument list from input buffer string
    char *p = strtok((char*)inbuf, ' ');
    while (p != NULL)
    {
        argv[argc++] = p;
        p = strtok(NULL, ' ');
    }

    // Iterate argv0 through the command list and call bound function
    while (CMDList[l].id != 0)
    {
        if (strcmp(argv[0], CMDList[l].id) == 0)
        {
            CMDList[l].fptr(argc, argv);

            for (u8 a = 0; a < argc; a++) free(argv[a]);                // !!!

            return 1;
        }

        l++;
    }

    // Or let the user know that the command was not found
    if (strlen(argv[0]) > 0)
    {
        char tmp[64];
        sprintf(tmp, "Command \"[36m%s[0m\" not found...\n", argv[0]);
        Stdout_Push(tmp);
        return 1;
    }

    return 0;
}

u8 DoBackspace()
{
    if (Buffer_IsEmpty(&TxBuffer)) return 0;

    TTY_MoveCursor(TTY_CURSOR_LEFT, 1);

    if (!sv_Font)
    {
        VDP_setTileMapXY(BG_B, TILE_ATTR_FULL(2, 0, 0, 0, 0), TTY_GetSX(), sy);
    }
    else
    {
        VDP_setTileMapXY(!(TTY_GetSX() & 1), TILE_ATTR_FULL(2, 0, 0, 0, 0), TTY_GetSX()>>1, sy);
    }

    Buffer_ReversePop(&TxBuffer);

    return 1;
}

void SetupTerminal()
{
    TELNET_Init();

    // Variable overrides
    vDoEcho = 0;
    vLineMode = 1;
    vNewlineConv = 1;
    sv_bWrapAround = TRUE;

    TTY_SetFontSize(sv_TerminalFont);

    LastCommand[0] = '\0';

    C_XMAX = (sv_TerminalFont == 0 ? 39 : 79);  // Make the cursor wrap at screen edge
}

void Enter_Terminal(u8 argc, char *argv[])
{
    SetupTerminal();
    Stdout_Push("SMDTC Command Interpreter v0.2\nType \"[32mhelp[0m\" for available commands\n\n");

    Stdout_Push(">");
}

void ReEnter_Terminal()
{
    SetupTerminal();
    
    if (Buffer_IsEmpty(&stdout))
    {
        Stdout_Push(">");
    }
}

void Exit_Terminal()
{
}

void Reset_Terminal()
{
    TTY_Reset(TRUE);
}

void Run_Terminal()
{
    Stdout_Flush();

    if (LastTime != SystemUptime)
    {
        TimeToStr_Time(SystemTime, TimeString);
        TRM_DrawText(TimeString, 27, 0, PAL1);
        LastTime = SystemUptime;
    }
}

void Input_Terminal()
{
    if (!bWindowActive)
    {
        if (is_KeyDown(KEY_UP))
        {
            while (DoBackspace());

            Stdout_Push(LastCommand);

            for (u16 i = 0; i < strlen(LastCommand); i++) Buffer_Push(&TxBuffer, LastCommand[i]);
        }

        if (is_KeyDown(KEY_DOWN))
        {
            while (DoBackspace());
        }

        if (is_KeyDown(KEY_KP4_LEFT))
        {
            if (!sv_Font)
            {
                HScroll += 8;
                VDP_setHorizontalScroll(BG_A, HScroll);
                VDP_setHorizontalScroll(BG_B, HScroll);
            }
            else
            {
                HScroll += 4;
                VDP_setHorizontalScroll(BG_A, (HScroll+4));  // -4
                VDP_setHorizontalScroll(BG_B, (HScroll  ));  // -8
            }

            TTY_MoveCursor(TTY_CURSOR_DUMMY);
        }

        if (is_KeyDown(KEY_KP6_RIGHT))
        {
            if (!sv_Font)
            {
                HScroll -= 8;
                VDP_setHorizontalScroll(BG_A, HScroll);
                VDP_setHorizontalScroll(BG_B, HScroll);
            }
            else
            {
                HScroll -= 4;
                VDP_setHorizontalScroll(BG_A, (HScroll+4));  // -4
                VDP_setHorizontalScroll(BG_B, (HScroll  ));  // -8
            }
            
            TTY_MoveCursor(TTY_CURSOR_DUMMY);
        }

        if (is_KeyDown(KEY_DELETE))
        {
        }

        if (is_KeyDown(KEY_RETURN))
        {        
            // Line feed (new line)
            TTY_MoveCursor(TTY_CURSOR_DOWN, 1);
            TTY_ClearLine(sy % 32, 1);

            // Carriage return
            TTY_SetSX(0);
            TTY_MoveCursor(TTY_CURSOR_DUMMY);

            if (ParseInputString())
            {
                Stdout_Flush();     // Flush stdout before printing the newline below, otherwise it might get caught up in the "More" prompt
                
                if (isCurrentState(PS_Terminal)) Stdout_Push("\n");
            }
            
            if (isCurrentState(PS_Terminal))
            {
                Stdout_Push(">");
            }
        }

        if (is_KeyDown(KEY_BACKSPACE))
        {
            DoBackspace();
        }
    }
}

const PRG_State TerminalState = 
{
    Enter_Terminal, ReEnter_Terminal, Exit_Terminal, Reset_Terminal, Run_Terminal, Input_Terminal, NULL, NULL
};

