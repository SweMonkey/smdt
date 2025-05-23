#include "StateCtrl.h"
#include "IRC.h"
#include "Terminal.h"   // sv_Font, sv_HSOffset
#include "Buffer.h"
#include "Input.h"
#include "Utils.h"
#include "UI.h"
#include "Network.h"
#include "Screensaver.h"

#include "devices/RL_Network.h"
#include "misc/ConfigFile.h"
#include "system/Stdout.h"

/*
    USERLIST WARNING:

    DO NOT TRY TO DEBUG THE USER LIST WINDOW IN AN EMULATOR. IT WILL MISLEAD AND CAUSE YOU HARM.
    IT WILL REPORT VERY WEIRD BEHAVIOURS WHEN ALLOCATING/FREEING MEMORY AND WHEN TRYING TO SHOW
    THE USERLIST WINDOW.

    IRC CMD 353 (START OF NAMES LIST FOLLOWED BY NICK LIST) AND IRC CMD 366 (END OF NAMES LIST)
    REQUIRES SOME SPECIAL "ASYNCHRONUS" HANDLING, DUE TO BEING RECEIVED AT COMPLETELY DIFFERENT
    TIMES SOME TIME AFTER A "NAMES" COMMAND HAS BEEN SENT TO THE REMOTE SERVER FROM SMDT.

    THIS SEQUENCE OF EVENTS CANNOT BE EMULATED BY REPLAYING A LOGGED STREAM!
*/

static u8 bOnce = FALSE;
static u8 rxdata;
static SM_Window *UserWin = NULL;
static u16 UserListScroll = 0;
static u8 KBTxData[40];            // Buffer for the last 40 typed characters from the keyboard
u8 sv_IRCFont = FONT_4x8_1;

extern bool bShowFavView;
extern bool bShowHexView;

void IRC_PrintChar(u8 c);


u16 Enter_IRC(u8 argc, char *argv[])
{
    sv_Font = sv_IRCFont;

    if (IRC_Init() == EXIT_FAILURE) return EXIT_FAILURE;

    // Set VDP tilemap size to 64 or 128 depending on TMB_SIZE_SELECTOR value
    VDP_setPlaneSize(TMB_TILEMAP_WIDTH, 32, FALSE);

    // Adjust horizontal wrap position depending on font and tilemap size
    if (TMB_TILEMAP_WIDTH == 128)
    {
        if (!sv_Font) C_XMAX = 126; // Wrap at >126
        else C_XMAX = 254;          // Wrap at >254
    }
    else if (TMB_TILEMAP_WIDTH == 64)
    {
        if (!sv_Font) C_XMAX = 62; // Wrap at >62
        else C_XMAX = 126;         // Wrap at >126
    }

    // Setup window plane to accomodate user list window on the right side
    TRM_SetWinParam(FALSE, TRUE, 20, 1);
    TRM_SetStatusText(STATUS_TEXT_SHORT);

    UserWin = malloc(sizeof(SM_Window));
    if (UserWin == NULL)
    {
        Stdout_Push("[91mIRC Client: Failed to allocate memory. Can't create UserWin\n[0m\n");
        return EXIT_FAILURE;
    }
    UI_CreateWindow(UserWin, "", WF_NoBorder);
    UI_SetVisible(UserWin, FALSE);

    memset(KBTxData, 0, 40);
    PrintTextLine(KBTxData);    // Calling this here to clear VRAM tiles used for textbox

    Buffer_Flush(&TxBuffer);
    Buffer_Flush(&RxBuffer);

    if (argc > 1)
    {
        if (NET_Connect(argv[1]) == FALSE) 
        {
            // Connection failed; Inform the user here
        }
    }

    return EXIT_SUCCESS;
}

void ReEnter_IRC()
{
}

void Exit_IRC()
{
    char TitleBuf[40];

    IRC_Exit();

    Buffer_Flush(&TxBuffer);
    
    sprintf(TitleBuf, "%s - Disconnecting...     ", STATUS_TEXT_SHORT);
    TRM_SetStatusText(TitleBuf);

    NET_Disconnect();

    VDP_setPlaneSize(128, 32, FALSE);   // Reset VDP tilemap size back to default (128x32)
    TRM_ClearArea(26, 1, 14, 25, PAL1, TRM_CLEAR_BG); // Clear area where the user list window may have been drawn
    TRM_SetWinParam(FALSE, FALSE, 0, 1);// Restore default window parameters

    if (UserWin != NULL)
    {
        free(UserWin);
        UserWin = NULL;
    }
}

void Reset_IRC()
{
    IRC_Reset();
}

void Run_IRC()
{
    #ifndef EMU_BUILD
    while (Buffer_Pop(&RxBuffer, &rxdata))
    {
        IRC_ParseRX(rxdata);

        if (bOnce)
        {
            TRM_SetStatusIcon(ICO_NET_RECV, ICO_POS_1);
            bOnce = !bOnce;
        }
    }
    #endif

    
    #ifndef EMU_BUILD
    if (!bOnce)
    {
        TRM_SetStatusIcon(ICO_NET_IDLE_RECV, ICO_POS_1);
        bOnce = !bOnce;
    }
    #endif

    TMB_UploadBuffer(PG_Buffer[PG_CurrentIdx]);

    Buffer_PeekLast(&TxBuffer, 38, KBTxData);
    PrintTextLine(KBTxData);

    if (bPG_UpdateUserlist)
    {
        if (UI_GetVisible(UserWin) && !(bShowHexView || bShowFavView))
        {
            if (PG_UserNum > 0)
            {
                UI_Begin(UserWin);
                UI_DrawWindow(25, 0, 14, 26, FALSE, "Nick list");
                UI_ClearRect(26, 3, 12, 22);
                UI_DrawItemList(26, 3, 12, 22, PG_UserList, PG_UserNum, UserListScroll);                
                UI_End();

                bPG_UpdateUserlist = 0;
            }
            else if (bPG_UpdateUserlist == 2)
            {
                UI_Begin(UserWin);
                UI_DrawWindow(25, 0, 14, 26, FALSE, "Nick list");
                UI_ClearRect(26, 3, 12, 22);
                UI_DrawText(26, 11, PAL1, "Requesting");
                UI_DrawText(26, 12, PAL1, "nick list.");
                UI_DrawText(26, 14, PAL1, "Please wait");
                UI_End();

                bPG_UpdateUserlist = 1;
            }
        }
    }
    
    if (bShowHexView || bShowFavView)
    {
        // Do redraw because a window has been opened since last time
        bPG_UpdateUserlist = 2;
    }

    if (bPG_UpdateMessage)
    {
        TRM_DrawChar('1', (35-IRC_MAX_CHANNELS)  , 0, PG_CurrentIdx == 0 ? PAL0 : (bPG_HasNewMessages[0] ? PAL3 : PAL1) );  // 29
        TRM_DrawChar('2', (35-IRC_MAX_CHANNELS)+1, 0, PG_CurrentIdx == 1 ? PAL0 : (bPG_HasNewMessages[1] ? PAL3 : PAL1) );  // 30
        TRM_DrawChar('3', (35-IRC_MAX_CHANNELS)+2, 0, PG_CurrentIdx == 2 ? PAL0 : (bPG_HasNewMessages[2] ? PAL3 : PAL1) );  // 31
        TRM_DrawChar('4', (35-IRC_MAX_CHANNELS)+3, 0, PG_CurrentIdx == 3 ? PAL0 : (bPG_HasNewMessages[3] ? PAL3 : PAL1) );  // 32
        TRM_DrawChar('5', (35-IRC_MAX_CHANNELS)+4, 0, PG_CurrentIdx == 4 ? PAL0 : (bPG_HasNewMessages[4] ? PAL3 : PAL1) );  // 33

        if (IRC_MAX_CHANNELS == 6) TRM_DrawChar('6', (35-IRC_MAX_CHANNELS)+5, 0, PG_CurrentIdx == 5 ? PAL0 : (bPG_HasNewMessages[5] ? PAL3 : PAL1) );  // 34

        bPG_UpdateMessage = FALSE;
    }
}

void ChangePage(u8 num)
{
    if (num >= IRC_MAX_CHANNELS) return;

    char TitleBuf[32];
    PG_CurrentIdx = num;

    if (strcmp(PG_Buffer[PG_CurrentIdx]->Title, PG_EMPTYNAME) == 0)
    {
        snprintf(TitleBuf, 30, "%s %s (%u)                              ", STATUS_TEXT_SHORT, PG_Buffer[PG_CurrentIdx]->Title, num+1);
    }
    else
    {
        snprintf(TitleBuf, 29, "%s %-*s", STATUS_TEXT_SHORT, 27-IRC_MAX_CHANNELS, PG_Buffer[PG_CurrentIdx]->Title);
    }

    bPG_HasNewMessages[num] = FALSE;
    bPG_UpdateMessage = TRUE;

    TRM_SetStatusText(TitleBuf);

    TMB_UploadBufferFull(PG_Buffer[PG_CurrentIdx]);
}

u8 ParseTx()
{
    u8 inbuf[256];
    u8 outbuf[300];
    u16 i = 0, j = 0;
    u8 data;
    NameOffset = 0;

    memset(inbuf, 0, 256);
    memset(outbuf, 0, 300);

    // Pop the TxBuffer back into inbuf
    while (Buffer_Pop(&TxBuffer, &data) && (i < 256))
    {
        inbuf[i] = data;
        i++;
    }

    if (inbuf[0] == '/') // Parse as command
    {
        char command[16];
        char param[64];
        u16 end_c = 1;
        u16 end_p = 1;
        while (inbuf[end_c++] != ' ');
        strncpy(command, (char*)inbuf+1, end_c-2);

        end_p = end_c;

        while (inbuf[end_p++] != 0);
        strncpy(param, (char*)inbuf+end_c, end_p-2);

        // do tolower() on command string here
        tolower_string(command);

        if (strcmp(command, "privmsg") == 0)
        {
            u8 tmbbuf[300] = {0};
            u8 last_pg = PG_CurrentIdx;
            end_p = end_c;
            while (inbuf[end_c++] != ' ');
            strncpy(command, (char*)inbuf+end_p, end_c-end_p-1);

            sprintf((char*)outbuf, "PRIVMSG %s :%s\n", command, (char*)inbuf+end_c);
            sprintf((char*)tmbbuf, "\5%s: \4%s\n", v_UsernameReset, (char*)inbuf+end_c);
            NameOffset = strlen(v_UsernameReset) + 2;

            // Try to find an unused page or an existing one
            for (u8 ch = 0; ch < IRC_MAX_CHANNELS; ch++)
            {
                if (strcmp(PG_Buffer[ch]->Title, command) == 0) // Find the page this message belongs to
                {
                    TMB_SetActiveBuffer(PG_Buffer[ch]);
                    break;
                }
                else if (strcmp(PG_Buffer[ch]->Title, PG_EMPTYNAME) == 0) // See if there is an empty page
                {
                    char TitleBuf[40];
                    strncpy(PG_Buffer[ch]->Title, command, 32);
                    TMB_SetActiveBuffer(PG_Buffer[ch]);

                    snprintf(TitleBuf, 40, "%s %-*s", STATUS_TEXT_SHORT, 27-IRC_MAX_CHANNELS, PG_Buffer[PG_CurrentIdx]->Title);
                    TRM_SetStatusText(TitleBuf);
                    bPG_UpdateMessage = TRUE;
                    break;
                }
                else    // No empty pages and no page match, print on overflow page (0)
                {
                    TMB_SetActiveBuffer(PG_Buffer[0]);
                }
            }

            IRC_PrintString((char*)tmbbuf);

            TMB_SetActiveBuffer(PG_Buffer[last_pg]);
        }
        else if (strcmp(command, "join") == 0)
        {
            sprintf((char*)outbuf, "JOIN %s\n", param);
        }
        else if (strcmp(command, "part") == 0)
        {
            char TitleBuf[40];
            char ChanNameBuf[64];
            u8 last_pg = PG_CurrentIdx;

            if (strlen(param) && (param[0] == '#'))
            {
                strcpy(ChanNameBuf, param);
            }
            else
            {
                if (PG_CurrentIdx == 0) return 1;   // TODO: Disconnect server here instead?

                strcpy(ChanNameBuf, PG_Buffer[PG_CurrentIdx]->Title);
            }

            sprintf((char*)outbuf, "PART %s\n", ChanNameBuf);

            // Try to find an unused page or an existing one
            for (u8 ch = 0; ch < IRC_MAX_CHANNELS; ch++)
            {                
                if (strcmp(PG_Buffer[ch]->Title, ChanNameBuf) == 0) // Find the page this message belongs to
                {
                    TMB_SetActiveBuffer(PG_Buffer[ch]);

                    // Clear this page buffer and reset the title string to empty
                    TMB_ZeroCurrentBuffer();
                    strcpy(PG_Buffer[ch]->Title, PG_EMPTYNAME);

                    // Reset HasNewMessages flag for this channel
                    bPG_HasNewMessages[ch] = FALSE;
                    bPG_UpdateMessage = TRUE;

                    //  Update status text and reupload buffer if this page was actually onscreen at the time of this command
                    if (ch == last_pg)
                    {
                        TMB_UploadBufferFull(PG_Buffer[ch]);

                        // Update status text
                        sprintf(TitleBuf, "%s %s (%u)", STATUS_TEXT_SHORT, PG_Buffer[ch]->Title, ch+1);
                        TRM_SetStatusText(TitleBuf);
                    }

                    break;
                }
            }

            TMB_SetActiveBuffer(PG_Buffer[last_pg]);
        }
        else if (strcmp(command, "raw") == 0)
        {
            sprintf((char*)outbuf, "%s\n", param);
        }
        else if (strcmp(command, "quit") == 0)
        {
            sprintf((char*)outbuf, "QUIT %s\n", sv_QuitStr);
        } 
        else if (strcmp(command, "nick") == 0)
        {
            sprintf((char*)outbuf, "NICK %s\n", param);
            strncpy(sv_Username, param, 31);

            memset(v_UsernameReset, 0, 32);
            strcpy(v_UsernameReset, sv_Username);

            char buf[64];
            sprintf(buf, "NICK %s\n", v_UsernameReset);
            NET_SendString(buf);

            // TODO: CHECK RESPONSE, NICK MAY NOT BE VALID!

            CFG_SaveData();
        }
        else
        {
            return 1;
        }
    }
    else // Else send as message
    {
        u8 tmbbuf[300] = {0};
        
        if ((strlen((char*)inbuf) == 0) ||                                  // Dont send empty messages
            (strcmp(PG_Buffer[PG_CurrentIdx]->Title, PG_EMPTYNAME) == 0))   // Dont send anything to "empty" channel pages
            return 1;

        TMB_SetActiveBuffer(PG_Buffer[PG_CurrentIdx]);

        sprintf((char*)outbuf, "PRIVMSG %s :%s\n", PG_Buffer[PG_CurrentIdx]->Title, (char*)inbuf);
        sprintf((char*)tmbbuf, "\5%s: \4%s\n", v_UsernameReset, (char*)inbuf);
        NameOffset = strlen(v_UsernameReset) + 2;

        IRC_PrintString((char*)tmbbuf);
    }

    #ifndef EMU_BUILD
    while (j < strlen((char*)outbuf))
    {
        Buffer_Push(&TxBuffer, outbuf[j]);
        j++;
    }
    #endif

    return 0;
}

void Input_IRC()
{
    if (bWindowActive) return;
    
    if (is_KeyDown(KEY_UP))
    {
        if (UserListScroll > 0) 
        {
            UserListScroll--;
            
            // Redraw user list window
            bPG_UpdateUserlist = 2;
        }
    }

    if (is_KeyDown(KEY_DOWN))
    {
        if (UserListScroll < (PG_UserNum-22))
        {
            UserListScroll++;
            
            // Redraw user list window
            bPG_UpdateUserlist = 2;
        }
    }

    if (is_KeyDown(KEY_KP4_LEFT))
    {
        if (!sv_Font)
        {
            HScroll += 8;
            VDP_setHorizontalScroll(BG_A, HScroll);
            VDP_setHorizontalScroll(BG_B, HScroll);
        }
        else
        {
            HScroll += 4;
            VDP_setHorizontalScroll(BG_A, (HScroll+4));  // -4
            VDP_setHorizontalScroll(BG_B, (HScroll  ));  // -8
        }
    }

    if (is_KeyDown(KEY_KP6_RIGHT))
    {
        if (!sv_Font)
        {
            HScroll -= 8;
            VDP_setHorizontalScroll(BG_A, HScroll);
            VDP_setHorizontalScroll(BG_B, HScroll);
        }
        else
        {
            HScroll -= 4;
            VDP_setHorizontalScroll(BG_A, (HScroll+4));  // -4
            VDP_setHorizontalScroll(BG_B, (HScroll  ));  // -8
        }
    }

    if (is_KeyDown(KEY_RETURN))
    {
        // Parse user input and send it
        if (ParseTx() == 0) NET_TransmitBuffer();
    }

    if (is_KeyDown(KEY_BACKSPACE))
    {
        Buffer_ReversePop(&TxBuffer);
    }

    if (is_KeyDown(KEY_F1))
    {
        ChangePage(0);
    }
    if (is_KeyDown(KEY_F2))
    {
        ChangePage(1);
    }
    if (is_KeyDown(KEY_F3))
    {
        ChangePage(2);
    }

    #if TMB_SIZE_SELECTOR == 6
    if (is_KeyDown(KEY_F4))
    {
        ChangePage(3);
    }
    if (is_KeyDown(KEY_F5))
    {
        ChangePage(4);
    }
    if (is_KeyDown(KEY_F6))
    {
        ChangePage(5);
    }
    #endif

    if (is_KeyDown(KEY_LEFT))
    {
        if (PG_CurrentIdx > 0) PG_CurrentIdx--;

        ChangePage(PG_CurrentIdx);
    }

    if (is_KeyDown(KEY_RIGHT))
    {
        if (PG_CurrentIdx < IRC_MAX_CHANNELS-1) PG_CurrentIdx++;

        ChangePage(PG_CurrentIdx);
    }

    // Toggle user list window and request NAMES list from remote server, list will be recieved at a later point
    if (is_KeyDown(KEY_TAB))
    {
        char req[40];
        u16 i = 0;

        UserListScroll = 0;
        memset(req, 0, 40);

        TRM_ClearArea(26, 1, 14, 25, PAL1, TRM_CLEAR_BG); // Clear area where the user list window will be drawn

        //if (PG_CurrentIdx != 0)
        {
            UI_ToggleVisible(UserWin);

            if (UserWin->bVisible)
            {
                TRM_SetWinParam(FALSE, TRUE, 13, 1);

                #ifndef EMU_BUILD
                snprintf(req, 40, "NAMES %s\n", PG_Buffer[PG_CurrentIdx]->Title);

                Buffer_Flush(&TxBuffer);

                while (i < strlen(req))
                {
                    Buffer_Push(&TxBuffer, req[i]);
                    i++;
                }

                NET_TransmitBuffer();
                #endif

                bPG_UpdateUserlist = 2;
            }
            else 
            {
                TRM_SetWinParam(FALSE, TRUE, 20, 1);
            }
        }
    }
}

const PRG_State IRCState = 
{
    Enter_IRC, ReEnter_IRC, Exit_IRC, Reset_IRC, Run_IRC, Input_IRC, NULL, NULL
};

