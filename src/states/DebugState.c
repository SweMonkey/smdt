
#include "StateCtrl.h"
#include "main.h"
#include "Input.h"
#include "UI.h"

SM_Window Test;
SM_Menu MenuTest;


void Enter_Debug(u8 argc, const char *argv[])
{
    VDP_setWindowVPos(FALSE, 30);
    TRM_clearTextArea(0, 0, 35, 1);
    TRM_clearTextArea(0, 1, 40, 29);

    kprintf("Testing mode");
    Test.bVisible = TRUE;
    UI_CreateWindow(&Test, "Test window");
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
    UI_Begin(&Test);
    UI_ClearRect(0, 0, 38, 24);

    UI_BeginMenu(&MenuTest);
    UI_MenuItem("Item1", 0, 0);
    UI_MenuItem("Item2", 0, 1);
    UI_MenuItem("Item3", 0, 2);
    UI_MenuItem("Item4", 0, 3);
    UI_EndMenu();

    UI_End();

    //UI_Begin(&Test);
    //UI_ClearRect(0, 0, 38, 24);

    //UI_End();

    /*sprintf(buf, "x: %u - y: %u", x, y);

    UI_Begin(&Test);
    UI_ClearRect(0, 0, 38, 24);
    UI_DrawVLine(4, 0, 24, UC_VLINE_SINGLE);
    UI_DrawVLine(28, 0, 24, UC_VLINE_SINGLE);

    UI_DrawText(14, 0, buf);

    UI_DrawVScrollbar(2, 0, 28, 0, 100, 2);

    UI_End();

    SYS_doVBlankProcess();
    SYS_doVBlankProcess();
    SYS_doVBlankProcess();
    SYS_doVBlankProcess();
    SYS_doVBlankProcess();
    SYS_doVBlankProcess();
    SYS_doVBlankProcess();
    SYS_doVBlankProcess();
    SYS_doVBlankProcess();
    SYS_doVBlankProcess();
    SYS_doVBlankProcess();
    SYS_doVBlankProcess();

    SYS_doVBlankProcess();
    SYS_doVBlankProcess();
    SYS_doVBlankProcess();
    SYS_doVBlankProcess();
    SYS_doVBlankProcess();
    SYS_doVBlankProcess();
    SYS_doVBlankProcess();
    SYS_doVBlankProcess();
    SYS_doVBlankProcess();
    SYS_doVBlankProcess();
    SYS_doVBlankProcess();
    SYS_doVBlankProcess();*/

    //if (x < 24) x++;
}

s8 sidx = 0;
void Input_Debug()
{
    if (is_KeyDown(KEY_UP))
    {
        if (sidx > 0) sidx--;
        else sidx = MenuTest.EntryCnt-1;

        UI_MenuSelect(&MenuTest, sidx);
    }

    if (is_KeyDown(KEY_DOWN))
    {
        if (sidx < MenuTest.EntryCnt-1) sidx++;
        else sidx = 0;

        UI_MenuSelect(&MenuTest, sidx);
    }

    if (is_KeyDown(KEY_RETURN))
    {

    }
}

const PRG_State DebugState = 
{
    Enter_Debug, ReEnter_Debug, Exit_Debug, Reset_Debug, Run_Debug, Input_Debug
};

