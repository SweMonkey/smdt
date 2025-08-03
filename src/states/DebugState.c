#include "StateCtrl.h"


u16 Enter_Debug(u8 argc, char *argv[])
{
    return EXIT_SUCCESS;
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
}

void Input_Debug()
{
}

const PRG_State DebugState = 
{
    Enter_Debug, ReEnter_Debug, Exit_Debug, Reset_Debug, Run_Debug, Input_Debug
};

