
#include "StateCtrl.h"
#include "Utils.h"
#include "Input.h"
#include "UI.h"
#include "SRAM.h"

//SM_Window Test;


void Enter_Debug(u8 argc, const char *argv[])
{
    /*VDP_setWindowVPos(FALSE, 30);
    TRM_clearTextArea(0, 0, 35, 1);
    TRM_clearTextArea(0, 1, 40, 29);

    kprintf("Testing mode");
    Test.bVisible = TRUE;
    UI_CreateWindow(&Test, "Test window", UC_NONE);*/
}

void ReEnter_Debug()
{
}

void Exit_Debug()
{
}

void Reset_Debug()
{
}

void Run_Debug()
{
    //UI_Begin(&Test);
    //UI_ClearRect(0, 0, 38, 24);
    //UI_End();
}

void Input_Debug()
{
    /*if (is_KeyDown(KEY_UP))
    {
    }

    if (is_KeyDown(KEY_DOWN))
    {
    }

    if (is_KeyDown(KEY_RETURN))
    {
    }*/
}

const PRG_State DebugState = 
{
    Enter_Debug, ReEnter_Debug, Exit_Debug, Reset_Debug, Run_Debug, Input_Debug, NULL, NULL
};

