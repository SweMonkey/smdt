#include "StateCtrl.h"
#include "Input.h"
#include "QMenu.h"
#include "HexView.h"
#include "Cursor.h"
#include "Screensaver.h"
#include "DevMgr.h"             // bRLNetwork
#include "Keyboard.h"
#include "Utils.h"              // TRM_ResetStatusText

#include "devices/RL_Network.h"
#include "system/Time.h"

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

static u8 kbdata;
bool bWindowActive = FALSE;


void VBlank()
{
    if (bRLNetwork)
    {
        RLN_Update();
    }

    //if (CurrentState->VBlank) CurrentState->VBlank();

    if ((is_KeyDown(KEY_RWIN) || is_KeyDown(KEY_F8)) && (CurrentStateEnum != PS_Entry) && (!bShowHexView)) QMenu_Toggle();   // Global quick menu

    if (CurrentState->Input != NULL) CurrentState->Input(); // Current PRG

    InputTick();    // Pump IO system
    CR_Blink();     // Cursor blink

    bWindowActive = (bShowQMenu || bShowHexView);
    
    ScreensaverTick();

    TickClock();    // Clock will drift when interrupts are disabled!
}

void ChangeState(State new_state, u8 argc, char *argv[])
{
    if (new_state == CurrentStateEnum) return;

    PrevState = CurrentState;
    PrevStateEnum = CurrentStateEnum;

    SYS_disableInts();

    CurrentState->Exit();

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

    TRM_SetStatusText(STATUS_TEXT);
    TRM_ResetStatusText();

    CurrentState->Enter(argc, argv);

    ScreensaverInit();
    SetupQItemTags();
    PAL_setColor(4, sv_CursorCL);

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

    TRM_SetStatusText(STATUS_TEXT);
    TRM_ResetStatusText();
    
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

    while (KB_Poll(&kbdata))
    {
        KB_Interpret_Scancode(kbdata);
    }
}
