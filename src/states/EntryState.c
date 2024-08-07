#include "StateCtrl.h"
#include "UI.h"
#include "Input.h"
#include "Terminal.h"
#include "Utils.h"
#include "Buffer.h"
#include "Network.h"
#include "SRAM.h"
#include "IRC.h"

static SM_Window *EntryWindow;
static char IPSTR[32];
static u8 pLineMode = 1;
static u8 SelectedIdx = 0;
static u8 SubMenuIdx = 0;


static void UpdateWindow()
{
    UI_Begin(EntryWindow);

    UI_DrawText( 1, 3 + SelectedIdx, PAL1, ">");
    UI_DrawText(36, 3 + SelectedIdx, PAL1, "<");

    switch (SubMenuIdx)
    {
        case 0:
            UI_DrawTextInput(2, 2, 34, "Address", IPSTR, (SelectedIdx == 0));

            UI_DrawText(2,  6, PAL1, "Launch Telnet Session");
            UI_DrawText(2,  8, PAL1, "Launch IRC Session");
            UI_DrawText(2, 10, PAL1, "Launch Terminal Session");
            UI_DrawText(2, 12, PAL1, "IRC Settings");
        break;

        case 1:
            UI_DrawTextInput(2, 2, 34, "IRC Nick", sv_Username, (SelectedIdx == 0));
            UI_DrawTextInput(2, 6, 34, "IRC Exit Message", sv_QuitStr, (SelectedIdx == 4));
            UI_DrawText(2, 10, PAL1, "Return");
        break;
    
        default:
        break;
    }

    UI_End();
}

void Enter_Entry(u8 argc, char *argv[])
{
    TRM_SetWinHeight(30);
    TRM_ClearArea(0, 1, 40, 29, PAL1, TRM_CLEAR_WINDOW);

    EntryWindow = malloc(sizeof(SM_Window));
    
    UI_CreateWindow(EntryWindow, "SMDTC Startup Menu", UC_NONE);

    memset(IPSTR, 0, 32);

    pLineMode = vLineMode;  // Save linemode setting
    vLineMode = 1;          // Causes keyboard input to be buffered

    TRM_SetStatusText(STATUS_TEXT);

    UpdateWindow();
}

void ReEnter_Entry()
{
}

void Exit_Entry()
{
    vLineMode = pLineMode;  // Revert linemode setting

    Buffer_Flush(&TxBuffer);
    Buffer_Flush(&RxBuffer);

    SRAM_SaveData();

    free(EntryWindow);

    TRM_SetWinHeight(1);
    TRM_ClearArea(0, 1, 40, 28, PAL1, TRM_CLEAR_BG);
}

void Reset_Entry()
{
}

void Run_Entry()
{
    if ((SelectedIdx == 0) && (SubMenuIdx == 0))        // Address
    {
        Buffer_PeekLast(&TxBuffer, 31, (u8*)IPSTR);
        UI_Begin(EntryWindow);
        UI_DrawTextInput(2, 2, 34, "Address", IPSTR, (SelectedIdx == 0));
        UI_End();
    }
    else if ((SelectedIdx == 0) && (SubMenuIdx == 1))   // Nick 
    {
        Buffer_PeekLast(&TxBuffer, 31, (u8*)sv_Username);
        UI_Begin(EntryWindow);
        UI_DrawTextInput(2, 2, 34, "IRC Nick", sv_Username, (SelectedIdx == 0));
        UI_End();
    }
    else if ((SelectedIdx == 4) && (SubMenuIdx == 1))   // Exit Message 
    {
        Buffer_PeekLast(&TxBuffer, 31, (u8*)sv_QuitStr);
        UI_Begin(EntryWindow);
        UI_DrawTextInput(2, 6, 34, "IRC Exit Message", sv_QuitStr, (SelectedIdx == 4));
        UI_End();
    }
}

void VBlank_Entry()
{
}

void Input_Entry()
{
    if (is_KeyDown(KEY_UP))
    {
        UI_Begin(EntryWindow);
        UI_DrawText( 1, 3 + SelectedIdx, PAL1, " ");
        UI_DrawText(36, 3 + SelectedIdx, PAL1, " ");

        if (SubMenuIdx == 0)
        {
            switch (SelectedIdx)
            {
                case 0: // Address textinput
                case 3: // Launch telnet
                    SelectedIdx = 0;

                    Buffer_Flush(&TxBuffer);                    
                    for (u8 i = 0; i < strlen(IPSTR); i++) Buffer_Push(&TxBuffer, IPSTR[i]);
                break;

                case 5: // Launch IRC
                    SelectedIdx = 3;
                break;

                case 7: // Launch terminal
                    SelectedIdx = 5;
                break;

                case 9: // IRC Settings
                    SelectedIdx = 7;
                break;
            
                default:
                break;
            }
        }
        else if (SubMenuIdx == 1)
        {
            switch (SelectedIdx)
            {
                case 0: // Nick textinput
                case 4: // Exit textinput
                    SelectedIdx = 0;

                    Buffer_Flush(&TxBuffer);
                    for (u8 i = 0; i < strlen(sv_Username); i++) Buffer_Push(&TxBuffer, sv_Username[i]);
                break;

                case 7: // Return
                    SelectedIdx = 4;
                break;
            
                default:
                break;
            }
        }

        UI_EndNoPaint();
        UpdateWindow();
    }

    if (is_KeyDown(KEY_DOWN))
    {
        UI_Begin(EntryWindow);
        UI_DrawText( 1, 3 + SelectedIdx, PAL1, " ");
        UI_DrawText(36, 3 + SelectedIdx, PAL1, " ");

        if (SubMenuIdx == 0)
        {
            switch (SelectedIdx)
            {
                case 0: // Address textinput
                    SelectedIdx = 3;
                break;

                case 3: // Launch telnet
                    SelectedIdx = 5;
                break;

                case 5: // Launch IRC
                    SelectedIdx = 7;
                break;

                case 7: // Launch terminal
                case 9: // IRC Settings
                    SelectedIdx = 9;
                break;
            
                default:
                break;
            }
        }
        else if (SubMenuIdx == 1)
        {
            switch (SelectedIdx)
            {
                case 0: // Nick textinput
                    SelectedIdx = 4;

                    Buffer_Flush(&TxBuffer);
                    for (u8 i = 0; i < strlen(sv_QuitStr); i++) Buffer_Push(&TxBuffer, sv_QuitStr[i]);
                break;

                case 4: // Exit textinput
                case 7: // Return
                    SelectedIdx = 7;
                break;
            
                default:
                break;
            }
        }
        
        UI_EndNoPaint();
        UpdateWindow();
    }

    if (is_KeyDown(KEY_LEFT))
    {
    }

    if (is_KeyDown(KEY_RIGHT))
    {
    }

    if (is_KeyDown(KEY_ESCAPE))
    {
        if (SubMenuIdx == 1)
        {
            SubMenuIdx = 0;
            SelectedIdx = 0;

            Buffer_Flush(&TxBuffer);
            for (u8 i = 0; i < strlen(IPSTR); i++) Buffer_Push(&TxBuffer, IPSTR[i]);

            UI_Begin(EntryWindow);
            UI_ClearRect(1, 1, 37, 23);
            UI_EndNoPaint();

            UpdateWindow();
        }
    }

    if (is_KeyDown(KEY_RETURN))
    {
        char *argv[1] =
        {
            IPSTR
        };

        if (SubMenuIdx == 0)
        {
            switch (SelectedIdx)
            {
                case 0: // Address textinput
                break;

                case 3: // Launch telnet
                    UI_SetVisible(EntryWindow, FALSE);
                    ChangeState(PS_Telnet, 1, argv);
                break;

                case 5: // Launch IRC
                    UI_SetVisible(EntryWindow, FALSE);
                    ChangeState(PS_IRC, 1, argv);
                break;

                case 7: // Launch terminal
                    UI_SetVisible(EntryWindow, FALSE);
                    ChangeState(PS_Terminal, 1, argv);
                break;

                case 9: // IRC Settings
                    SubMenuIdx = 1;
                    SelectedIdx = 0;

                    Buffer_Flush(&TxBuffer);                    
                    for (u8 i = 0; i < strlen(sv_Username); i++) Buffer_Push(&TxBuffer, sv_Username[i]);

                    UI_Begin(EntryWindow);
                    UI_ClearRect(1, 1, 37, 22);
                    UI_EndNoPaint();
                break;
            
                default:
                break;
            }
        }
        else if (SubMenuIdx == 1)
        {
            switch (SelectedIdx)
            {
                case 0: // Nick textinput
                break;

                case 4: // Exit textinput
                break;

                case 7: // Return
                    SubMenuIdx = 0;
                    SelectedIdx = 0;

                    Buffer_Flush(&TxBuffer);
                    for (u8 i = 0; i < strlen(IPSTR); i++) Buffer_Push(&TxBuffer, IPSTR[i]);

                    UI_Begin(EntryWindow);
                    UI_ClearRect(1, 1, 37, 22);
                    UI_EndNoPaint();
                break;
            
                default:
                break;
            }
        }

        UpdateWindow();
    }

    if (is_KeyDown(KEY_BACKSPACE))
    {
        if ((SelectedIdx == 0) || ((SelectedIdx == 4) && (SubMenuIdx == 1))) 
        {
            Buffer_ReversePop(&TxBuffer);
            UpdateWindow();
        }
    }
}

const PRG_State EntryState = 
{
    Enter_Entry, ReEnter_Entry, Exit_Entry, Reset_Entry, Run_Entry, Input_Entry, NULL, VBlank_Entry
};
