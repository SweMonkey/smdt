#include "FavView.h"
#include "Input.h"
#include "UI.h"
#include "Utils.h"
#include "StateCtrl.h"
#include "Buffer.h"         // Buffer functions
#include "Network.h"        // TxBuffer
#include "WinMgr.h"
#include "system/Stdout.h"

static SM_Window *FavWindow = NULL;
static s16 ScrollY = 0;
static s8 sIdx = -1;    // Selector idx between tab + buttons
static u16 tIdx = 0;    // Selector idx for tabs
static u8 swIdx = 0;    // Selector idx for subwindows
static bool bAdd = FALSE;
static bool bEdit = FALSE;
static bool bWarning = FALSE, bAcceptWarn = FALSE;
static bool bChangesMade = FALSE;
static char AddrStr[64];
static u16 OldHead = 0;
static u16 LastNum = 0;

static const char * const tab_text[3] =
{
    "Telnet", "IRC", "Gopher"
};

static char **list_telnet = NULL;
static char **list_irc = NULL;
static char **list_gopher = NULL;
static u16 num_fav[3] = {0, 0, 0};

void CMD_CopyFile(u8 argc, char *argv[]);


void ReadFavList()
{
    SM_File *f;

    f = F_Open("/sram/system/favlist.cfg", FM_RDONLY);

    if (f == NULL)
    {
        // Copy default config from /rom/... and try again
        char *argv[3] = {0, "/rom/cfg/fvlist_default.cfg", "/sram/system/favlist.cfg"};

        CMD_CopyFile(3, argv);

        f = F_Open("/sram/system/favlist.cfg", FM_RDONLY);

        if (f == NULL) 
        {
            // Failed to copy default config file, bail
            return;
        }
    }

    F_Seek(f, 0, SEEK_END);
    u16 size = F_Tell(f);

    u8 *buf = (u8*)malloc(size);

    if (buf)
    {
        u16 p = 0, p_end = 0, num = 0;
        u16 tid = 0, iid = 0, gid = 0;

        memset(buf, 0, size);
        F_Seek(f, 0, SEEK_SET);
        F_Read(buf, size, 1, f);

        char type = buf[0];

        // Read the first type (t, i or g)
             if (type == 't') num_fav[0]++;
        else if (type == 'i') num_fav[1]++;
        else if (type == 'g') num_fav[2]++;

        while (p < size)
        {
            if (buf[p] == '\n') 
            {
                type = buf[p+1];

                // Read the next type (t, i or g)
                     if (type == 't') num_fav[0]++;
                else if (type == 'i') num_fav[1]++;
                else if (type == 'g') num_fav[2]++;
            }
            p++;
        }

        list_telnet = malloc(num_fav[0] * sizeof(char*));
        list_irc    = malloc(num_fav[1] * sizeof(char*));
        list_gopher = malloc(num_fav[2] * sizeof(char*));

        p = 0;

        while (p < size)
        {
            while ((buf[p + p_end++] != '\n') && (p_end < 64));

            switch (buf[p])
            {
                case 't':
                    list_telnet[tid] = malloc(p_end-1);
                    memset(list_telnet[tid], 0, p_end-1);
                    memcpy(list_telnet[tid], buf+p+1, p_end-2);
                    tid++;
                break;
                case 'i':
                    list_irc[iid] = malloc(p_end-1);
                    memset(list_irc[iid], 0, p_end-1);
                    memcpy(list_irc[iid], buf+p+1, p_end-2);
                    iid++;
                break;
                case 'g':
                    list_gopher[gid] = malloc(p_end-1);
                    memset(list_gopher[gid], 0, p_end-1);
                    memcpy(list_gopher[gid], buf+p+1, p_end-2);
                    gid++;
                break;
            
                default:
                //printf("Type: Unknown (%c)\n", buf[p]);
                break;
            }

            p += p_end;
            p_end = 0;
            num++;
        }

        free(buf);
    }

    F_Close(f);
}

void SaveFavList()
{
    if (bChangesMade)
    {
        SM_File *f;

        f = F_Open("/sram/system/favlist.cfg", FM_WRONLY | FM_TRUNC | FM_CREATE);

        if (f != NULL)
        {
            char buf[64];

            for (u16 i = 0; i < 3; i++)
            {
                for (u16 j = 0; j < num_fav[i]; j++)
                {
                    memset(buf, 0, 64);

                    if (i == 0)
                    {
                        snprintf(buf, 64, "t%s\n", list_telnet[j]);
                    }
                    else if (i == 1)
                    {
                        snprintf(buf, 64, "i%s\n", list_irc[j]);
                    }
                    else if (i == 2)
                    {
                        snprintf(buf, 64, "g%s\n", list_gopher[j]);
                    }
                    
                    F_Write(buf, strlen(buf), 1, f);
                }
            }

            F_Close(f);
        }
        else
        {
            //printf("Failed to save bookmark list!\n");
        }
    }

    // Telnet list destruction
    for (u16 i = 0; i < num_fav[0]; i++)
    {
        free(list_telnet[i]);
        list_telnet[i] = NULL;
    }

    free(list_telnet);
    list_telnet = NULL;

    // IRC list destruction
    for (u16 i = 0; i < num_fav[1]; i++)
    {
        free(list_irc[i]);
        list_irc[i] = NULL;
    }

    free(list_irc);
    list_irc = NULL;

    // Gopher list destruction
    for (u16 i = 0; i < num_fav[2]; i++)
    {
        free(list_gopher[i]);
        list_gopher[i] = NULL;
    }

    free(list_gopher);
    list_gopher = NULL;

    // ...
    num_fav[0] = 0;
    num_fav[1] = 0;
    num_fav[2] = 0;
}

static void UpdateView()
{
    UI_Begin(FavWindow);

    UI_DrawTabs(0, 0, 38, 3, tIdx, sIdx+1, tab_text);
    UI_ClearRect(0, 2, 38, 18);

    switch (tIdx)
    {
        case 0:
            UI_DrawItemListSelect(0, 0, 38, 18, "", list_telnet, num_fav[0], sIdx, IL_NoBorder);
        break;

        case 1:
            UI_DrawItemListSelect(0, 0, 38, 18, "", list_irc, num_fav[1], sIdx, IL_NoBorder);
        break;

        case 2:
            UI_DrawItemListSelect(0, 0, 38, 18, "", list_gopher, num_fav[2], sIdx, IL_NoBorder);
        break;
    
        default:
        break;
    }

    UI_DrawHLine(0, 20, 40);

    if (bAdd)
    {
        UI_DrawWindow(6, 6, 26, 10, TRUE, "Add bookmark");
        UI_DrawTextInput(7, 9, 24, "Address", AddrStr, swIdx==0?TRUE:FALSE);
        UI_DrawConfirmBox(12, 13, CM_Ok_Cancel, swIdx-1);
    }
    else if (bEdit)
    {
        UI_DrawWindow(6, 6, 26, 10, TRUE, "Edit bookmark");
        UI_DrawTextInput(7, 9, 24, "Address", AddrStr, swIdx==0?TRUE:FALSE);
        UI_DrawConfirmBox(12, 13, CM_Ok_Cancel, swIdx-1);
    }
    else if (bWarning)
    {
        UI_DrawWindow(1, 5, 36, 12, TRUE, "Warning");
        UI_DrawText(2, 9, PAL1, "The gopher client is a broken mess");
        UI_DrawText(2, 10, PAL1, "and will cause system crashes!");
        UI_DrawText(2, 12, PAL1, "Are you sure you want to continue?");
        UI_DrawConfirmBox(14, 15, CM_Yes_No, swIdx);
    }

    UI_DrawText( 0, 21, PAL1, "[RET] Open");
    UI_DrawText(14, 21, PAL1, "[INS] Add");
    UI_DrawText(29, 21, PAL1, "[F2] Edit");
    UI_DrawText( 0, 22, PAL1, "[ESC] Back");
    UI_DrawText(14, 22, PAL1, "[DEL] Remove");

    UI_End();
}

static u8 DoBackspace()
{
    if (Buffer_IsEmpty(&TxBuffer)) return 0;

    Buffer_ReversePop(&TxBuffer);

    return 1;
}

void FavView_Input()
{
    if (bWarning)
    {
        if (is_KeyDown(KEY_LEFT) && (swIdx == 1))
        {
            swIdx = 0;
            UpdateView();
        }

        if (is_KeyDown(KEY_RIGHT) && (swIdx == 0))
        {
            swIdx = 1;
            UpdateView();
        }

        if (is_KeyDown(KEY_RETURN))
        {
            // No
            if (swIdx)
            {
                bWarning = FALSE;
                bAcceptWarn = FALSE;
                UpdateView();
            }
            // Yes
            else
            {
                bWarning = FALSE;
                bAcceptWarn = TRUE;
                goto UglyHack;
            }
        }
    }
    else if (bAdd || bEdit)
    {
        u16 num = Buffer_GetNum(&TxBuffer);

        if (num != LastNum)
        {
            LastNum = num;
            Buffer_PeekLast(&TxBuffer, 63, (u8*)AddrStr);
            UpdateView();
        }

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
            TxBuffer.head = OldHead;
            UpdateView();
        }

        if (is_KeyDown(KEY_RETURN))
        {
            // Cancel for both subwindows
            if (swIdx == 2)
            {
                TxBuffer.head = OldHead;
                bAdd = FALSE;
                bEdit = FALSE;
                UpdateView();
            }

            // Ok for add subwindow
            if ((swIdx == 1) && bAdd)
            {
                if (strlen(AddrStr) > 0)
                {
                    switch (tIdx)
                    {
                        case 0:
                        {
                            char **nlist = realloc(list_telnet, (num_fav[0] * sizeof(char*)), ((num_fav[0]+1) * sizeof(char*)));
                            if (nlist == NULL) break;

                            list_telnet = nlist;
                            list_telnet[num_fav[0]] = malloc(strlen(AddrStr)+1);
                            strcpy(list_telnet[num_fav[0]], AddrStr);
                            num_fav[0]++;
                            break;
                        }
                        case 1:
                        {
                            char **nlist = realloc(list_irc, (num_fav[1] * sizeof(char*)), ((num_fav[1]+1) * sizeof(char*)));
                            if (nlist == NULL) break;

                            list_irc = nlist;
                            list_irc[num_fav[1]] = malloc(strlen(AddrStr)+1);
                            strcpy(list_irc[num_fav[1]], AddrStr);
                            num_fav[1]++;
                            break;
                        }
                        case 2:
                        {
                            char **nlist = realloc(list_gopher, (num_fav[2] * sizeof(char*)), ((num_fav[2]+1) * sizeof(char*)));
                            if (nlist == NULL) break;

                            list_gopher = nlist;
                            list_gopher[num_fav[2]] = malloc(strlen(AddrStr)+1);
                            strcpy(list_gopher[num_fav[2]], AddrStr);
                            num_fav[2]++;
                            break;
                        }
                        default: break;
                    }
                }

                TxBuffer.head = OldHead;
                bAdd = FALSE;
                bEdit = FALSE;
                bChangesMade = TRUE;
                UpdateView();
            }

            // Ok for edit subwindow
            if ((swIdx == 1) && bEdit)
            {                
                u8 n = (sIdx >= num_fav[tIdx]) ? num_fav[tIdx]-1 : sIdx;

                switch (tIdx)
                {
                    case 0:
                    {
                        free(list_telnet[n]);
                        list_telnet[n] = malloc(strlen(AddrStr)+1);
                        strcpy(list_telnet[n], AddrStr);
                        break;
                    }
                    case 1:
                    {
                        free(list_irc[n]);
                        list_irc[n] = malloc(strlen(AddrStr)+1);
                        strcpy(list_irc[n], AddrStr);
                        break;
                    }
                    case 2:
                    {
                        free(list_gopher[n]);
                        list_gopher[n] = malloc(strlen(AddrStr)+1);
                        strcpy(list_gopher[n], AddrStr);
                        break;
                    }
                    default: break;
                }
                
                TxBuffer.head = OldHead;                                
                bAdd = FALSE;
                bEdit = FALSE;
                bChangesMade = TRUE;
                UpdateView();
            }
        }

        
        if (is_KeyDown(KEY_BACKSPACE))
        {
            DoBackspace();
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
            if (num_fav[tIdx] > 0)
            {
                if (sIdx < num_fav[tIdx]-1) sIdx++;   // < 1
                
                UpdateView();
            }
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
            UpdateView();
        }

        // Open
        if (is_KeyDown(KEY_RETURN) && (sIdx >= 0))
        {
            if ((tIdx == 2) && (bAcceptWarn == FALSE))
            {
                bWarning = TRUE;
                swIdx = 1;
                UpdateView();
            }
            else
            {
                UglyHack:
                set_KeyPress(KEY_RETURN, KEYSTATE_NONE);            
                if (getState() != PS_Terminal) RevertState();

                switch (tIdx)
                {
                    case 0:
                    {
                        char *argv[2] = {0, list_telnet[(u8)sIdx]};
                        ChangeState(PS_Telnet, 2, argv);
                        break;
                    }
                    case 1:
                    {
                        char *argv[2] = {0, list_irc[(u8)sIdx]};
                        ChangeState(PS_IRC, 2, argv);
                        break;
                    }
                    case 2:
                    {
                        char *argv[2] = {0, list_gopher[(u8)sIdx]};
                        ChangeState(PS_Gopher, 2, argv);
                        break;
                    }
                
                    default:
                    break;
                }
                WinMgr_Close(W_FavView);
            }
        }

        // Add
        if (is_KeyDown(KEY_INSERT))
        {
            //LastNum = Buffer_GetNum(&TxBuffer);
            Buffer_Flush(&TxBuffer);
            OldHead = TxBuffer.head;
            memset(AddrStr, 0, 64);
            bAdd = TRUE;
            swIdx = 0;
            UpdateView();
        }

        // Edit
        if (is_KeyDown(KEY_F2) && (sIdx >= 0))
        {
            u8 n = (sIdx >= num_fav[tIdx]) ? num_fav[tIdx]-1 : sIdx;
            
            Buffer_Flush(&TxBuffer);
            OldHead = TxBuffer.head;
            memset(AddrStr, 0, 64);

            switch (tIdx)
            {
                case 0: Buffer_PushString(&TxBuffer, list_telnet[n]); break;
                case 1: Buffer_PushString(&TxBuffer, list_irc[n]); break;
                case 2: Buffer_PushString(&TxBuffer, list_gopher[n]); break;
                default: break;
            }

            Buffer_PeekLast(&TxBuffer, 63, (u8*)AddrStr);

            bEdit = TRUE;
            swIdx = 0;
            UpdateView();
        }

        // Remove
        if (is_KeyDown(KEY_DELETE) && (sIdx >= 0))
        {
            u8 n = (sIdx >= num_fav[tIdx]) ? num_fav[tIdx]-1 : sIdx;

            switch (tIdx)
            {
                case 0:
                {
                    free(list_telnet[n]);

                    for (int i = n; i < num_fav[0] - 1; ++i) 
                    {
                        list_telnet[i] = list_telnet[i + 1];
                    }

                    char **nlist = realloc(list_telnet, (num_fav[0] * sizeof(char*)), ((num_fav[0] - 1) * sizeof(char*)));
                    if (nlist == NULL && num_fav[0] > 1) break;

                    list_telnet = nlist;
                    num_fav[0]--;
                    bChangesMade = TRUE;
                    break;
                }
                case 1:
                {
                    free(list_irc[n]);

                    for (int i = n; i < num_fav[1] - 1; ++i) 
                    {
                        list_irc[i] = list_irc[i + 1];
                    }

                    char **nlist = realloc(list_irc, (num_fav[1] * sizeof(char*)), ((num_fav[1] - 1) * sizeof(char*)));
                    if (nlist == NULL && num_fav[1] > 1) break;

                    list_irc = nlist;
                    num_fav[1]--;
                    bChangesMade = TRUE;
                    break;
                }
                case 2:
                {
                    free(list_gopher[n]);

                    for (int i = n; i < num_fav[2] - 1; ++i) 
                    {
                        list_gopher[i] = list_gopher[i + 1];
                    }

                    char **nlist = realloc(list_gopher, (num_fav[2] * sizeof(char*)), ((num_fav[2] - 1) * sizeof(char*)));
                    if (nlist == NULL && num_fav[2] > 1) break;

                    list_gopher = nlist;
                    num_fav[2]--;
                    bChangesMade = TRUE;
                    break;
                }
            
                default:
                break;
            }

            sIdx = (sIdx >= num_fav[tIdx]) ? num_fav[tIdx]-1 : sIdx;    // Make sure the item selector is not invalid

            UpdateView();
        }

        // Back/Close
        if (is_KeyDown(KEY_ESCAPE))
        {
            WinMgr_Close(W_FavView);
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

u16 FavView_Open()
{
    FavWindow = malloc(sizeof(SM_Window));

    if (FavWindow == NULL)
    {
        Stdout_Push("[91mFailed to allocate memory;\nCan't create FavWindow[0m\n");
        return 1;
    }

    sIdx = -1;
    tIdx = 0;
    swIdx = 0;
    bAdd = FALSE;
    bEdit = FALSE;
    bWarning = FALSE;
    bAcceptWarn = FALSE;
    bChangesMade = FALSE;

    ReadFavList();

    UI_CreateWindow(FavWindow, "Bookmarks", WF_None);
    DrawFavView();

    return 0;
}

void FavView_Close()
{
    SaveFavList();

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
