#include "IRC.h"
#include "Terminal.h"
#include "UTF8.h"
#include "Utils.h"
#include "Buffer.h"
#include "Network.h"
#include "Cursor.h"
#include "StateCtrl.h"
#include "Telnet.h"         // LMSM_EDIT
#include "../res/system.h"

#include "system/Stdout.h"
#include "system/Time.h"

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

#define TTS_VRAMIDX 0x400

static struct s_linebuf
{
    char prefix[B_PREFIX_LEN];
    char command[B_COMMAND_LEN];
    char param[16][B_PARAM_LEN];
} *LineBuf = NULL;

static bool CR_Set = FALSE; // Carridge return flag
static u8 bLookingForCL = 0;// Looking for colour flag (Parsing current/following bytes as colour)
static u8 NewColor = 0;     // Temporary buffer for incomming colour bytes 

static char *RXString = NULL;
static u16 RXStringSeq = 0;

static bool bFirstRun = TRUE;
static u8 NickReRegisterCount = 0;

static const u16 pColors[16] =
{
    0x000, 0x800, 0x0A0, 0x00E, 0x008, 0xA0A, 0x08E, 0x0EE,
    0x0E0, 0xAA0, 0xEE0, 0xE00, 0xE0E, 0x888, 0xCCC, 0xEEE
};

char sv_Username[32] = "smd_user";                   // Saved preferred IRC nickname
char v_UsernameReset[32] = "ERROR_NOTSET";           // Your IRC nickname modified to suit the server (nicklen etc)
char sv_QuitStr[32] = "SMDT IRC client quit";        // IRC quit message
u8 sv_ShowJoinQuitMsg = 1;
u8 sv_WrapAtScreenEdge = 1;

u8 PG_CurrentIdx = 0;                                // Current active page/channel number
u8 PG_OpenPages = 1;                                 // Number of pages/channels in use
TMBuffer *PG_Buffer[IRC_MAX_CHANNELS] = {NULL};      // Page/Channel buffers
char **PG_UserList = {NULL};                         // Array of username strings (nick)
u16 PG_UserNum = 0;                                  // Number of entries in PG_UserList
u8 bPG_UpdateUserlist = 2;                           // Flag to update the user list window (0=Do nothing - 1=Do full redraw with a populated NAMES list - 2=Do full redraw and wait for NAMES list)
bool bPG_HasNewMessages[IRC_MAX_CHANNELS] = {FALSE}; // Flag to show if a channel has new messages
bool bPG_UpdateMessage = TRUE;                       // Flag to update the "HasNewMessages" text
u16 UserIterator = 0;                                // Used to iterate through PG_UserList during a 353 command
u8 NameOffset = 0;                                   // Used to indent multiline messages


u16 IRC_Init()
{
    TTY_Init(TF_Everything);
    return IRC_Reset();
}

u16 IRC_Reset()
{
    if (sv_Font) 
    {
        PAL_setColor(14, sv_CFG1CL);    // Nick name colour - AA
        PAL_setColor(15, sv_CFG0CL);    // Nick name colour
        PAL_setColor(46, 0x666);        // Text colour - AA
        PAL_setColor(47, 0xEEE);        // Text colour
    }
    else
    {
        PAL_setPalette(PAL2, pColors, DMA);
        DMA_waitCompletion();
    }

    memset(v_UsernameReset, 0, 32);
    strcpy(v_UsernameReset, sv_Username);

    TMB_SetColorFG(15);

    // Variable overrides
    vDoEcho = 1;
    vLineMode = LMSM_EDIT;
    bFirstRun = TRUE;
    NickReRegisterCount = 0;
    NameOffset = 0;
    LastCursor = 0x12;
    
    Stdout_Flush();
    Buffer_Flush(&TxBuffer);

    // Upload typing fontset to VRAM, it will be used as source for DMA VRAM copy - Do this here or we won't have enough memory to unpack the fontset
    VDP_loadTileSet(&GFX_IRC_TYPE, AVR_FONT1, DMA);
    DMA_waitCompletion();

    // Allocate and setup channel slots
    for (u8 ch = 0; ch < IRC_MAX_CHANNELS; ch++)
    {
        PG_Buffer[ch] = malloc(sizeof(TMBuffer));
        if (PG_Buffer[ch] == NULL) 
        {
            #ifdef IRC_LOGGING
            kprintf("Failed to allocate memory for PG_Buffer[%u]", ch);
            #endif
            
            Stdout_Push("[91mIRC Client: Failed to allocate memory for PG_Buf![0m\n");
            return EXIT_FAILURE;
        }
        TMB_SetActiveBuffer(PG_Buffer[ch]);
        TMB_ZeroCurrentBuffer();
        strcpy(PG_Buffer[ch]->Title, PG_EMPTYNAME);
        PG_Buffer[ch]->sy = C_YSTART;
    }

    // Allocate line buffer
    LineBuf = malloc(sizeof(struct s_linebuf));
    if (LineBuf == NULL) 
    {
        #ifdef IRC_LOGGING
        kprintf("Failed to allocate memory for LineBuf");
        #endif

        Stdout_Push("[91mIRC Client: Failed to allocate memory for LineBuf![0m\n");
        return EXIT_FAILURE;
    }
    memset(LineBuf, 0, sizeof(struct s_linebuf));

    // Allocate RXString buffer
    RXString = malloc(B_RXSTRING_LEN);
    if (RXString == NULL) 
    {
        #ifdef IRC_LOGGING
        kprintf("Failed to allocate memory for RXString");
        #endif

        Stdout_Push("[91mIRC Client: Failed to allocate memory for RXString![0m\n");
        return EXIT_FAILURE;
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

                printf("[91mIRC Client: Failed to allocate memory for PG_UserList[%u]!\n", i);
                printf("Free: %u - Needed: %u (Total: %u)[0m\n", MEM_getLargestFreeBlock(), IRC_MAX_USERNAME_LEN, IRC_MAX_USERNAME_LEN*IRC_MAX_USERLIST);

                return EXIT_FAILURE;
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
        return EXIT_FAILURE;
    }

    // Set defaults
    PG_CurrentIdx = 0;
    TMB_SetActiveBuffer(PG_Buffer[PG_CurrentIdx]);
    PG_OpenPages = 1;
    bPG_UpdateMessage = TRUE;

    for (u8 i = 0; i < IRC_MAX_CHANNELS; i++)
    {
        bPG_HasNewMessages[i] = FALSE;
    }

    // Setup the cursor for the typing input box at the bottom of the screen
    s16 spr_x = 8 + 128;
    s16 spr_y = (bPALSystem?232:216) + 128;

    // First 9 textboxes
    for (u8 i = 0; i < 9; i++)
    {
        SetSprite_Y(i+2, spr_y);
        SetSprite_SIZELINK(i+2, SPR_WIDTH_4x1, i+3);
        SetSprite_TILE(i+2, 0x6000+TTS_VRAMIDX+(i*4));  // 0x6000 = PAL3
        SetSprite_X(i+2, spr_x+(i*32));
    }

    // Final 10th textbox
    SetSprite_Y(11, spr_y);
    SetSprite_SIZELINK(11, SPR_WIDTH_3x1, 0);
    SetSprite_TILE(11, 0x6000+TTS_VRAMIDX+36);  // 0x6000 = PAL3
    SetSprite_X(11, spr_x+288);

    // Setup cursor sprite y position and tile
    SetSprite_Y(SPRITE_ID_CURSOR, spr_y);
    SetSprite_TILE(SPRITE_ID_CURSOR, LastCursor);
    //SetSprite_SIZELINK(SPRITE_ID_CURSOR, 0, 1);

    return EXIT_SUCCESS;
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
        if (((int)PG_UserList[i] & 0xFFFFFF) > 0xFF0000)
        {
            free(PG_UserList[i]);
            PG_UserList[i] = NULL;
        }
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
            DMA_doVRamCopy(AVR_FONT1<<5, (TTS_VRAMIDX+px)<<5, 32, 1);
        }
        else 
        {
            spr_x = px+1;
            DMA_doVRamCopy((AVR_FONT1+str[px]-32)<<5, (TTS_VRAMIDX+px)<<5, 32, 1);
        }

        DMA_waitCompletion();
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
        case 0x4:   // Custom - Set default colour
        case 0xF:   // Colour reset
            TMB_SetColorFG(15);
            return;
        break;

        case 0x5:   // Custom - Set name colour
            TMB_SetColorFG(6);
            return;
        break;

        case 0x03:  // ^C
            bLookingForCL++;
            return;
        break;

        case 0x0A:  // Line feed, new line
            TMB_SetSX(0);
            TMB_MoveCursor(TTY_CURSOR_DOWN, 1);
        break;

        default:
            if (c >= 0x20)
            {
                TMB_PrintChar(c);
            }
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

    NameOffset = 0;
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

        snprintf(PrintBuf, B_PRINTSTR_LEN, "\5%s: \4%s\n", subprefix, LineBuf->param[1]);
        NameOffset = strlen(subprefix) + 2;

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
        if (sv_ShowJoinQuitMsg == 0) return;

        // Extract nickname from <nick>!<<hostname>
        u16 end = 1;
        while ((LineBuf->prefix[end++] != '!') && (end < B_SUBPREFIX_LEN));
        strncpy(subprefix, LineBuf->prefix, end-1);

        if (strcmp(subprefix, v_UsernameReset) != 0)
        {
            snprintf(PrintBuf, B_PRINTSTR_LEN, "%s has joined this channel\n", LineBuf->prefix);
            strncpy(ChanBuf, LineBuf->param[0], 40);
        }
    }
    else if (strcmp(LineBuf->command, "QUIT") == 0)  // Todo: figure out which channel this user is in
    {
        if (sv_ShowJoinQuitMsg == 0) return;

        // Extract nickname from <nick>!<<hostname>
        u16 end = 1;
        while ((LineBuf->prefix[end++] != '!') && (end < B_SUBPREFIX_LEN));
        strncpy(subprefix, LineBuf->prefix, end-1);

        if (strcmp(subprefix, v_UsernameReset) != 0)
        {
            snprintf(PrintBuf, B_PRINTSTR_LEN, "%s: %s\n", subprefix, LineBuf->param[0]);
        }

        // Use "WHOWAS <subprefix> 1" to query user information and print above quit message in proper channel later?
    }
    else if (strcmp(LineBuf->command, "MODE") == 0)
    {
        // Extract nickname from <nick>!<<hostname>
        u16 end = 1;
        while ((LineBuf->prefix[end++] != '!') && (end < B_SUBPREFIX_LEN));
        strncpy(subprefix, LineBuf->prefix, end-1);

        //kprintf("MODE: Cmd: %s - Prefix: %s - P0: %s - P1: %s - P2: %s -- subprefix: %s", LineBuf->command, LineBuf->prefix, LineBuf->param[0], LineBuf->param[1], LineBuf->param[2], subprefix);

        if (strcmp(subprefix, v_UsernameReset) != 0)
        {
            // https://datatracker.ietf.org/doc/html/rfc2811#section-4
            switch (LineBuf->param[1][1])
            {
                // Nick limit
                case 'l':
                    if (LineBuf->param[1][0] == '-') snprintf(PrintBuf, B_PRINTSTR_LEN, "[Mode] %s removes the channel nick limit.\n", subprefix);
                    else                             snprintf(PrintBuf, B_PRINTSTR_LEN, "[Mode] %s sets the channel limit to %s nicks.\n", subprefix, LineBuf->param[2]);
                break;

                // Voice
                case 'v':
                    // Is this your nick? Then use "you/your"
                    if (strcmp(LineBuf->param[2], v_UsernameReset) == 0)
                    {
                        if (LineBuf->param[1][0] == '-') snprintf(PrintBuf, B_PRINTSTR_LEN, "[Mode] %s removes your permission to talk.\n", subprefix);
                        else                             snprintf(PrintBuf, B_PRINTSTR_LEN, "[Mode] %s gives you permission to talk.\n", subprefix);
                    }
                    // If this is not you, then use the nick it is refering to.
                    else
                    {
                        if (LineBuf->param[1][0] == '-') snprintf(PrintBuf, B_PRINTSTR_LEN, "[Mode] %s removes %s' permission to talk.\n", subprefix, LineBuf->param[2]);
                        else                             snprintf(PrintBuf, B_PRINTSTR_LEN, "[Mode] %s gives %s permission to talk.\n", subprefix, LineBuf->param[2]);
                    }
                break;
            
                default:
                    snprintf(PrintBuf, B_PRINTSTR_LEN, "[Mode] %s sets mode %s.\n", subprefix, LineBuf->param[1]);
                break;
            }

            strncpy(ChanBuf, LineBuf->param[0], 40);
        }
        else snprintf(PrintBuf, B_PRINTSTR_LEN, "[Mode] You have set personal modes: %s\n", LineBuf->param[1]);
    }
    else if (strcmp(LineBuf->command, "NICK") == 0)
    {
        // Extract nickname from <nick>!<<hostname>
        u16 end = 1;
        while ((LineBuf->prefix[end++] != '!') && (end < B_SUBPREFIX_LEN));
        strncpy(subprefix, LineBuf->prefix, end-1);

        snprintf(PrintBuf, B_PRINTSTR_LEN, "*** %s is now known as %s\n", subprefix, LineBuf->param[0]);
    }
    else if (strcmp(LineBuf->command, "PART") == 0)
    {
        snprintf(PrintBuf, B_PRINTSTR_LEN, "%s left this channel\n", LineBuf->prefix);  // LineBuf->param[1] = ?
        strncpy(ChanBuf, LineBuf->param[0], 40);
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
                SM_Time t = SecondsToDateTime(atoi32(LineBuf->param[3]));
                char buf[40];
                TimeToStr_Full(t, buf);

                snprintf(PrintBuf, B_PRINTSTR_LEN, "*** The topic was set by %s on %s.\n", LineBuf->param[2], buf);
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
                        if ((PG_UserList == NULL) || (UserIterator >= IRC_MAX_USERLIST-1)) break;

                        // Find the end of a nick or break on CR
                        while (LineBuf->param[3][end++] != ' ')
                        {
                            if (LineBuf->param[3][end] == 0xD)
                            {
                                break;
                            }
                        }

                        // Detect end of line space (not a valid nick)
                        if (strcmp(LineBuf->param[3]+start, " ") == 0)
                        {
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
                return;
            }
            case 366:
            {
                // End of /NAMES list that begins with command 353
                if (strcmp(PG_Buffer[PG_CurrentIdx]->Title, LineBuf->param[1]) == 0)
                {
                    PG_UserNum = UserIterator;
                    UserIterator = 0;
                }
                return;
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
                #if IRC_LOGGING >= 1
                kprintf("Error: Unhandled IRC CMD: %u ---------------------------------------------", cmd);
                kprintf("Error: Prefix: \"%s\"", LineBuf->prefix);
                kprintf("Error: Command: \"%s\"", LineBuf->command);
                for (u8 i = 0; i < 16; i++) if (strlen(LineBuf->param[i]) > 0) kprintf("Error: Param %u: \"%s\"", i, LineBuf->param[i]);
                kprintf("--------------------------------------------------------------------------");
                #endif
                return;
            break;
        }
    }

    // Print text to the correct channel page/tab
    for (u8 ch = 0; ch < IRC_MAX_CHANNELS; ch++)
    {
        if (strcmp(PG_Buffer[ch]->Title, ChanBuf) == 0) // Find the page this message belongs to
        {
            TMB_SetActiveBuffer(PG_Buffer[ch]);
            
            if (PG_CurrentIdx != ch) 
            {
                bPG_HasNewMessages[ch] = TRUE;
            }
            
            bPG_UpdateMessage = TRUE;

            break;
        }
        else if (strcmp(PG_Buffer[ch]->Title, PG_EMPTYNAME) == 0) // Or see if there is an empty page
        {
            char TitleBuf[32];
            strncpy(PG_Buffer[ch]->Title, ChanBuf, 32);
            TMB_SetActiveBuffer(PG_Buffer[ch]);

            snprintf(TitleBuf, 29, "%s %-*s", STATUS_TEXT_SHORT, 27-IRC_MAX_CHANNELS, PG_Buffer[PG_CurrentIdx]->Title);
            TRM_SetStatusText(TitleBuf);

            if (PG_CurrentIdx != ch) 
            {
                bPG_HasNewMessages[ch] = TRUE;
            }

            bPG_UpdateMessage = TRUE;

            break;
        }
        else    // No empty pages and no page match, bail.
        {
        }
    }

    IRC_PrintString(PrintBuf);

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

    #if IRC_LOGGING >= 2
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
            memset(LineBuf->prefix, 0, B_PREFIX_LEN);
            memset(LineBuf->command, 0, B_COMMAND_LEN);
            for (u8 i = 0; i < 16; i++){memset(LineBuf->param[i], 0, B_PARAM_LEN);}

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

// Print string to screen, word wrap at screen edge if settings allow for it
void IRC_PrintString(char *string)
{
    u16 len = strlen(string);
    u16 screen_width = ((sv_Font == 0) ? 40 : 80) - (sv_HSOffset >> 3); // Effective screen width
    s16 current_column = -1;
    u16 i = 0;

    while (i < len) 
    {
        if (sv_WrapAtScreenEdge)
        {
            // Look ahead to determine the length of the next word
            u16 word_end = i;
            u8 wo = 0;  // Word offset
            while (word_end+wo < len && 
                   string[word_end+wo] != ' '  && 
                   string[word_end+wo] != 0x1  && // ...
                   string[word_end+wo] != 0x3  && // Control character
                   string[word_end+wo] != 0x4  && // Custom text colour reset
                   string[word_end+wo] != 0x5  && // Custom text colour set
                   string[word_end+wo] != 0xF  && // Colour reset
                   string[word_end+wo] != '\\' && 
                   string[word_end+wo] != '\n') 
            {
                if ((string[word_end+wo-1] == 0x3 || string[word_end+wo-2] == 0x3) && (string[word_end+wo] >= '0' || string[word_end+wo] <= '9'))
                {
                    wo++;
                    continue;
                }

                // UTF-8
                if ((u8)string[word_end+wo] == 0xE2)
                {
                    // '
                    if (((u8)string[word_end+wo+1] == 0x80) && ((u8)string[word_end+wo+2] == 0xA6))
                    {
                        string[word_end+wo  ] = '.';
                        string[word_end+wo+1] = '.';
                        string[word_end+wo+2] = '.';
                        wo += 2;
                    }

                    // '
                    if (((u8)string[word_end+wo+1] == 0x80) && ((u8)string[word_end+wo+2] == 0x99))
                    {
                        string[word_end+wo  ] = '\2';
                        string[word_end+wo+1] = '\2';
                        string[word_end+wo+2] = '\'';
                        wo += 2;
                        continue;
                    }

                }
                
                word_end++;
            }

            u16 word_length = word_end - i;

            // Check if the word fits in the current line
            if (current_column + word_length > screen_width) 
            {
                // If the word itself is too long for the screen width, print as much as fits
                if (word_length + NameOffset > screen_width)
                {
                    u16 chars_to_print = screen_width - current_column;
                    
                    // Print characters up to the screen boundary, then wrap
                    for (u16 j = 0; j < chars_to_print; j++) 
                    {
                        IRC_PrintChar(string[i]);
                        i++;
                        current_column++;
                    }

                    IRC_PrintChar('\n');      // Insert newline after reaching screen width

                    for (u8 sp = 0; sp < NameOffset; sp++) IRC_PrintChar(' ');  // Offset any extra lines to line messages up
                    current_column = NameOffset;                                // Reset column for new line + offset for extra lines
                }
                else 
                {
                    // If the word can fit on the next line, wrap before printing it
                    IRC_PrintChar('\n');

                    for (u8 sp = 0; sp < NameOffset; sp++) IRC_PrintChar(' ');  // Offset any extra lines to line messages up
                    current_column = NameOffset;                                // Reset column for new line + offset for extra lines
                }
            }
        }

        // Print the current character and move to the next
        IRC_PrintChar(string[i]);
        i++;
        
        if (string[i] != 0x0  && 
            string[i] != 0x1  && // ...
            string[i] != 0x3  && // Control character
            string[i] != 0x4  && // Custom text colour reset
            string[i] != 0x5  && // Custom text colour set
            string[i] != 0xF  && // Colour reset
            string[i] != '\\' && 
            string[i] != '\n')
        {
            current_column++;
        }

        // Reset column counter if a newline character is encountered in the input
        if (string[i - 1] == '\n') 
        {
            current_column = -1;
        }
    }
}
