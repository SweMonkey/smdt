#include "WinMgr.h"
#include "UI.h"
#include "QMenu.h"
#include "windows/HexView.h"
#include "windows/FavView.h"
#include "windows/InfoView.h"

static bool bWindowOpen = FALSE;
static u8 CurrentWinID = 255;

typedef struct
{
    WinID         Id;
    U16Callback  *OpenFunc;
    VoidCallback *CloseFunc;
    VoidCallback *InputFunc;
} WM_Window;

static const WM_Window WList[] =
{
    {W_QMenu,    QMenu_Open,    QMenu_Close,    QMenu_Input},
    {W_HexView,  NULL,          HexView_Close,  HexView_Input},
    {W_FavView,  FavView_Open,  FavView_Close,  FavView_Input},
    {W_InfoView, InfoView_Open, InfoView_Close, InfoView_Input},
};


void WinMgr_Init()
{
    bWindowOpen = FALSE;
    CurrentWinID = 255;
}

void WinMgr_Open(WinID winid, u8 argc, char *argv[])
{
    if (bWindowOpen || (winid > WINID_END)) return;

    u16 ret = 0;

    //ret = WList[winid].OpenFunc();

    switch (winid)
    {
        case W_QMenu:
            QMenu_Open();
        break;

        case W_HexView:
            ret = HexView_Open(argv[0]);
        break;

        case W_FavView:
            ret = FavView_Open();
        break;

        case W_InfoView:
            ret = InfoView_Open();
        break;
    
        default:
        break;
    }

    if (ret)
    {
        return;
    }

    bWindowOpen = TRUE;
    CurrentWinID = winid;
}

void WinMgr_Close(WinID winid)
{
    if (winid > WINID_END) return;

    WList[winid].CloseFunc();

    bWindowOpen = FALSE;
    CurrentWinID = 255;
    UI_EndNoPaint();        // In case of dangling winptr
}

void WinMgr_Input()
{
    if (CurrentWinID > WINID_END) return;

    WList[CurrentWinID].InputFunc();
}

bool WinMgr_isWindowOpen()
{
    return bWindowOpen;
}

u8 WinMgr_GetCurrentWinID()
{
    return CurrentWinID;
}