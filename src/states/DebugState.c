#include "StateCtrl.h"
#include "UI.h"
#include "Input.h"
#include "Utils.h"
#include "Buffer.h"
#include "Network.h"
#include "system/Stdout.h"
#include "Terminal.h"

static SM_Window *DebugWindow = NULL;
static u16 RGBVal = 0xE80;
static s16 sIdx = 0;
static u16 tIdx = 0;
static char IPSTR[32];
static u8 pLineMode = 1;

static void UpdateWindow()
{
    UI_Begin(DebugWindow);
    
    UI_DrawTabs(0, 0, 38, 5, tIdx, sIdx+1);
    UI_ClearRect(0, 2, 38, 21);

    switch (tIdx)
    {
        case 0:
            UI_DrawColourPicker(0, 2, &RGBVal, sIdx);
            UI_DrawConfirmBox(0, 7, UC_MODEL_APPLY_CANCEL, sIdx-3);
        break;

        case 1:
            UI_DrawTextInput(2, 2, 34, "Text input", IPSTR, (sIdx == 0));
        break;
    
        default:
        break;
    }

    UI_End();
}

void Enter_Debug(u8 argc, char *argv[])
{
    TRM_SetWinHeight(30);
    TRM_ClearArea(0, 1, 40, 29, PAL1, TRM_CLEAR_WINDOW);

    DebugWindow = malloc(sizeof(SM_Window));

    if (DebugWindow == NULL)
    {
        Stdout_Push("[91mFailed to allocate memory\n for DebugWindow![0m\n");
        RevertState();
    }
    
    UI_CreateWindow(DebugWindow, "UI Debugger & Tester", UC_NONE);

    memset(IPSTR, 0, 32);

    pLineMode = vLineMode;  // Save linemode setting
    vLineMode = 1;          // Causes keyboard input to be buffered

    TRM_SetStatusText(STATUS_TEXT);

    UpdateWindow();
}

void ReEnter_Debug()
{
}

void Exit_Debug()
{
    vLineMode = pLineMode;  // Revert linemode setting

    Buffer_Flush(&TxBuffer);
    Buffer_Flush(&RxBuffer);

    free(DebugWindow);

    TRM_SetWinHeight(1);
    TRM_ClearArea(0, 1, 40, 28, PAL1, TRM_CLEAR_BG);
}

void Reset_Debug()
{
}

void Run_Debug()
{
    if ((sIdx == 0) && (tIdx == 1))
    {
        Buffer_PeekLast(&TxBuffer, 31, (u8*)IPSTR);
        UpdateWindow();
    }
}

void Input_Debug()
{
    if (is_KeyDown(KEY_UP))
    {
        //if (sIdx == 0) sIdx = 4; else sIdx--;
        if (sIdx > -1) sIdx--;
        
        UpdateWindow();
    }

    if (is_KeyDown(KEY_DOWN))
    {
        //if (sIdx == 4) sIdx = 0; else sIdx++;
        if (sIdx < 3) sIdx++;
        
        UpdateWindow();
    }

    if (is_KeyDown(KEY_LEFT))
    {
        switch (sIdx)
        {
            case  4: sIdx = 3; break;
            case  2: if ((RGBVal & 0xE00) > 0) RGBVal -= 0x200; break;
            case  1: if ((RGBVal & 0x0E0) > 0) RGBVal -= 0x020; break;
            case  0: if ((RGBVal & 0x00E) > 0) RGBVal -= 0x002; break;
            case -1: if (tIdx == 0) tIdx = 4; else tIdx--; break;
            default: break;
        }
        UpdateWindow();
    }

    if (is_KeyDown(KEY_RIGHT))
    {
        switch (sIdx)
        {
            case 3: sIdx = 4; break;
            case 2: if ((RGBVal & 0xE00) < 0xE00) RGBVal += 0x200; break;
            case 1: if ((RGBVal & 0x0E0) < 0x0E0) RGBVal += 0x020; break;
            case 0: if ((RGBVal & 0x00E) < 0x00E) RGBVal += 0x002; break;
            case -1: if (tIdx == 4) tIdx = 0; else tIdx++; break;
            default: break;
        }
        UpdateWindow();
    }

    if (is_KeyDown(KEY_RETURN))
    {
        if (sIdx == 4) RevertState();

        if (sIdx == 3)
        {
            PAL_setColor(21, RGBVal & 0x666);   // Window shadow
            PAL_setColor(22, RGBVal);           // Window border
        }
    }

    if (is_KeyDown(KEY_ESCAPE))
    {
        RevertState();
    }

    if (is_KeyDown(KEY_BACKSPACE))
    {
        if ((sIdx == 0) && (tIdx == 1)) 
        {
            Buffer_ReversePop(&TxBuffer);
            UpdateWindow();
        }
    }
}

const PRG_State DebugState = 
{
    Enter_Debug, ReEnter_Debug, Exit_Debug, Reset_Debug, Run_Debug, Input_Debug, NULL, NULL
};

