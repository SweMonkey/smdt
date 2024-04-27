#include "StateCtrl.h"
#include "Telnet.h"
#include "Terminal.h"
#include "Buffer.h"
#include "Input.h"
#include "Keyboard.h"
#include "devices/Keyboard_PS2.h"
#include "Utils.h"
#include "Network.h"

#define INPUT_SIZE 96
#define INPUT_SIZE_ARGV 64

#ifndef EMU_BUILD
static u8 kbdata;
#endif

static u8 pFontSize = 0;

void PrintOutput(const char *str);

void CMD_LaunchTelnet(u8 argc, char *argv[]) { ChangeState(PS_Telnet, argc, argv); }
void CMD_LaunchIRC(u8 argc, char *argv[]) { ChangeState(PS_IRC, argc, argv); }
void CMD_LaunchMenu(u8 argc, char *argv[]) { ChangeState(PS_Entry, argc, argv); }

void CMD_Test(u8 argc, char *argv[])
{
    char tmp[32];

    if (argc < 2) return;

    sprintf(tmp, "%s %s\n", argv[0], argv[1]);
    PrintOutput(tmp);
}

void CMD_Echo(u8 argc, char *argv[])
{
    char tmp[64];

    if (argc < 2) return;

    sprintf(tmp, "%s\n", argv[1]);
    PrintOutput(tmp);
}

void CMD_KeyboardSend(u8 argc, char *argv[])
{
    char tmp[32];
    char kbcmd_string[32] = {'\0'};;

    if (argc < 2) 
    {
        PrintOutput("Send command to keyboard\n\nUsage:\n");
        PrintOutput(argv[0]);
        PrintOutput(" <decimal number between 0 and 255>\n");
        return;
    }

    u8 kbcmd = atoi(argv[1]);
    u8 ret = 0;

    sprintf(tmp, "Sending command $%X to keyboard...\n", kbcmd);
    PrintOutput(tmp);
    
    ret = KB_PS2_SendCommand(kbcmd, kbcmd_string);
    PrintOutput(kbcmd_string);
    
    sprintf(tmp, "Recieved byte $%X from keyboard   \n", ret);

    PrintOutput(tmp);
}

void CMD_Help(u8 argc, char *argv[])
{
    char tmp[256];
    sprintf(tmp, "Commands available:\n\
telnet <address:port>\n\
irc <address:port>\n\
menu - Run graphical start menu\n\
echo <string>\n\
kbc <decimal number>\n\
help - This command\n");
    PrintOutput(tmp);
}


static const struct s_cmdlist
{
    const char *id;
    void (*fptr)(u8, char *[]);
} CMDList[] =
{
    {"telnet",  CMD_LaunchTelnet},
    {"irc",     CMD_LaunchIRC},
    {"menu",    CMD_LaunchMenu},
    {"test",    CMD_Test},
    {"echo",    CMD_Echo},
    {"kbc",     CMD_KeyboardSend},
    {"help",    CMD_Help},
    {0, 0}  // Terminator
};


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
        sprintf(tmp, "Command \"%s\" not found...\n", argv[0]);
        PrintOutput(tmp);
        return 1;
    }

    return 0;
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

            if (ParseInputString()) TELNET_ParseRX(0xA);
            
            TTY_PrintChar('>');
        }

        if (is_KeyDown(KEY_BACKSPACE) && !Buffer_IsEmpty(&TxBuffer))
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

