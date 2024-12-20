#ifndef STATECTRL_H_INCLUDED
#define STATECTRL_H_INCLUDED

#include <genesis.h>

typedef enum e_state {PS_Dummy = 0, PS_Telnet = 1, PS_Debug = 3, PS_IRC = 4, PS_Terminal = 5, PS_Gopher = 6} State;

typedef void StateArg_CB(u8 argc, char *argv[]);

typedef struct s_state
{
    StateArg_CB *Enter;
    VoidCallback *ReEnter;  // Return to this context
    VoidCallback *Exit;
    VoidCallback *Reset;

    VoidCallback *Run;
    VoidCallback *Input;
    VoidCallback *HBlank;
    VoidCallback *VBlank;
} PRG_State;

extern bool bWindowActive;

void VBlank();
void ChangeState(State new_state, u8 argc, char *argv[]);
void RevertState();
bool isCurrentState(State this);
State getState();
void ResetSystem(bool bHard);
void StateTick();

#endif // STATECTRL_H_INCLUDED
