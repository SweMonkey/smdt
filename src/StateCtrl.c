
#include "StateCtrl.h"
#include "Input.h"
#include "QMenu.h"
#include "HexView.h"
#include "IRQ.h"

extern PRG_State DummyState;
extern PRG_State TelnetState;
extern PRG_State IRCState;
extern PRG_State EntryState;
extern PRG_State DebugState;
extern PRG_State TerminalState;

static PRG_State *CurrentState = &DummyState;
static PRG_State *PrevState = &DummyState;

static State CurrentStateEnum = PS_Dummy;
static State PrevStateEnum = PS_Dummy;
bool bWindowActive = FALSE;


void VBlank()
{
    //if (CurrentState->VBlank) CurrentState->VBlank();

    if (is_KeyDown(KEY_RWIN) && (CurrentStateEnum != PS_Entry) && (!bShowHexView)) QMenu_Toggle();   // Global quick menu

    if (CurrentState->Input != NULL) CurrentState->Input(); // Current PRG

    InputTick();    // Pump IO system
    VB_IRQ();       // Old cursor blink

    bWindowActive = (bShowQMenu || bShowHexView);
}

void ChangeState(State new_state, u8 argc, const char *argv[])
{
    PrevState = CurrentState;
    PrevStateEnum = CurrentStateEnum;

    CurrentState->Exit();

    SYS_disableInts();

    InputTick();    // Flush input queue to prevent inputs "leaking" into new state

    switch (new_state)
    {
    case PS_Dummy:
    {
        CurrentState = &DummyState;
        break;
    }

    case PS_Telnet:
    {
        CurrentState = &TelnetState;
        break;
    }

    case PS_Entry:
    {
        CurrentState = &EntryState;
        break;
    }

    case PS_Debug:
    {
        CurrentState = &DebugState;
        break;
    }

    case PS_IRC:
    {
        CurrentState = &IRCState;
        break;
    }

    case PS_Terminal:
    {
        CurrentState = &TerminalState;
        break;        
    }
    
    default:
    {
        CurrentState = &DummyState;
        break;
    }
    }

    CurrentStateEnum = new_state;

    CurrentState->Enter(argc, argv);
    
    SetupQItemTags();

    SYS_setVBlankCallback(VBlank);
    SYS_setHIntCallback(CurrentState->HBlank);
    SYS_enableInts();
}

// Return to previous state
void RevertState()
{
    PRG_State *ShadowState = CurrentState;

    CurrentState->Exit();

    SYS_disableInts();
    
    CurrentState = PrevState;
    CurrentStateEnum = PrevStateEnum;

    CurrentState->ReEnter();

    PrevState = ShadowState;

    SYS_enableInts();
}

bool isCurrentState(State this)
{
    return CurrentStateEnum == this;
}

State getState()
{
    return CurrentStateEnum;
}

void ResetSystem(bool bHard)
{
    CurrentState->Reset();
}

void StateTick()
{
    CurrentState->Run();
}
