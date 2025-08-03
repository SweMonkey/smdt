#include "StateCtrl.h"


u16 Enter_Dummy(u8 argc, char *argv[])
{
    return EXIT_SUCCESS;
}

void ReEnter_Dummy()
{
}

void Exit_Dummy()
{
}

void Reset_Dummy()
{
}

void Run_Dummy()
{
}

void Input_Dummy()
{
}

const PRG_State DummyState = 
{
    Enter_Dummy, ReEnter_Dummy, Exit_Dummy, Reset_Dummy, Run_Dummy, Input_Dummy
};

