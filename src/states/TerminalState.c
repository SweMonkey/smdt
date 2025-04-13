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
#define INPUT_SIZE_ARGV 64

u8 sv_TerminalFont = FONT_4x8_16;
static char TimeString[9];
static s32 LastTime = 666;
static bool bRunCMD = FALSE;

static char LastCommand[2][INPUT_SIZE] = {'\0'};
static u8 LCPos = 0;   // Last command position


u8 ParseInputString()
{
    u8 inbuf[INPUT_SIZE] = {0};     // Input buffer string
    char *argv[INPUT_SIZE_ARGV];    // Argument list
    int argc = 0;   // Argument count
    u8 data;        // Byte buffer
    u16 i = 0;      // Buffer iterator
    u16 l = 0;      // List position
    u8 ret = 0;

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
        ret = 0;
        goto Exit;
    }

    // Iterate argv0 through the command list and call bound function
    while (CMDList[l].id != 0)
    {
        if (strcmp(argv[0], CMDList[l].id) == 0)
        {
            Stdout_Flush();
            bRunCMD = TRUE;

            CMDList[l].fptr(argc, argv);

            ret = 1;
            goto Exit;
        }

        l++;
    }

    // Or let the user know that the command was not found
    if (strlen(argv[0]) > 0)
    {
        printf("Command \"[36m%s[0m\" not found...\n", argv[0]);
        ret = 1;
        goto Exit;
    }

    Exit:
    for (u8 a = 0; a < argc; a++)
    {
        free(argv[a]);
        argv[a] = NULL;
    }
    MEM_pack();
    bRunCMD = FALSE;
    return ret;
}

u8 DoBackspace()
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

void SetupTerminal()
{
    TTY_SetFontSize(sv_TerminalFont);
    TELNET_Init(TF_Everything);

    // Variable overrides
    vDoEcho = 0;
    vLineMode = LMSM_EDIT;
    vNewlineConv = 1;
    sv_bWrapAround = TRUE;

    //DoTimeSync(sv_TimeServer);

    Buffer_Flush(&TxBuffer);

    MEM_pack();
}

u16 Enter_Terminal(u8 argc, char *argv[])
{
    SetupTerminal();
    Stdout_Push("SMDT Command Interpreter v0.2\n");
    printf("Type \"[32mhelp[0m\" for available commands%s", sv_Font?" - ":"\n");
    Stdout_Push("Press [32mF8[0m for quick menu\n\n");

    printf("%s> ", FS_GetCWD());

    Stdout_Flush();

    return EXIT_SUCCESS;
}

void ReEnter_Terminal()
{
    SetupTerminal();
    Stdout_Flush();

    printf("%s> ", FS_GetCWD());
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
    if (bRunCMD == FALSE) Stdout_Flush();

    #ifdef ENABLE_CLOCK
    if (LastTime != SystemUptime)
    {
        TimeToStr_Time(SystemTime, TimeString);
        TRM_DrawText(TimeString, 27, 0, PAL1);
        LastTime = SystemUptime;
    }
    #endif
}

void Input_Terminal()
{
    if (bWindowActive) return;

    if (is_KeyDown(KEY_UP))
    {
        if (strlen(LastCommand[LCPos]) > 0)
        {
            while (DoBackspace());

            Stdout_Push(LastCommand[LCPos]);

            for (u16 i = 0; i < strlen(LastCommand[LCPos]); i++) Buffer_Push(&TxBuffer, LastCommand[LCPos][i]);
        }

        if ((LCPos == 0) && (strlen(LastCommand[1]) > 0)) LCPos = 1;
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
    }

    if (is_KeyDown(KEY_DELETE))
    {
    }

    // Temp - cant type ^[ in emulator
    /*if (is_KeyDown(KEY_END))
    {
        TTY_PrintChar(0x1B);
        NET_SendChar(0x1B, 0);
    }*/

    if (is_KeyDown(KEY_RETURN) || is_KeyDown(KEY_KP_RETURN))
    {
        Stdout_Push("\n\r");

        if (ParseInputString())
        {
            Stdout_Flush();     // Flush stdout before printing the newline below, otherwise it might get caught up in the "More" prompt
            
            if (isCurrentState(PS_Terminal) && StateHasChanged() == FALSE) 
            {
                Stdout_Push("\n\r");
            }
        }

        if (isCurrentState(PS_Terminal) && StateHasChanged() == FALSE)
        {
            printf("%s> ", FS_GetCWD());
        }
    }

    if (is_KeyDown(KEY_BACKSPACE))
    {
        DoBackspace();
    }

    // ^C special case
    if (is_KeyDown(KEY_C) && bKB_Ctrl)
    {
        Stdout_Flush();
        printf("\n%s> ", FS_GetCWD());
        Buffer_Flush0(&TxBuffer);
    }
}

const PRG_State TerminalState = 
{
    Enter_Terminal, ReEnter_Terminal, Exit_Terminal, Reset_Terminal, Run_Terminal, Input_Terminal, NULL, NULL
};

