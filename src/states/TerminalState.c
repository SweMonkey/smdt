#include "StateCtrl.h"
#include "Telnet.h"
#include "Terminal.h"
#include "Buffer.h"
#include "Input.h"
#include "devices/Keyboard_PS2.h"
#include "Utils.h"
#include "Network.h"

#define INPUT_SIZE 96
#define INPUT_COMMAND 32
#define INPUT_PARAM 64

#ifndef EMU_BUILD
static u8 kbdata;
#endif

static u8 pFontSize = 0;


void PrintOutput(const char *str)
{
    for (u16 c = 0; c < strlen(str); c++)
    {
        TELNET_ParseRX((u8)str[c]);
    }
}

u8 ParseInputString()
{
    u8 inbuf[INPUT_SIZE] = {0};
    char command[INPUT_COMMAND] = {0};
    char param[INPUT_PARAM] = {0};
    u16 i = 0;
    u8 data;
    u16 end_c = 0;
    u16 end_p = 0;

    memset(inbuf, 0, INPUT_SIZE);
    memset(command, 0, INPUT_COMMAND);
    memset(param, 0, INPUT_PARAM);

    // Pop the TxBuffer back into inbuf
    while ((Buffer_Pop(&TxBuffer, &data) != 0xFF) && (i < INPUT_SIZE))
    {
        inbuf[i] = data;
        i++;
    }

    Buffer_Flush(&TxBuffer);

    while ((inbuf[end_c] != ' ') && (inbuf[end_c++] != 0));
    strncpy(command, (char*)inbuf, end_c);

    end_p = end_c;

    if (strlen((char*)inbuf) > end_p)
    {
        while (inbuf[end_p++] != 0);
        strncpy(param, (char*)inbuf+end_c+1, end_p-1);
    }

    //tolower_string(command);
    TELNET_ParseRX(0x0A);

    if (strcmp(command, "telnet") == 0)
    {
        char *argv[1] =
        {
            param
        };
        ChangeState(PS_Telnet, 1, argv);
        return 0;
    }
    else if (strcmp(command, "irc") == 0)
    {
        char *argv[1] =
        {
            param
        };
        ChangeState(PS_IRC, 1, argv);
        return 0;
    }
    else if (strcmp(command, "menu") == 0)
    {
        ChangeState(PS_Entry, 0, NULL);
        return 0;
    }
    else if (strcmp(command, "test") == 0)
    {
        char tmp[32];
        sprintf(tmp, "%s %s\n", command, param);
        PrintOutput(tmp);
    }
    else if (strcmp(command, "echo") == 0)
    {
        char tmp[32];
        sprintf(tmp, "%s\n", param);
        PrintOutput(tmp);
    }
    else if (strcmp(command, "help") == 0)
    {
        char tmp[256];
        sprintf(tmp, "Commands available:\ntelnet <address:port>\nirc <address:port>\nmenu - Run graphical start menu\necho <string>\nhelp - This command\n");
        PrintOutput(tmp);
    }
    else if (strlen(command) > 0)
    {
        char tmp[64];
        sprintf(tmp, "Command \"%s\" not found...\n", command);
        PrintOutput(tmp);
    }
    else
    {
        //TELNET_ParseRX(0x0A);
    }    

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
}

void Enter_Terminal(u8 argc, char *argv[])
{
    TELNET_Init();
    SetupTerminal();
    PrintOutput("SMDTC Command Interpreter v0.1\nType \"help\" for available commands\n\n>");
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
    #ifndef EMU_BUILD
    while (KB_Poll(&kbdata))
    {
        KB_Interpret_Scancode(kbdata);
    }
    #endif
}

void Input_Terminal()
{
    if (!bWindowActive)
    {
        if (is_KeyDown(KEY_UP))
        {
            #ifdef EMU_BUILD
            Buffer_Push(&TxBuffer, 't');
            Buffer_Push(&TxBuffer, 'e');
            Buffer_Push(&TxBuffer, 'l');
            Buffer_Push(&TxBuffer, 'n');
            Buffer_Push(&TxBuffer, 'e');
            Buffer_Push(&TxBuffer, 't');
            #endif
        }

        if (is_KeyDown(KEY_DOWN))
        {
            #ifdef EMU_BUILD
            if (ParseInputString()) TTY_PrintChar('>');
            #endif
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

            if (ParseInputString()) TTY_PrintChar('>');
        }

        if (is_KeyDown(KEY_BACKSPACE))
        {
            TTY_MoveCursor(TTY_CURSOR_LEFT, 1);

            if (!FontSize)
            {
                VDP_setTileMapXY(BG_B, TILE_ATTR_FULL(2, 0, 0, 0, 0), TTY_GetSX(), sy);
            }
            else
            {
                VDP_setTileMapXY(!(TTY_GetSX() % 2), TILE_ATTR_FULL(2, 0, 0, 0, 0), TTY_GetSX()>>1, sy);
            }

            Buffer_ReversePop(&TxBuffer);
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

