#ifndef WINMGR_H_INCLUDED
#define WINMGR_H_INCLUDED

#include <genesis.h>

typedef enum 
{
    W_QMenu     = 0,
    W_HexView   = 1,
    W_FavView   = 2,
    W_InfoView  = 3
} WinID;

#define WINID_END W_InfoView    // Should always point to the last entry in WinID

void WinMgr_Init();
void WinMgr_Open(WinID winid, u8 argc, char *argv[]);
void WinMgr_Close(WinID winid);
void WinMgr_Input();

bool WinMgr_isWindowOpen();
u8 WinMgr_GetCurrentWinID();

#endif // WINMGR_H_INCLUDED
