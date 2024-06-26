#include "StateCtrl.h"
#include "IRC.h"
#include "Terminal.h"   // sv_Font, sv_HSOffset
#include "Buffer.h"
#include "Input.h"
#include "Utils.h"
#include "UI.h"
#include "Network.h"
#include "devices/RL_Network.h"
#include "SRAM.h"
#include "Screensaver.h"
#include "HexView.h"

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

#ifndef EMU_BUILD
static u8 rxdata;
#endif

static u8 bOnce = FALSE;
static SM_Window UserWin;
static u16 UserListScroll = 0;
static u8 KBTxData[40];            // Buffer for the last 40 typed characters from the keyboard
u8 sv_IRCFont = FONT_4x8_1;

#ifdef EMU_BUILD
#include "kdebug.h"
asm(".global ircdump\nircdump:\n.incbin \"tmp/streams/rx_names.log\"");
extern const unsigned char ircdump[];
#endif

void IRC_PrintChar(u8 c);


void Enter_IRC(u8 argc, char *argv[])
{
    sv_Font = sv_IRCFont;

    IRC_Init();

    TRM_SetWinParam(FALSE, TRUE, 20, 1);
    TRM_SetStatusText(STATUS_TEXT);

    UI_CreateWindow(&UserWin, "", UC_NOBORDER);
    TRM_ClearTextArea(26, 1, 14, 2);    // Clear top 2 tile lines above Channel/User window - can contain leftovers from fullscreen windows

    memset(KBTxData, 0, 40);
    PrintTextLine(KBTxData);    // Calling this here to clear VRAM tiles used for textbox

    #ifdef EMU_BUILD
    u8 data; 
    u32 p = 0;
    u32 s = 24975;//0x252C;//
    KDebug_StartTimer();
    while (p < s)
    {
        while(Buffer_Push(&RxBuffer, ircdump[p]) != 0xFF)
        {
            p++;
            if (bOnce)
            {
                TRM_SetStatusIcon(ICO_NET_RECV, ICO_POS_1);
                bOnce = !bOnce;
            }

            if (p >= s) break;
        }
        
        while (Buffer_Pop(&RxBuffer, &data) != 0xFF)
        {
            IRC_ParseRX(data);
            
            if (!bOnce)
            {
                TRM_SetStatusIcon(ICO_NET_IDLE_RECV, ICO_POS_1);
                bOnce = !bOnce;
            }
        }
        
        TMB_UploadBuffer(PG_Buffer[PG_CurrentIdx]);
    }
    KDebug_StopTimer();
    #endif

    Buffer_Flush(&TxBuffer);
    Buffer_Flush(&RxBuffer);

    if (argc > 1)
    {
        if (NET_Connect(argv[1]) == FALSE) 
        {
            /*char TitleBuf[40];
            sprintf(TitleBuf, "%s - <Connection Error>", STATUS_TEXT);
            TRM_SetStatusText(TitleBuf);*/
        }
    }
}

void ReEnter_IRC()
{
}

void Exit_IRC()
{
    IRC_Exit();

    NET_Disconnect();
}

void Reset_IRC()
{
    IRC_Reset();
}

void Run_IRC()
{
    #ifndef EMU_BUILD
    while (Buffer_Pop(&RxBuffer, &rxdata) != 0xFF)
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

    if (UI_GetVisible(&UserWin) && !bShowHexView)
    {
        TRM_ClearTextArea(26, 1, 14, 2);
        UI_Begin(&UserWin);
        if (PG_UserNum > 0) UI_DrawItemList(25, 0, 14, 25, "User List", PG_UserList, PG_UserNum, UserListScroll);
        else
        {
            //UI_ClearRect(25, 0, 14, 25);
            UI_DrawPanel(25, 0, 14, 25, UC_PANEL_DOUBLE);
            UI_DrawText(26, 10, PAL1, "Requesting");
            UI_DrawText(26, 11, PAL1, "user list.");
            UI_DrawText(26, 13, PAL1, "Please wait");
        }
        UI_End();
    }
}

void ChangePage(u8 num)
{
    char TitleBuf[40];
    PG_CurrentIdx = num;
    //TMB_SetActiveBuffer(PG_Buffer[PG_CurrentIdx]);

    if (PG_CurrentIdx == 0) UI_SetVisible(&UserWin, FALSE);

    sprintf(TitleBuf, "%s - %-21s", STATUS_TEXT, PG_Buffer[PG_CurrentIdx]->Title);
    TRM_SetStatusText(TitleBuf);

    TMB_UploadBufferFull(PG_Buffer[PG_CurrentIdx]);
}

u8 ParseTx()
{
    u8 inbuf[256] = {0};
    u8 outbuf[300] = {0};
    u16 i = 0, j = 0;
    u8 data;

    // Pop the TxBuffer back into inbuf
    while ((Buffer_Pop(&TxBuffer, &data) != 0xFF) && (i < 256))
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
            strncpy(command, (char*)inbuf+end_p, end_c-end_p-1);    // -1 there might be the cause.

            sprintf((char*)outbuf, "PRIVMSG %s :%s\n", command, (char*)inbuf+end_c);
            sprintf((char*)tmbbuf, "%s: %s\n", v_UsernameReset, (char*)inbuf+end_c);

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

                    snprintf(TitleBuf, 40, "%s - %-21s", STATUS_TEXT, PG_Buffer[PG_CurrentIdx]->Title);
                    TRM_SetStatusText(TitleBuf);
                    break;
                }
                else    // No empty pages and no page match, print on overflow page (0)
                {
                    TMB_SetActiveBuffer(PG_Buffer[0]);
                }
            }

            for (u16 c = 0; c < strlen((char*)tmbbuf); c++)
            {
                IRC_PrintChar(tmbbuf[c]);
            }

            TMB_SetActiveBuffer(PG_Buffer[last_pg]);
        }
        else if (strcmp(command, "join") == 0)
        {
            sprintf((char*)outbuf, "JOIN %s\n", param);
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

            // TODO: CHECK RESPONSE, NICK MAY NOT BE VALID!

            SRAM_SaveData();
        } 
        else
        {
            return 1;
        }
    }
    else // Else send as message
    {
        u8 tmbbuf[300] = {0};
        
        if (strlen((char*)inbuf) == 0) return 1;  // Dont send empty messages

        TMB_SetActiveBuffer(PG_Buffer[PG_CurrentIdx]);

        sprintf((char*)outbuf, "PRIVMSG %s :%s\n", PG_Buffer[PG_CurrentIdx]->Title, (char*)inbuf);
        sprintf((char*)tmbbuf, "%s: %s\n", v_UsernameReset, (char*)inbuf);

        for (u16 c = 0; c < strlen((char*)tmbbuf); c++)
        {
            IRC_PrintChar(tmbbuf[c]);
        }        
    }

    while (j < strlen((char*)outbuf))
    {
        Buffer_Push(&TxBuffer, outbuf[j]);
        j++;
    }

    return 0;
}

void Input_IRC()
{
    if (!bWindowActive)
    {
        if (is_KeyDown(KEY_UP))
        {
            if (UserListScroll > 0) UserListScroll--;
        }

        if (is_KeyDown(KEY_DOWN))
        {
            if (UserListScroll < (PG_UserNum-19)) UserListScroll++;
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
        /*if (is_KeyDown(KEY_F4))
        {
            ChangePage(3);
        }*/

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
        if (is_KeyDown(KEY_F5))
        {
            char req[40];
            u16 i = 0;

            UserListScroll = 0;

            if (PG_CurrentIdx != 0)
            {
                UI_ToggleVisible(&UserWin);

                if (UserWin.bVisible)
                {
                    TRM_SetWinWidth(13);

                    snprintf(req, 40, "NAMES %s\n", PG_Buffer[PG_CurrentIdx]->Title);

                    Buffer_Flush(&TxBuffer);

                    while (i < strlen(req))
                    {
                        Buffer_Push(&TxBuffer, req[i]);
                        i++;
                    }

                    NET_TransmitBuffer();
                }
                else TRM_SetWinWidth(20);
            }
        }
    }
}

const PRG_State IRCState = 
{
    Enter_IRC, ReEnter_IRC, Exit_IRC, Reset_IRC, Run_IRC, Input_IRC, NULL, NULL
};

