#include "FavView.h"
#include "Input.h"
#include "UI.h"
#include "Utils.h"
#include "StateCtrl.h"
#include "Buffer.h"         // Buffer functions
#include "Network.h"        // TxBuffer
#include "Terminal.h"       // vLineMode
#include "system/Stdout.h"

#define ARR_SIZE(a) (sizeof(a)/sizeof(a[0]))

static SM_Window *FavWindow = NULL;
bool bShowFavView = FALSE;
static s16 ScrollY = 0;
static s8 sIdx = -1;    // Selector idx between tab + buttons
static u16 tIdx = 0;    // Selector idx for tabs
static u8 swIdx = 0;    // Selector idx for subwindows
static bool bAdd = FALSE;
static bool bEdit = FALSE;
static char AddrStr[64];
static u8 pLineMode = 1;

static const char *tab_text[3] =
{
    "Telnet", "IRC", "Gopher"
};

static char *list_telnet[] =
{
    "alt.org:23", "telehack.com:23", "bbs.bottomlessabyss.net:2023"
};

static char *list_irc[] =
{
    "chat.freenode.net:6667", "efnet.portlane.se:6667", "irc.rizon.net:6667", "irc.dal.net:6667", "irc.libera.chat:6667", "irc.slashnet.org:6667"
};

static char *list_gopher[] =
{
    "gopher.floodgap.com:70", "quux.org:70"
};

static const u16 num_fav[3] = {ARR_SIZE(list_telnet)-1, ARR_SIZE(list_irc)-1, ARR_SIZE(list_gopher)-1};


void DrawList(u8 Line)
{
}

static void UpdateView()
{
    UI_Begin(FavWindow);

    UI_DrawTabs(0, 0, 38, 3, tIdx, sIdx+1, tab_text);
    UI_ClearRect(0, 2, 38, 18);

    switch (tIdx)
    {
        case 0:
            UI_DrawItemListSelect(0, 0, 38, 18, "", list_telnet, num_fav[0]+1, sIdx, IL_NoBorder);
        break;

        case 1:
            UI_DrawItemListSelect(0, 0, 38, 18, "", list_irc, num_fav[1]+1, sIdx, IL_NoBorder);
        break;

        case 2:
            UI_DrawItemListSelect(0, 0, 38, 18, "", list_gopher, num_fav[2]+1, sIdx, IL_NoBorder);
        break;
    
        default:
        break;
    }

    UI_DrawHLine(0, 20, 40);

    if (bAdd)
    {
        UI_DrawWindow(6, 6, 26, 10, TRUE, "Add bookmark");
        UI_DrawTextInput(7, 9, 24, "Address", AddrStr, swIdx==0?TRUE:FALSE);
        UI_DrawConfirmBox(12, 13, UC_MODEL_OK_CANCEL, swIdx-1);
    }
    else if (bEdit)
    {
        UI_DrawWindow(6, 6, 26, 10, TRUE, "Edit bookmark");
        UI_DrawTextInput(7, 9, 24, "Address", AddrStr, swIdx==0?TRUE:FALSE);
        UI_DrawConfirmBox(12, 13, UC_MODEL_OK_CANCEL, swIdx-1);
    }

    UI_DrawText( 0, 21, PAL1, "[RET] Open");
    UI_DrawText(14, 21, PAL1, "[INS] Add");
    UI_DrawText(29, 21, PAL1, "[F2] Edit");
    UI_DrawText( 0, 22, PAL1, "[ESC] Back");
    UI_DrawText(14, 22, PAL1, "[DEL] Remove");

    UI_End();
}

void FavView_Input()
{
    if (!bShowFavView) return;

    if (bAdd || bEdit)
    {
        Buffer_PeekLast(&TxBuffer, 63, (u8*)AddrStr);

        if (is_KeyDown(KEY_UP))
        {
            swIdx = 0;
            
            UpdateView();
        }

        if (is_KeyDown(KEY_DOWN))
        {
            if (swIdx < 1) swIdx++;   // < 1
            
            UpdateView();
        }
        
        if (is_KeyDown(KEY_LEFT) && (swIdx == 2))
        {
            swIdx = 1;
            UpdateView();
        }

        if (is_KeyDown(KEY_RIGHT) && (swIdx == 1))
        {
            swIdx = 2;
            UpdateView();
        }

        // Back/Cancel
        if (is_KeyDown(KEY_ESCAPE))
        {
            bAdd = FALSE;
            bEdit = FALSE;
            UpdateView();
        }

        if (is_KeyDown(KEY_RETURN))
        {
            // Cancel for both subwindows
            if (swIdx == 2)
            {
                bAdd = FALSE;
                bEdit = FALSE;
                UpdateView();
            }

            // Ok for add subwindow
            if ((swIdx == 1) && bAdd)
            {
                // <Add bookmark here>

                bAdd = FALSE;
                bEdit = FALSE;
                UpdateView();
            }

            // Ok for edit subwindow
            if ((swIdx == 1) && bAdd)
            {
                // <Edit bookmark here>
                
                bAdd = FALSE;
                bEdit = FALSE;
                UpdateView();
            }
        }
    }
    else
    {
        if (is_KeyDown(KEY_UP))
        {
            if (sIdx > -1) sIdx--;
            
            UpdateView();
        }

        if (is_KeyDown(KEY_DOWN))
        {
            if (sIdx < num_fav[tIdx]) sIdx++;   // < 1
            
            UpdateView();
        }

        if (is_KeyDown(KEY_LEFT))
        {
            switch (sIdx)
            {
                case -1: if (tIdx == 0) tIdx = 2; else tIdx--; break;
                default: break;
            }
            UpdateView();
        }

        if (is_KeyDown(KEY_RIGHT))
        {
            switch (sIdx)
            {
                case -1: if (tIdx == 2) tIdx = 0; else tIdx++; break;
                default: break;
            }
            UpdateView();
        }

        // Quick switch tab
        if (is_KeyDown(KEY_TAB))
        {
            if (tIdx < 2) tIdx++;
            else tIdx = 0;

            sIdx = -1;
        }

        // Open
        if (is_KeyDown(KEY_RETURN) && (sIdx >= 0))
        {
            char **argv;

            set_KeyPress(KEY_RETURN, KEYSTATE_NONE);
            FavView_Toggle();
            
            if (getState() != PS_Terminal) RevertState();

            switch (tIdx)
            {
                case 0:
                    argv = malloc(sizeof(char*) * 2);
                    argv[1] = (char*)malloc(strlen(list_telnet[(u8)sIdx])+1);
                    strncpy(argv[1], list_telnet[(u8)sIdx], strlen(list_telnet[(u8)sIdx]));

                    ChangeState(PS_Telnet, 2, argv);

                    free(argv[1]);
                    free(argv);
                break;
                case 1:
                    argv = malloc(sizeof(char*) * 2);
                    argv[1] = (char*)malloc(strlen(list_irc[(u8)sIdx])+1);
                    strncpy(argv[1], list_irc[(u8)sIdx], strlen(list_irc[(u8)sIdx]));

                    ChangeState(PS_IRC, 2, argv);

                    free(argv[1]);
                    free(argv);
                break;
                case 2:
                    argv = malloc(sizeof(char*) * 2);
                    argv[1] = (char*)malloc(strlen(list_gopher[(u8)sIdx])+1);
                    strncpy(argv[1], list_gopher[(u8)sIdx], strlen(list_gopher[(u8)sIdx]));

                    ChangeState(PS_Gopher, 2, argv);

                    free(argv[1]);
                    free(argv);
                break;
            
                default:
                break;
            }
        }

        // Add
        if (is_KeyDown(KEY_INSERT))
        {
            memset(AddrStr, 0, 64);
            bAdd = TRUE;
            swIdx = 0;
            UpdateView();
        }

        // Edit
        if (is_KeyDown(KEY_F2))
        {
            memset(AddrStr, 0, 64);
            //strncpy(AddrStr, list_telnet[sIdx], 64);
            //Stdout_Push(list_telnet[sIdx]);
            bEdit = TRUE;
            swIdx = 0;
            UpdateView();
        }

        // Back/Close
        if (is_KeyDown(KEY_ESCAPE))
        {
            FavView_Toggle();
        }

        // Remove
        if (is_KeyDown(KEY_DELETE))
        {
        }
    }

    return;
}

void DrawFavView()
{
    TRM_SetWinHeight(30);
    TRM_ClearArea(0, 1, 40, 26, PAL1, TRM_CLEAR_BG);  // h=27

    UI_Begin(FavWindow);
    UI_FillRect(0, 27, 40, 2, 0xDE);
    UI_End();

    ScrollY = 0;

    UpdateView();
}

void FavView_Toggle()
{
    if (bShowFavView)
    {
        vLineMode = pLineMode;  // Revert linemode setting

        TRM_SetWinHeight(1);

        // Clear favorite viewer window tiles
        TRM_ClearArea(0, 1, 40, 26 + (bPALSystem?2:0), PAL1, TRM_CLEAR_BG);
        
        // Erase bottom most row tiles (May obscure IRC text input box). 
        // Normally the entire window should be erased by this call, but not all other windows may fill in the erased (black opaque) tiles.
        TRM_ClearArea(0, 27 + (bPALSystem?2:0), 40, 1, PAL1, TRM_CLEAR_INVISIBLE);

        if (FavWindow != NULL)
        {
            free(FavWindow);
            FavWindow = NULL;
        }
        MEM_pack();
    }
    else
    {
        FavWindow = malloc(sizeof(SM_Window));

        if (FavWindow == NULL)
        {
            Stdout_Push("[91mFailed to allocate memory;\nCan't create FavWindow[0m\n");
            bShowFavView = FALSE;
            return;
        }

        sIdx = -1;
        tIdx = 0;
        swIdx = 0;
        bAdd = FALSE;
        bEdit = FALSE;
        pLineMode = vLineMode;  // Save linemode setting
        vLineMode = 1;          // Causes keyboard input to be buffered

        UI_CreateWindow(FavWindow, "Bookmarks - WIP", UC_NONE);
        DrawFavView();
    }

    bShowFavView = !bShowFavView;
}
