#include "StateCtrl.h"
#include "Telnet.h"
#include "Terminal.h"
#include "Buffer.h"
#include "Input.h"
#include "Keyboard.h"
#include "Utils.h"
#include "Network.h"
#include "misc/CMDFunc.h"

#include "DevMgr.h"
#include "devices/XP_Network.h"

#define INPUT_SIZE 96
#define INPUT_SIZE_ARGV 64

static u8 kbdata;
static u8 pFontSize = 0;
static char LastCommand[INPUT_SIZE] = {'\0'};


void PrintOutput(const char *str)
{
    for (u16 c = 0; c < strlen(str); c++)
    {
        TELNET_ParseRX((u8)str[c]);
    }
}

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
    while ((Buffer_Pop(&TxBuffer, &data) != 0xFF) && (i < INPUT_SIZE))
    {
        inbuf[i] = data;
        i++;
    }

    // Clear TxBuffer input
    Buffer_Flush(&TxBuffer);

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

            for (u8 a = 0; a < argc; a++) if (argv[a] != NULL) free(argv[a]);                // !!!

            return 1;
        }

        l++;
    }

    // Or let the user know that the command was not found
    if (strlen(argv[0]) > 0)
    {
        char tmp[64];
        sprintf(tmp, "Command \"%s\" not found...\n", argv[0]);
        PrintOutput(tmp);
        return 1;
    }

    return 0;
}

u8 DoBackspace()
{
    if (Buffer_IsEmpty(&TxBuffer)) return 0;

    TTY_MoveCursor(TTY_CURSOR_LEFT, 1);

    if (!FontSize)
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
    TRM_SetStatusText(STATUS_TEXT);

    // Variable overrides
    vDoEcho = 0;
    vLineMode = 1;
    vNewlineConv = 1;
    bWrapAround = TRUE;

    pFontSize = FontSize;
    TTY_SetFontSize(0);

    LastCommand[0] = '\0';
}

void Enter_Terminal(u8 argc, char *argv[])
{
    if ((argc > 0) && (strcmp(argv[0], "reset") == 0) && (bXPNetwork))
    {
        // Mainly meant to make sure an emulated Xport module disconnects and closes the current socket (if there is one)
        // Xport emulator listens for a "enter monitor mode" to close any open connections/sockets
        // TODO: How does a real Xport module do disconnect?
        XPN_EnterMonitorMode();
        waitMs(200);
        XPN_ExitMonitorMode();
    }

    TELNET_Init();
    SetupTerminal();
    PrintOutput("SMDTC Command Interpreter v0.2\nType \"help\" for available commands\n\n>");
}

void ReEnter_Terminal()
{
    SetupTerminal();
    TTY_PrintChar('>');
}

void Exit_Terminal()
{
    TTY_SetFontSize(pFontSize);
}

void Reset_Terminal()
{
    TTY_Reset(TRUE);
}

void Run_Terminal()
{
    while (KB_Poll(&kbdata))
    {
        KB_Interpret_Scancode(kbdata);
    }
}

void Input_Terminal()
{
    if (!bWindowActive)
    {
        if (is_KeyDown(KEY_UP))
        {
            while (DoBackspace());

            PrintOutput(LastCommand);

            for (u16 i = 0; i < strlen(LastCommand); i++) Buffer_Push(&TxBuffer, LastCommand[i]);
        }

        if (is_KeyDown(KEY_DOWN))
        {
            while (DoBackspace());
        }

        if (is_KeyDown(KEY_KP4_LEFT))
        {
            if (!FontSize)
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
            if (!FontSize)
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

            if ((ParseInputString()) && (isCurrentState(PS_Terminal))) TELNET_ParseRX(0xA);
            
            if (isCurrentState(PS_Terminal)) TTY_PrintChar('>');
        }

        if (is_KeyDown(KEY_BACKSPACE))
        {
            DoBackspace();
        }

        if (is_KeyDown(KEY_F9))
        {
            ChangeState(PS_Telnet, 0, NULL);
        }

        if (is_KeyDown(KEY_F10))
        {
            ChangeState(PS_IRC, 0, NULL);
        }

        if (is_KeyDown(KEY_F11))
        {
            ChangeState(PS_Entry, 0, NULL);
        }

        if (is_KeyDown(KEY_F12))
        {
            ChangeState(PS_Debug, 0, NULL);
        }
    }
}

const PRG_State TerminalState = 
{
    Enter_Terminal, ReEnter_Terminal, Exit_Terminal, Reset_Terminal, Run_Terminal, Input_Terminal, NULL, NULL
};

