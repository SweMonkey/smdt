#include "IRC.h"
#include "Terminal.h"
#include "UTF8.h"
#include "Utils.h"
#include "Buffer.h"
#include "Network.h"
#include "Cursor.h"
#include "StateCtrl.h"
#include "../res/system.h"
#include "misc/Stdout.h"

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

#define B_PRINTSTR_LEN 512
#define B_SUBPREFIX_LEN 256

#define B_PREFIX_LEN 256
#define B_COMMAND_LEN 64
#define B_PARAM_LEN 512

#define B_RXSTRING_LEN 1024

#define TTS_VRAMIDX 0x240    // TODO: find a spot for all the tiles. It requires at least 38 continous tiles in VRAM

static struct s_linebuf
{
    char prefix[B_PREFIX_LEN];
    char command[B_COMMAND_LEN];
    char param[16][B_PARAM_LEN];
} *LineBuf = NULL;

static bool CR_Set = FALSE;
static u8 bLookingForCL = 0;
static u8 NewColor = 0;

static char *RXString = NULL;
static u16 RXStringSeq = 0;

static bool bFirstRun = TRUE;
static u8 NickReRegisterCount = 0;

char sv_Username[32] = "smd_user";                  // Saved preferred IRC nickname
char v_UsernameReset[32] = "ERROR_NOTSET";              // Your IRC nickname modified to suit the server (nicklen etc)
char sv_QuitStr[32] = "Mega Drive IRC Client Quit"; // IRC quit message

static const u16 pColors[16] =
{
    0x000, 0x800, 0x0A0, 0x00E, 0x008, 0xA0A, 0x08E, 0x0EE,
    0x0E0, 0xAA0, 0xEE0, 0xE00, 0xE0E, 0x888, 0xCCC, 0xEEE
};

u8 PG_CurrentIdx = 0;
u8 PG_OpenPages = 1;
TMBuffer *PG_Buffer[IRC_MAX_CHANNELS] = {NULL};
char **PG_UserList = {NULL};
u16 PG_UserNum = 0;
u16 UserIterator = 0;


void IRC_Init()
{
    TTY_Init(TRUE);
    UTF8_Init();

    IRC_Reset();
}

void IRC_Reset()
{
    if (!sv_Font) PAL_setPalette(PAL2, pColors, DMA);

    memset(v_UsernameReset, 0, 32);
    strcpy(v_UsernameReset, sv_Username);

    TMB_SetColorFG(15);

    // Variable overrides
    vDoEcho = 1;
    vLineMode = 1;
    bFirstRun = TRUE;
    NickReRegisterCount = 0;
    sv_CursorCL = 0x0E0;
    LastCursor = 0x12;
    
    Stdout_Flush();

    // Allocate and setup channel slots
    for (u8 ch = 0; ch < IRC_MAX_CHANNELS; ch++)
    {
        PG_Buffer[ch] = malloc(sizeof(TMBuffer));
        if (PG_Buffer[ch] == NULL) 
        {
            #ifdef IRC_LOGGING
            kprintf("Failed to allocate memory for PG_Buffer[%u]", ch);
            #endif
            Stdout_Push("[91mIRC Client: Failed to allocate memory\n for PG_Buf![0m\n");
            RevertState();
            return;
        }
        memset(PG_Buffer[ch], 0, sizeof(TMBuffer));
        strcpy(PG_Buffer[ch]->Title, PG_EMPTYNAME);
        PG_Buffer[ch]->sy = C_YSTART;

        TMB_SetActiveBuffer(PG_Buffer[ch]);
        TMB_ClearBuffer();
    }

    // Allocate line buffer
    LineBuf = malloc(sizeof(struct s_linebuf));
    if (LineBuf == NULL) 
    {
        #ifdef IRC_LOGGING
        kprintf("Failed to allocate memory for LineBuf");
        #endif
        Stdout_Push("[91mIRC Client: Failed to allocate memory\n for LineBuf![0m\n");
        RevertState();
        return;
    }
    memset(LineBuf, 0, sizeof(struct s_linebuf));

    // Allocate RXString buffer
    RXString = malloc(B_RXSTRING_LEN);
    if (RXString == NULL) 
    {
        #ifdef IRC_LOGGING
        kprintf("Failed to allocate memory for RXString");
        #endif
        Stdout_Push("[91mIRC Client: Failed to allocate memory\n for RXString![0m\n");
        RevertState();
        return;
    }
    memset(RXString, 0, B_RXSTRING_LEN);

    // Allocate UserList pointers and UserList strings
    PG_UserList = malloc(IRC_MAX_USERLIST * sizeof(char*));
    if (PG_UserList != NULL)
    {
        for (u16 i = 0; i < IRC_MAX_USERLIST; i++)
        {
            PG_UserList[i] = (char*)malloc(IRC_MAX_USERNAME_LEN);
            
            if (PG_UserList[i] == NULL)
            {
                #ifdef IRC_LOGGING
                kprintf("Failed to allocate memory for PG_UserList[%u]", i);
                #endif
                //Stdout_Push("[91mIRC Client: Failed to allocate memory\n for PG_UserList[x]![0m\n\n");
                char tmp[80];

                sprintf(tmp, "[91mIRC Client: Failed to allocate memory\n for PG_UserList[%u]!\n", i);
                Stdout_Push(tmp);

                sprintf(tmp, "Free: %u - Needed: %u", MEM_getLargestFreeBlock(), IRC_MAX_USERNAME_LEN*IRC_MAX_USERLIST);
                Stdout_Push(tmp);

                Stdout_Push("[0m\n");

                RevertState();
                return;
            }
            memset(PG_UserList[i], 0, IRC_MAX_USERNAME_LEN);
        }
    }
    else
    {
        #ifdef IRC_LOGGING
        kprintf("Failed to allocate memory for PG_UserList");
        #endif
        Stdout_Push("[91mFailed to allocate memory for PG_UserList![0m\n");
        RevertState();
        return;
    }

    // Set defaults
    PG_CurrentIdx = 0;
    TMB_SetActiveBuffer(PG_Buffer[PG_CurrentIdx]);
    PG_OpenPages = 1;

    // Setup the cursor for the typing input box at the bottom of the screen
    s16 spr_x = 8 + 128;
    s16 spr_y = (bPALSystem?232:216) + 128;

    // First 9 textboxes
    for (u8 i = 0; i < 9; i++)
    {
        SetSprite_Y(i+2, spr_y);
        SetSprite_SIZELINK(i+2, SPR_WIDTH_4x1, i+3);
        SetSprite_TILE(i+2, 0x2000+TTS_VRAMIDX+(i*4));
        SetSprite_X(i+2, spr_x+(i*32));
    }

    // Final 10th textbox
    SetSprite_Y(11, spr_y);
    SetSprite_SIZELINK(11, SPR_WIDTH_3x1, 0);
    SetSprite_TILE(11, 0x2000+TTS_VRAMIDX+36);
    SetSprite_X(11, spr_x+288);

    // Setup cursor sprite y position and tile
    SetSprite_Y(SPRITE_ID_CURSOR, spr_y);
    SetSprite_TILE(SPRITE_ID_CURSOR, LastCursor);
    //SetSprite_SIZELINK(SPRITE_ID_CURSOR, 0, 1);
}

void IRC_Exit()
{
    char buf[64];
    sprintf(buf, "QUIT %s\n", sv_QuitStr);
    NET_SendString(buf);

    // Free previously allocated memory. DO NOT TRY TO DEBUG IN EMULATORS, IT WILL MISLEAD AND CAUSE YOU HARM.
    for (u8 ch = 0; ch < IRC_MAX_CHANNELS; ch++)
    {
        free(PG_Buffer[ch]);
        PG_Buffer[ch] = NULL;
    }

    free(LineBuf);
    LineBuf = NULL;

    free(RXString);
    RXString = NULL;

    for (u16 i = 0; i < IRC_MAX_USERLIST; i++)
    {
        free(PG_UserList[i]);
        PG_UserList[i] = NULL;
    }

    free(PG_UserList);
    PG_UserList = NULL;
}

// Text input at bottom of screen
void PrintTextLine(const u8 *str)
{
    u8 spr_x = 0;
    for (u8 px = 0; px < 39; px++)
    {
        if (str[px] == 0)
        {
            VDP_loadTileData(GFX_ASCII_MENU.tiles, TTS_VRAMIDX+px, 1, CPU);
        }
        else 
        {
            VDP_loadTileData(GFX_ASCII_MENU.tiles+((str[px])<<3), TTS_VRAMIDX+px, 1, CPU);
            spr_x = px+1;
        }
    }

    // Update cursor X position
    SetSprite_X(SPRITE_ID_CURSOR, (spr_x*8)+136);

    return;
}

void IRC_PrintChar(u8 c)
{
    if (bLookingForCL)
    {
        // Check if background colour follows, if not its a printable character
        if ((c != ',') && (bLookingForCL == 3))
        {
            bLookingForCL = 0;
            goto DoPrint;
        }
        else if ((c == ',') && (bLookingForCL == 3))
        {
            bLookingForCL++;
            return;
        }

        // Skip background colour bytes since we don't support it in the IRC client anymore
        if (bLookingForCL == 5)
        {
            bLookingForCL = 0;
            if ((c >= 0x30) && (c < 0x40)) return;
            else goto DoPrint;
        } 
        else if (bLookingForCL == 4)
        {
            bLookingForCL++;
            return;
        } 

        // Get second foreground colour byte if there is one or apply colour and print current char
        if (bLookingForCL >= 2)
        {
            if ((c >= 0x30) && (c < 0x40)) 
            {
                NewColor *= 10;
                NewColor += (c-48);
                NewColor--;
            }

            //bLookingForCL = 0;
            bLookingForCL++;

            if (NewColor < 16)
            {
                TMB_SetColorFG(NewColor);
            }
            else
            {
                TMB_SetColorFG(15);
            }
            
            NewColor = 0;

            if ((c >= 0x30) && (c < 0x40)) return;
            else goto DoPrint;
        }

        if ((c >= 0x30) && (c < 0x40)) // Get first foreground colour byte if there is a valid number between '0' - '9'
        {
            NewColor = (c-48);
            bLookingForCL++;
        }
        else    // Reset foreground colour and print current char
        {
            TMB_SetColorFG(15);
            bLookingForCL = 0;
            goto DoPrint;
        }
        
        return;
    }

    DoPrint:

    switch (c)
    {
        case 0x00:  //null
        case 0x01:  //start of heading
        case 0x02:  //start of text
        case 0x04:  //end of transmission
        case 0x05:  //enquiry
        case 0x06:  //acknowledge
        case 0x07:  //bell
        break;
        
        case 0x03:  //^C
            bLookingForCL++;
            return;
        break;

        case 0x08:  //backspace
            TMB_MoveCursor(TTY_CURSOR_LEFT, 1);
        break;
        case 0x09:  //horizontal tab
            TMB_MoveCursor(TTY_CURSOR_RIGHT, C_HTAB);
        break;
        case 0x0A:  //line feed, new line
            TMB_SetSX(0);
            TMB_MoveCursor(TTY_CURSOR_DOWN, 1);
        break;
        case 0x0B:  //vertical tab
            TMB_MoveCursor(TTY_CURSOR_DOWN, C_VTAB);
        break;
        case 0x0C:  //form feed, new page
            TMB_SetSX(0);
            TMB_SetSY(C_YSTART);

            TMB_SetVScroll(D_VSCROLL);
            TMB_ClearBuffer();

            TMB_MoveCursor(TTY_CURSOR_DUMMY);   // Dummy
        break;
        case 0x0D:  //carriage return
            TMB_SetSX(0);
            TMB_MoveCursor(TTY_CURSOR_DUMMY);   // Dummy
        break;
        case 0xE2:  // Dumb handling of UTF8
        case 0xEF:
            //bUTF8 = TRUE;
            return;
        break;

        default:
        break;
    }

    if ((c >= 0x20) && (c <= 0x7E))
    {
        TMB_PrintChar(c);
        
        return;
    }
}

// IRC/2 cmd numbers: https://www.alien.net.au/irc/irc2numerics.html
void IRC_DoCommand()
{
    char PrintBuf[B_PRINTSTR_LEN] = {0};    // Do not overflow this please
    char subprefix[B_SUBPREFIX_LEN] = {0};
    char subparam[B_SUBPREFIX_LEN] = {0};
    char ChanBuf[40];   // Channel name
    

    strncpy(ChanBuf, PG_Buffer[0]->Title, 40);

    if (LineBuf->param[1][0] == 1)
    {
        u16 end = 1;
        while ((LineBuf->prefix[end++] != '!') && (end < B_SUBPREFIX_LEN));
        strncpy(subprefix, LineBuf->prefix, end-1);

        end = 1;
        while (((LineBuf->param[1][end++] != ' ') && (LineBuf->param[1][end++] != 1)) && (end < B_SUBPREFIX_LEN));  // != 1
        strncpy(subparam, LineBuf->param[1]+1, end-2);

        if (strcmp(subparam, "VERSION") == 0)
        {
            snprintf(PrintBuf, B_PRINTSTR_LEN, "NOTICE %s :\1VERSION %s - Sega Mega Drive [m68k @ 7.6MHz]\1\n", subprefix, STATUS_TEXT);
            NET_SendString(PrintBuf);
            //kprintf("Version string: \"%s\"", PrintBuf);

            snprintf(PrintBuf, B_PRINTSTR_LEN, "[CTCP] Received Version request from %s\n", subprefix);
        }
        else if (strcmp(subparam, "ACTION") == 0)
        {
            u16 start = end;
            while ((LineBuf->param[1][end++] != 1) && (start+end < B_SUBPREFIX_LEN));  // != 1
            strncpy(subparam, LineBuf->param[1]+start, end-2);

            snprintf(PrintBuf, B_PRINTSTR_LEN, "* %s %s\n", subprefix, subparam);
            strncpy(ChanBuf, LineBuf->param[0], 40);
        }
        else
        {
            char dst[5];
            strncpy(dst, subparam, 4);

            if (strcmp(dst, "PING") == 0)
            {
                snprintf(PrintBuf, B_PRINTSTR_LEN, "[CTCP] Received ping from %s\n", subprefix);
            }
        }
    }
    else if (strcmp(LineBuf->command, "PING") == 0)
    {
        snprintf(PrintBuf, B_PRINTSTR_LEN, "PONG %s\n", LineBuf->param[0]);
        NET_SendString(PrintBuf);

        return;
    }
    else if (strcmp(LineBuf->command, "NOTICE") == 0)
    {
        u16 end = 1;
        while ((LineBuf->prefix[end++] != '!') && (end < B_SUBPREFIX_LEN));
        strncpy(subprefix, LineBuf->prefix, end-1);

        snprintf(PrintBuf, B_PRINTSTR_LEN, "[Notice] -%s- %s\n", subprefix, LineBuf->param[1]);
    }
    else if (strcmp(LineBuf->command, "ERROR") == 0)
    {
        snprintf(PrintBuf, B_PRINTSTR_LEN, "[Error] %s %s\n", LineBuf->prefix, LineBuf->param[0]);
    }
    else if (strcmp(LineBuf->command, "PRIVMSG") == 0)
    {
        // Extract nickname from <nick>!<<hostname>
        u16 end = 1;
        while ((LineBuf->prefix[end++] != '!') && (end < B_SUBPREFIX_LEN));
        strncpy(subprefix, LineBuf->prefix, end-1);

        snprintf(PrintBuf, B_PRINTSTR_LEN, "%s: %s\n", subprefix, LineBuf->param[1]);

        if (strcmp(LineBuf->param[0], v_UsernameReset) == 0)
        {
            strncpy(ChanBuf, subprefix, 40);
        }
        else 
        {
            strncpy(ChanBuf, LineBuf->param[0], 40);
        }
    }
    else if (strcmp(LineBuf->command, "JOIN") == 0)
    {
        // Extract nickname from <nick>!<<hostname>
        u16 end = 1;
        while ((LineBuf->prefix[end++] != '!') && (end < B_SUBPREFIX_LEN));
        strncpy(subprefix, LineBuf->prefix, end-1);

        if (strcmp(subprefix, v_UsernameReset) != 0)
        {
            snprintf(PrintBuf, B_PRINTSTR_LEN, "%s has joined the channel\n", LineBuf->prefix);
            strncpy(ChanBuf, LineBuf->param[0], 40);
        }
    }
    else if (strcmp(LineBuf->command, "QUIT") == 0)  // Todo: figure out which channel this user is in
    {
        // Extract nickname from <nick>!<<hostname>
        u16 end = 1;
        while ((LineBuf->prefix[end++] != '!') && (end < B_SUBPREFIX_LEN));
        strncpy(subprefix, LineBuf->prefix, end-1);

        if (strcmp(subprefix, v_UsernameReset) != 0)
        {
            snprintf(PrintBuf, B_PRINTSTR_LEN, "%s: %s\n", subprefix, LineBuf->param[0]);
        }
    }
    else if (strcmp(LineBuf->command, "MODE") == 0)
    {
        snprintf(PrintBuf, B_PRINTSTR_LEN, "[Mode] You have set personal modes: %s\n", LineBuf->param[1]);
    }
    else
    {
        u16 cmd = atoi16(LineBuf->command);

        switch (cmd)
        {
            case 0:
            {
                return;
            }
            case 1:
            case 2:
            case 3:
            {
                snprintf(PrintBuf, B_PRINTSTR_LEN, "[Welcome] %s\n", LineBuf->param[1]);
                strncpy(ChanBuf, LineBuf->prefix, 40);
                break;
            }
            case 4:
            {
                snprintf(PrintBuf, B_PRINTSTR_LEN, "[Welcome] Server %s (%s), User modes: %s, Channel modes: %s\n", LineBuf->param[1], LineBuf->param[2], LineBuf->param[3], LineBuf->param[4]);
                break;
            }
            case 5:
            {
                snprintf(PrintBuf, B_PRINTSTR_LEN, "[Support] %s %s %s %s %s %s %s %s %s %s %s %s %s %s\n", LineBuf->param[1], LineBuf->param[2], LineBuf->param[3], LineBuf->param[4], LineBuf->param[5], LineBuf->param[6], LineBuf->param[7], 
                                                                                                            LineBuf->param[8], LineBuf->param[9], LineBuf->param[10], LineBuf->param[11], LineBuf->param[12], LineBuf->param[13], LineBuf->param[14]);

                // Check for NICKLEN < your_nickname_len. If true then resize the temporary v_UsernameReset to be smaller
                char n[8] = {0};
                char l[4] = {0};
                u8 len = 0;
                
                strncpy(n, LineBuf->param[5], 7);

                if (strcmp(n, "NICKLEN") == 0)
                {
                    strncpy(l, LineBuf->param[5]+8, 4);
                    len = atoi(l);

                    if ((len > 0) && (len >= strlen(v_UsernameReset)-1))
                    {
                        strclr(v_UsernameReset);
                        strcpy(v_UsernameReset, LineBuf->param[0]);
                    }
                }

                break;
            }
            case 250:
            case 251:
            case 253:
            case 255:
            case 265:
            case 266:
            {
                snprintf(PrintBuf, B_PRINTSTR_LEN, "[Users] %s\n", LineBuf->param[1]);
                break;
            }
            case 252:
            case 254:
            {
                snprintf(PrintBuf, B_PRINTSTR_LEN, "[Users] %s %s\n", LineBuf->param[1], LineBuf->param[2]);
                break;
            }
            case 332:
            {
                snprintf(PrintBuf, B_PRINTSTR_LEN, "*** The channel topic is \"%s\".\n", LineBuf->param[2]);  // Fixme: multi line topics? if that is even a thing
                strncpy(ChanBuf, LineBuf->param[1], 40);
                break;
            }
            case 333:
            {
                snprintf(PrintBuf, B_PRINTSTR_LEN, "*** The topic was set by %s on %s.\n", LineBuf->param[2], LineBuf->param[3]);
                strncpy(ChanBuf, LineBuf->param[1], 40); 
                break;
            }
            case 353:   // Command 353 is a list of all users in param[3]
            {
                u16 start = 0;
                u16 end = 0;
                u8 len = 16;

                if (strcmp(PG_Buffer[PG_CurrentIdx]->Title, LineBuf->param[2]) == 0)
                {
                    while (1)
                    {
                        if (PG_UserList == NULL) break;

                        // Find the end of a nick or break on CR
                        while (LineBuf->param[3][end++] != ' ')
                        {
                            if (LineBuf->param[3][end] == 0xD)
                            {
                                if (UserIterator) UserIterator--;
                                break;
                            }
                        }

                        // Detect end of line space (not a valid nick)
                        if (strcmp(LineBuf->param[3]+start, "") == 0)
                        {
                            if (UserIterator) UserIterator--;
                            break;
                        }
                        
                        // Cap nick length
                        len = (end-start-1) > (IRC_MAX_USERNAME_LEN-1) ? (IRC_MAX_USERNAME_LEN-1) : (end-start-1);

                        memset(PG_UserList[UserIterator], 0, IRC_MAX_USERNAME_LEN);         // Clear old userlist nick string
                        strncpy(PG_UserList[UserIterator], LineBuf->param[3]+start, len);   // Copy new user nick to userlist

                        start = end;

                        if (end >= IRC_MAX_USERLIST) break;

                        UserIterator++;
                    }
                }

                strncpy(ChanBuf, LineBuf->param[2], 40); 
                break;
            }
            case 366:
            {
                // End of /NAMES list that begins with command 353
                if (strcmp(PG_Buffer[PG_CurrentIdx]->Title, LineBuf->param[1]) == 0)
                {
                    PG_UserNum = UserIterator;
                    UserIterator = 0;
                }
                break;
            }
            case 372:
            case 375:
            case 376:
            {
                snprintf(PrintBuf, B_PRINTSTR_LEN, "[MOTD] %s\n", LineBuf->param[1]);
                break;
            }
            case 396:   //Reply to a user when user mode +x (host masking) was set successfully 
            {
                snprintf(PrintBuf, B_PRINTSTR_LEN, "[Info] '%s' is now your hidden host (set by services).\n", LineBuf->param[1]);
                break;
            }
            case 401:   // ERR_NOSUCHNICK   RFC1459 <nick> :<reason>   -- Used to indicate the nickname parameter supplied to a command is currently unused
            {
                snprintf(PrintBuf, B_PRINTSTR_LEN, "[Error] %s\n", LineBuf->param[2]);
                break;
            }
            case 404:   // ERR_CANNOTSENDTOCHAN RFC1459 <channel> :<reason>   -- Sent to a user who does not have the rights to send a message to a channel
            {
                snprintf(PrintBuf, B_PRINTSTR_LEN, "[Error] %s\n", LineBuf->param[2]);
                strncpy(ChanBuf, LineBuf->param[1], 40);
                break;
            }
            case 412:   // ERR_NOTEXTTOSEND RFC1459 :<reason>   -- Returned when NOTICE/PRIVMSG is used with no message given
            {
                snprintf(PrintBuf, B_PRINTSTR_LEN, "[Error] %s\n", LineBuf->param[1]);
                break;
            }
            case 433:   // ERR_NICKNAMEINUSE RFC1459 <nick> :<reason>   -- Returned by the NICK command when the given nickname is already in use 
            {
                snprintf(PrintBuf, B_PRINTSTR_LEN, "[Error] %s\n", LineBuf->param[2]);

                // Attempt to re-register with a new nick 
                if (NickReRegisterCount < 2)
                {
                    snprintf(v_UsernameReset, 32, "_%s_", sv_Username);
                    IRC_RegisterNick();

                    NickReRegisterCount++;
                }
                break;
            }
            case 462:   // ERR_ALREADYREGISTERED
            {
                snprintf(PrintBuf, B_PRINTSTR_LEN, "[Error] %s\n", LineBuf->param[1]);
                break;
            }
            case 477:   // ERR_NEEDREGGEDNICK
            {
                snprintf(PrintBuf, B_PRINTSTR_LEN, "[Channel] %s\n", LineBuf->param[2]);
                break;
            }
            case 480:   // 
            {
                snprintf(PrintBuf, B_PRINTSTR_LEN, "[480] %s %s %s\n", v_UsernameReset, LineBuf->param[1], LineBuf->param[2]);  // LineBuf->param[3] = Further information (tls required for example etc)
                break;
            }
        
            default:
                #ifdef IRC_LOGGING
                kprintf("Error: Unhandled IRC CMD: %u", cmd);
                #endif
                return;
            break;
        }     
    }

    for (u8 ch = 0; ch < IRC_MAX_CHANNELS; ch++)
    {
        if (strcmp(PG_Buffer[ch]->Title, ChanBuf) == 0) // Find the page this message belongs to
        {
            TMB_SetActiveBuffer(PG_Buffer[ch]);
            break;
        }
        else if (strcmp(PG_Buffer[ch]->Title, PG_EMPTYNAME) == 0) // See if there is an empty page
        {
            char TitleBuf[40];
            strncpy(PG_Buffer[ch]->Title, ChanBuf, 32);
            TMB_SetActiveBuffer(PG_Buffer[ch]);

            snprintf(TitleBuf, 40, "%s - %-21s", STATUS_TEXT, PG_Buffer[PG_CurrentIdx]->Title);
            TRM_SetStatusText(TitleBuf);

            break;
        }
        else    // No empty pages and no page match, bail.
        {
        }
    }

    u16 len = strlen(PrintBuf);
    for (u16 i = 0; i < len; i++) IRC_PrintChar(PrintBuf[i]);

    TMB_SetColorFG(15);
}

void IRC_ParseString()
{
    u8 bBreak = FALSE;
    u16 it = 0;
    u8 seq = 0;

    while (!bBreak)
    {
        switch (RXString[it])
        {
            case ':':
            {
                if (it == 0)
                {
                    u16 end = 1;
                    while (RXString[end++] != ' ');

                    strncpy(LineBuf->prefix, RXString+1, (end-2)>=B_PREFIX_LEN?B_PREFIX_LEN-1:(end-2));
                    it = end-1;
                }
                else
                {
                    u16 end = it+1;
                    while (RXString[end++] != '\0');

                    strncpy(LineBuf->param[seq-1], RXString+it+1, (end-it-2)>=B_PARAM_LEN?B_PARAM_LEN-1:(end-it-2));                    
                    bBreak = TRUE;
                }

                break;
            }
            case ' ':
            {
                u16 end = it+1;
                while (RXString[end++] != ' ');

                if (seq == 0)
                {
                    strncpy(LineBuf->command, RXString+it+1, (end-it-2)>=B_COMMAND_LEN?B_COMMAND_LEN-1:(end-it-2));
                    seq++;
                    it = end-1;
                }
                else if (RXString[it+1] != ':')
                {
                    strncpy(LineBuf->param[seq-1], RXString+it+1, (end-it-2)>=B_PARAM_LEN?B_PARAM_LEN-1:(end-it-2));
                    seq++;
                    it = end-2;
                }

                break;
            }
            case '\0':
            {
                bBreak = TRUE;
                break;
            }

            default:
            {
                if (seq == 0)
                {
                    u16 end = it+1;
                    while (RXString[end++] != ' ');
                    strncpy(LineBuf->command, RXString+it, (end-it-1)>=B_COMMAND_LEN?B_COMMAND_LEN-1:(end-it-1));
                    seq++;
                }

                break;
            }
        }

        if (seq >= 16) break;

        it++;
    }

    #ifdef IRC_LOGGING
    kprintf("Prefix: \"%s\"", LineBuf->prefix);
    kprintf("Command: \"%s\"", LineBuf->command);
    for (u8 i = 0; i < 16; i++) if (strlen(LineBuf->param[i]) > 0) kprintf("Param[%u]: \"%s\"", i, LineBuf->param[i]);
    #endif

    IRC_DoCommand();
}

void IRC_ParseRX(u8 byte)
{
    RXBytes++;

    if (bFirstRun)
    {
        IRC_RegisterNick();
    }

    switch (byte)
    {
        case 0x0A:  // Line feed (new line)
            RXString[RXStringSeq++] = '\0';
            LineBuf->prefix[0] = '\0';
            LineBuf->command[0] = '\0';
            for (u8 i = 0; i < 16; i++){LineBuf->param[i][0] = '\0';}

            IRC_ParseString();

            RXStringSeq = 0;
            CR_Set = FALSE;
        break;
        case 0x0D:  // Carriage return
            CR_Set = TRUE;
        break;

        default:
            if (RXStringSeq < (B_RXSTRING_LEN-1)) RXString[RXStringSeq++] = byte;
            else RXStringSeq--;
        break;
    }
}

void IRC_RegisterNick()
{
    char buf[64];
    sprintf(buf, "NICK %s\n", v_UsernameReset);
    NET_SendString(buf);

    if (bFirstRun)
    {
        sprintf(buf, "USER %s m68k smdnet :Mega Drive\n", v_UsernameReset); 
        NET_SendString(buf);

        bFirstRun = FALSE;
    }
}
