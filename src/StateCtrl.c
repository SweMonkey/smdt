#include "StateCtrl.h"
#include "Input.h"
#include "QMenu.h"
#include "HexView.h"
#include "FavView.h"
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
extern PRG_State DebugState;
extern PRG_State TerminalState;
extern PRG_State GopherState;

static PRG_State *CurrentState = &DummyState;
static PRG_State *PrevState = &DummyState;

static State CurrentStateEnum = PS_Dummy;
static State PrevStateEnum = PS_Dummy;

static u16 SC_RetValue = 0;         // Return value from new state init
static bool bStateChanged = FALSE;  // Indicates wheter a state change has occured during the previous frame (needed in case a state change fails and reverts state change back)
static u8 kbdata;
bool bWindowActive = FALSE;


void VBlank()
{
    #ifdef SHOW_FRAME_USAGE
    PAL_setColor(0, 0x00A);
    #endif

    while (KB_Poll(&kbdata))
    {
        KB_Interpret_Scancode(kbdata);
    }
    
    //if (CurrentState->VBlank) CurrentState->VBlank();

    if ((is_KeyDown(KEY_RWIN) || is_KeyDown(KEY_F8)) &&
        (CurrentStateEnum != PS_Debug) && 
        (!bShowHexView) && (!bShowFavView)) 
        {
            QMenu_Toggle();   // Global quick menu
        }

    if (CurrentState->Input != NULL) 
    {   
        CurrentState->Input();
    }

    InputTick();        // Pump IO system
    #ifdef ENABLE_CLOCK
    TickClock();        // Clock will drift when interrupts are disabled!
    #endif
    ScreensaverTick();  // Screensaver counter/animation
    CR_Blink();         // Cursor blink

    bWindowActive = (bShowQMenu || bShowHexView || bShowFavView);

    #ifdef SHOW_FRAME_USAGE
    PAL_setColor(0, 0);
    #endif
}

void ChangeState(State new_state, u8 argc, char *argv[])
{
    if (new_state == CurrentStateEnum) return;
    
    bStateChanged = TRUE;

    PrevState = CurrentState;
    PrevStateEnum = CurrentStateEnum;

    CurrentState->Exit();

    SYS_disableInts();

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

    case PS_Gopher:
    {
        CurrentState = &GopherState;
        break;        
    }
    
    default:
    {
        CurrentState = &DummyState;
        break;
    }
    }

    CurrentStateEnum = new_state;

    TRM_SetStatusText(STATUS_TEXT_SHORT);
    TRM_ResetStatusText();

    SetupQItemTags();
    PAL_setColor(4, sv_CursorCL);

    SYS_setHIntCallback(CurrentState->HBlank);

    SYS_enableInts();

    SYS_setVBlankCallback(NULL);
    SC_RetValue = CurrentState->Enter(argc, argv);
    ScreensaverInit();
    SYS_setVBlankCallback(VBlank);

    if (SC_RetValue == EXIT_FAILURE)
    {
        // Revert back to previous state
        RevertState();
    }
}

// Return to previous state
void RevertState()
{
    PRG_State *ShadowState = CurrentState;
    State ShadowStateEnum = CurrentStateEnum;

    bStateChanged = TRUE;

    CurrentState->Exit();

    SYS_disableInts();

    TRM_SetStatusText(STATUS_TEXT_SHORT);
    TRM_ResetStatusText();

    CurrentState = PrevState;
    CurrentStateEnum = PrevStateEnum;
    PrevState = ShadowState;
    PrevStateEnum = ShadowStateEnum;

    SetupQItemTags();
    PAL_setColor(4, sv_CursorCL);
    SYS_setHIntCallback(CurrentState->HBlank);

    CurrentState->ReEnter();
    ScreensaverInit();

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

State getPrevState()
{
    return PrevStateEnum;
}

void ResetSystem(bool bHard)
{
    CurrentState->Reset();
}

bool StateHasChanged()
{
    return bStateChanged;
}

void StateTick()
{
    #ifdef SHOW_FRAME_USAGE
    PAL_setColor(0, 0xA00);
    #endif

    bStateChanged = FALSE;

    CurrentState->Run();

    if (bRLNetwork)
    {
        RLN_Update();
    }

    #ifdef SHOW_FRAME_USAGE
    PAL_setColor(0, 0x000);
    #endif
}
