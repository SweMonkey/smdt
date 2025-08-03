#include "StateCtrl.h"
#include "Telnet.h"
#include "Terminal.h"
#include "Buffer.h"
#include "Input.h"
#include "Utils.h"
#include "Network.h"
#include "Keyboard.h"       // bKB_Ctrl
#include "misc/CMDFunc.h"

#include "system/Stdout.h"
#include "system/Time.h"
#include "system/Filesystem.h"

#define INPUT_SIZE 128
#define INPUT_SIZE_ARGV 32

u8 sv_TerminalFont = FONT_4x8_16;
u8 ClockLastTime = 60;
static char TimeString[9];
static bool bRunningCMD = FALSE;

static char LastCommand[2][INPUT_SIZE] = {'\0'};
static u8 LCPos = 0;   // Last command position

char *argv[INPUT_SIZE_ARGV]; // Argument list
int argc = 0;                // Argument count


static void ClearArgv()
{
    for (u8 a = 0; a < INPUT_SIZE_ARGV; a++)
    {
        free(argv[a]);
        argv[a] = NULL;
    }
    argc = 0;
}

void PrintCWD()
{
    printf("%s> ", FS_GetCWD());
    Stdout_Flush();
}

static void RunCommand()
{
    u16 l = 0;      // List position

    // Iterate argv0 through the command list and call bound function
    while (CMDList[l].id != 0)
    {
        if (strcmp(argv[0], CMDList[l].id) == 0)
        {
            bRunningCMD = TRUE;
            CMDList[l].fptr(argc, argv);
            bRunningCMD = FALSE;
            Stdout_Flush();
            goto Exit;
        }

        l++;
    }

    // Or let the user know that the command was not found
    if (strlen(argv[0]) > 0)
    {
        printf("Command \"[36m%s[0m\" not found...\n", argv[0]);
    }

    Exit:

    if (isCurrentState(PS_Terminal) && StateHasChanged() == FALSE)
    {
        Stdout_PushByte('\n');
        PrintCWD();
    }

    Stdout_Flush();
    ClearArgv();
    MEM_pack();
    return;
}

static u8 ParseInputString()
{
    u8 inbuf[INPUT_SIZE] = {0};     // Input buffer string
    u8 data;        // Byte buffer
    u16 i = 0;      // Buffer iterator

    memset(inbuf, 0, INPUT_SIZE);

    // Pop the TxBuffer back into inbuf
    while (Buffer_Pop(&TxBuffer, &data) && (i < INPUT_SIZE-1))
    {
        inbuf[i] = data;
        i++;
    }

    // Clear TxBuffer input
    Buffer_Flush0(&TxBuffer);

    // Copy the current input string into last command string
    if (strlen((char*)inbuf) > 0) 
    {
        // Only push the history up the queue if the new command is unique.
        // This is to avoid filling the history queue when re-running the same command.
        if (strcmp(LastCommand[0], (char*)inbuf))
        {
            strncpy(LastCommand[1], LastCommand[0], INPUT_SIZE);
        }

        strncpy(LastCommand[0], (char*)inbuf, INPUT_SIZE);
        LCPos = 0;
    }

    // Extract argument list from input buffer string
    char *p = strtok((char*)inbuf, ' ');
    while (p != NULL)
    {
        argv[argc++] = p;
        p = strtok(NULL, ' ');
    }

    // Filter out Ctrl^ sequences
    if (argv[0][0] < ' ')
    {
        ClearArgv();
        return 1;
    }

    return 0;
}

static u8 DoBackspace()
{
    if (Buffer_IsEmpty(&TxBuffer)) return 0;

    TTY_MoveCursor(TTY_CURSOR_LEFT, 1);

    if (!sv_Font)
    {
        VDP_setTileMapXY(BG_B, TILE_ATTR_FULL(2, 0, 0, 0, 0), TTY_GetSX()+BufferSelect, sy);
    }
    else
    {
        VDP_setTileMapXY(!(TTY_GetSX() & 1), TILE_ATTR_FULL(2, 0, 0, 0, 0), (TTY_GetSX()+BufferSelect)>>1, sy);
    }

    Buffer_ReversePop(&TxBuffer);

    return 1;
}

static void DrawClockUpdate()
{
    TimeToStr_TimeNoSec(&SystemTime, TimeString);
    TRM_DrawText(TimeString, 30, 0, PAL1);
    ClockLastTime = SystemTime.minute;
}

static void SetupTerminal()
{
    TTY_SetFontSize(sv_TerminalFont);
    TELNET_Init(TF_Everything);

    // Variable overrides (TTY/Telnet)
    vDoEcho = 0;
    vLineMode = LMSM_EDIT;
    vNewlineConv = 1;
    sv_bWrapAround = TRUE;

    // Shell
    bRunningCMD = FALSE;

    //DoTimeSync(sv_TimeServer);
    DrawClockUpdate();

    Buffer_Flush(&TxBuffer);
    Buffer_Flush(&RxBuffer);
    ClearArgv();
    MEM_pack();
}

u16 Enter_Terminal(u8 argc, char *argv[])
{
    SetupTerminal();
    Stdout_Push("SMDT Command Shell v0.3\n");
    printf("Type [32mhelp[0m for available commands%s", sv_Font?" - ":"\n");
    Stdout_Push("Press [32mF8[0m for quick menu\n\n");

    PrintCWD();

    return EXIT_SUCCESS;
}

void ReEnter_Terminal()
{
    SetupTerminal();
    PrintCWD();
}

void Exit_Terminal()
{
    Telnet_Quit();
}

void Reset_Terminal()
{
    TTY_Init(TF_ClearScreen);
}

void Run_Terminal()
{
    if ((argc > 0) && (bRunningCMD == FALSE))
    {
        RunCommand();
    }

    #ifdef ENABLE_CLOCK
    if (ClockLastTime != SystemTime.minute)
    {
        DrawClockUpdate();
    }
    #endif
}

void Input_Terminal()
{
    if (bWindowActive || bRunningCMD) return;

    if (is_KeyDown(KEY_UP))
    {
        if (strlen(LastCommand[LCPos]) > 0)
        {
            while (DoBackspace());

            Stdout_Push(LastCommand[LCPos]);

            for (u16 i = 0; i < strlen(LastCommand[LCPos]); i++) Buffer_Push(&TxBuffer, LastCommand[LCPos][i]);
        }

        if ((LCPos == 0) && (strlen(LastCommand[1]) > 0)) LCPos = 1;

        Stdout_Flush();
    }

    if (is_KeyDown(KEY_DOWN))
    {
        while (DoBackspace());

        if (LCPos == 1) 
        {
            LCPos = 0;

            Stdout_Push(LastCommand[LCPos]);

            for (u16 i = 0; i < strlen(LastCommand[LCPos]); i++) Buffer_Push(&TxBuffer, LastCommand[LCPos][i]);
        }

        Stdout_Flush();
    }

    if (is_KeyDown(KEY_DELETE))
    {
    }

    if (is_KeyDown(KEY_BACKSPACE))
    {
        DoBackspace();
    }

    // ^C special case
    if (is_KeyDown(KEY_C) && bKB_Ctrl)
    {
        printf("\n%s> ", FS_GetCWD());
        Stdout_Flush();
        Buffer_Flush0(&TxBuffer);
    }

    // Temp - cant type ^[ in emulator
    /*if (is_KeyDown(KEY_END))
    {
        TTY_PrintChar(0x1B);
        NET_SendChar(0x1B, 0);
    }*/

    if (is_KeyDown(KEY_RETURN) || is_KeyDown(KEY_KP_RETURN))
    {
        printf("\n\r");
        if (ParseInputString() == 1)
        {
            PrintCWD();
        }
        Stdout_Flush();
    }
}

const PRG_State TerminalState = 
{
    Enter_Terminal, ReEnter_Terminal, Exit_Terminal, Reset_Terminal, Run_Terminal, Input_Terminal
};
