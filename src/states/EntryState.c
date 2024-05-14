#include "StateCtrl.h"
#include "UI.h"
#include "Input.h"
#include "Terminal.h"
#include "Keyboard.h"
#include "Utils.h"
#include "Buffer.h"
#include "Network.h"
#include "SRAM.h"
#include "IRC.h"

static SM_Window EntryWindow;
static char IPSTR[32];
static u8 pLineMode = 1;
static u8 SelectedIdx = 0;
static u8 SubMenuIdx = 0;
static u8 kbdata;


void UpdateWindow()
{
    UI_Begin(&EntryWindow);

    UI_DrawText( 1, 3 + SelectedIdx, ">");
    UI_DrawText(36, 3 + SelectedIdx, "<");

    switch (SubMenuIdx)
    {
        case 0:
            UI_DrawTextInput(2, 2, 34, "Address", IPSTR, (SelectedIdx == 0));

            UI_DrawText(2,  6, "Launch Telnet Session");
            UI_DrawText(2,  8, "Launch IRC Session");
            UI_DrawText(2, 10, "Launch Terminal Session");
            UI_DrawText(2, 12, "IRC Settings");
        break;

        case 1:
            UI_DrawTextInput(2, 2, 34, "IRC Nick", vUsername, (SelectedIdx == 0));
            UI_DrawTextInput(2, 6, 34, "IRC Exit Message", vQuitStr, (SelectedIdx == 4));
            UI_DrawText(2, 10, "Return");
        break;
    
        default:
        break;
    }

    UI_End();
}

void Enter_Entry(u8 argc, char *argv[])
{
    VDP_setWindowVPos(FALSE, 30);
    TRM_ClearTextArea(0, 0, 35, 1);
    TRM_ClearTextArea(0, 1, 40, 29);
    
    UI_CreateWindow(&EntryWindow, "SMDTC - Startup Menu", UC_NONE);
    
    // Hack to draw window border only once
    UI_Begin(&EntryWindow);
    UI_End();
    EntryWindow.Flags = UC_NOBORDER;

    memset(IPSTR, 0, 32);

    pLineMode = vLineMode;  // Save linemode setting
    vLineMode = 1;          // Causes keyboard input to be buffered

    UpdateWindow();
}

void ReEnter_Entry()
{
}

void Exit_Entry()
{
    VDP_setWindowVPos(FALSE, 1);
    TRM_ClearTextArea(0, 0, 36, 1);

    vLineMode = pLineMode;  // Revert linemode setting

    Buffer_Flush(&TxBuffer);
    Buffer_Flush(&RxBuffer);

    SRAM_SaveData();
}

void Reset_Entry()
{
}

void Run_Entry()
{
    while (KB_Poll(&kbdata))
    {
        KB_Interpret_Scancode(kbdata);

        if ((SelectedIdx == 0) && (SubMenuIdx == 0))        // Address
        {
            Buffer_PeekLast(&TxBuffer, 31, (u8*)IPSTR);
            UI_Begin(&EntryWindow);
            UI_DrawTextInput(2, 2, 34, "Address", IPSTR, (SelectedIdx == 0));
            UI_End();
        }
        else if ((SelectedIdx == 0) && (SubMenuIdx == 1))   // Nick 
        {
            Buffer_PeekLast(&TxBuffer, 31, (u8*)vUsername);
            UI_Begin(&EntryWindow);
            UI_DrawTextInput(2, 2, 34, "IRC Nick", vUsername, (SelectedIdx == 0));
            UI_End();
        }
        else if ((SelectedIdx == 4) && (SubMenuIdx == 1))   // Exit Message 
        {
            Buffer_PeekLast(&TxBuffer, 31, (u8*)vQuitStr);
            UI_Begin(&EntryWindow);
            UI_DrawTextInput(2, 6, 34, "IRC Exit Message", vQuitStr, (SelectedIdx == 4));
            UI_End();
        }
    }
}

void VBlank_Entry()
{
}

void Input_Entry()
{
    if (is_KeyDown(KEY_UP))
    {
        UI_Begin(&EntryWindow);
        UI_DrawText( 1, 3 + SelectedIdx, " ");
        UI_DrawText(36, 3 + SelectedIdx, " ");

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
                    for (u8 i = 0; i < strlen(vUsername); i++) Buffer_Push(&TxBuffer, vUsername[i]);
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
        UI_Begin(&EntryWindow);
        UI_DrawText( 1, 3 + SelectedIdx, " ");
        UI_DrawText(36, 3 + SelectedIdx, " ");

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
                    for (u8 i = 0; i < strlen(vQuitStr); i++) Buffer_Push(&TxBuffer, vQuitStr[i]);
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

            UI_Begin(&EntryWindow);
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
                    ChangeState(PS_Telnet, 1, argv);
                break;

                case 5: // Launch IRC
                    ChangeState(PS_IRC, 1, argv);
                break;

                case 7: // Launch terminal
                    ChangeState(PS_Terminal, 1, argv);
                break;

                case 9: // IRC Settings
                    SubMenuIdx = 1;
                    SelectedIdx = 0;

                    Buffer_Flush(&TxBuffer);                    
                    for (u8 i = 0; i < strlen(vUsername); i++) Buffer_Push(&TxBuffer, vUsername[i]);

                    UI_Begin(&EntryWindow);
                    UI_ClearRect(1, 1, 37, 23);
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

                    UI_Begin(&EntryWindow);
                    UI_ClearRect(1, 1, 37, 23);
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
