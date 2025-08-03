#ifndef STATECTRL_H_INCLUDED
#define STATECTRL_H_INCLUDED

#include <genesis.h>

typedef enum 
{
    PS_Dummy    = 0, 
    PS_Telnet   = 1, 
    PS_Debug    = 3, 
    PS_IRC      = 4, 
    PS_Terminal = 5, 
    PS_Gopher   = 6
} State;

typedef u16 StateArg_CB(u8 argc, char *argv[]);

typedef struct
{
    StateArg_CB *Enter;
    VoidCallback *ReEnter;  // Return to this context
    VoidCallback *Exit;
    VoidCallback *Reset;

    VoidCallback *Run;
    VoidCallback *Input;
} PRG_State;

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

extern bool bWindowActive;

void VBlank();
void ChangeState(State new_state, u8 argc, char *argv[]);
void RevertState();
bool isCurrentState(State this);
State getState();
State getPrevState();
void ResetSystem(bool bHard);
bool StateHasChanged();
void StateTick();

#endif // STATECTRL_H_INCLUDED
