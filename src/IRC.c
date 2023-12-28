
#include "IRC.h"
#include "Terminal.h"
#include "UTF8.h"
#include "Utils.h"

#define B_PRINTSTR_LEN 256
#define B_SUBPREFIX_LEN 256

/*
1. Pass message
2. Nick message
3. User message
----

Command: PASS
Parameters: <password>

PASS secretpasswordhere

ERR_NEEDMOREPARAMS
ERR_ALREADYREGISTRED


----

Command: NICK
Parameters: <nickname> [ <hopcount> ]


ERR_NONICKNAMEGIVEN
ERR_ERRONEUSNICKNAME
ERR_NICKNAMEINUSE
ERR_NICKCOLLISION

NICK Wiz                        ; Introducing new nick "Wiz".
:WiZ NICK Kilroy                ; WiZ changed his nickname to Kilroy.

----

Command: USER
Parameters: <username> <hostname> <servername> <realname>

ERR_NEEDMOREPARAMS
ERR_ALREADYREGISTRED

USER guest tolmoon tolsun :Ronnie Reagan

----

Command: QUIT
Parameters: [<Quit message>]

QUIT :Gone to have lunch        ; Preferred message format.

----

Command: PING
Parameters: <server1> [<server2>]

ERR_NOORIGIN
ERR_NOSUCHSERVER

PING WiZ                        ; PING message being sent to nick WiZ

----

Command: PONG
Parameters: <daemon> [<daemon2>]

ERR_NOORIGIN
ERR_NOSUCHSERVER

PONG csd.bu.edu tolsun.oulu.fi  ; PONG message from csd.bu.edu to
*/

static struct s_linebuf
{
    char prefix[256];
    char command[64];
    char param[16][256];
} LineBuf =
{
    {0}, {0}, {{0}}
};

static bool CR_Set = FALSE;
static u8 bLookingForCL = 0;
static u8 NewColor = 0;
extern u16 ColorFG;

static char RXString[1024];
static u16 RXStringSeq = 0;

static const u16 pColors[16] =
{
    0x000, 0x800, 0x0A0, 0x00E, 0x008, 0xA0A, 0x08E, 0x0EE,
    0x0E0, 0xAA0, 0xEE0, 0xE00, 0xE0E, 0x888, 0xCCC, 0xEEE
};


void IRC_Init()
{
    TTY_Init(TRUE);
    vNewlineConv = 1;
    UTF8_Init();

    PAL_setPalette(PAL2, pColors, DMA);
    ColorFG = 15;
}

void IRC_PrintChar(u8 c)
{
    if (bLookingForCL)
    {
        if (bLookingForCL >= 2)
        {
            if ((c >= 0x30) && (c < 0x40)) 
            {
                NewColor *= 10;
                NewColor += (c-48);
                NewColor--;
            }

            bLookingForCL = 0;

            if (NewColor < 16)
            {
                ColorFG = NewColor;
            }
            else
            {
                ColorFG = 15;
            }
            
            NewColor = 0;

            return;
        }

        if ((c >= 0x30) && (c < 0x40)) 
        {
            NewColor = (c-48);
            bLookingForCL++;
        }
        else 
        {
            ColorFG = 15;
            bLookingForCL = 0;
        }
        
        return;
    }

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
            TTY_MoveCursor(TTY_CURSOR_LEFT, 1);
        break;
        case 0x09:  //horizontal tab
            TTY_MoveCursor(TTY_CURSOR_RIGHT, C_HTAB);
        break;
        case 0x0A:  //line feed, new line
            if (vNewlineConv == 1)  // Convert \n to \n\r
            {
                TTY_SetSX(0);
            }
            TTY_MoveCursor(TTY_CURSOR_DOWN, 1);
        break;
        case 0x0B:  //vertical tab
            TTY_MoveCursor(TTY_CURSOR_DOWN, C_VTAB);
        break;
        case 0x0C:  //form feed, new page
            TTY_SetSX(0);
            TTY_SetSY(C_YSTART);

            VScroll = D_VSCROLL;

            VDP_clearPlane(BG_A, TRUE);
            VDP_clearPlane(BG_B, TRUE);

            VDP_setVerticalScrollVSync(BG_A, VScroll);
            VDP_setVerticalScrollVSync(BG_B, VScroll);

            TTY_MoveCursor(TTY_CURSOR_DUMMY);   // Dummy
        break;
        case 0x0D:  //carriage return
            TTY_SetSX(0);
            TTY_MoveCursor(TTY_CURSOR_DUMMY);   // Dummy
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
        TTY_PrintChar(c);
        
        return;
    }
}

// IRC/2 cmd numbers: https://www.alien.net.au/irc/irc2numerics.html
void IRC_DoCommand()
{
    char PrintBuf[B_PRINTSTR_LEN] = {0};
    char subprefix[B_SUBPREFIX_LEN] = {0};
    char subparam[B_SUBPREFIX_LEN] = {0};

    //memset(PrintBuf, 0, B_PRINTSTR_LEN);

    if (LineBuf.param[1][0] == 1)
    {
        u16 end = 1;
        while ((LineBuf.prefix[end++] != '!') && (end < 256));
        strncpy(subprefix, LineBuf.prefix, end-1);

        end = 1;
        while ((LineBuf.param[1][end++] != 1) && (end < 256));
        strncpy(subparam, LineBuf.param[1]+1, end-2);

        if (strcmp(subparam, "VERSION") == 0)
        {
            sprintf(PrintBuf, "NOTICE %s :\1VERSION %s - Sega Mega Drive [m68k @ 7.6MHz]\1\n", subprefix, STATUS_TEXT);
            TTY_SendString(PrintBuf);
            kprintf("Version string: \"%s\"", PrintBuf);

            sprintf(PrintBuf, "[CTCP] Received Version request from %s\n", subprefix);
        }
        else
        {
            char dst[5];
            strncpy(dst, subparam, 4);

            if (strcmp(dst, "PING") == 0)
            {
                sprintf(PrintBuf, "[CTCP] Received ping from %s\n", subprefix);
            }
        }
    }
    else if (strcmp(LineBuf.command, "PING") == 0)
    {
        sprintf(PrintBuf, "PONG %s\n", LineBuf.param[0]);
        TTY_SendString(PrintBuf);

        return;
    }
    else if (strcmp(LineBuf.command, "NOTICE") == 0)
    {
        u16 end = 1;
        while ((LineBuf.prefix[end++] != '!') && (end < 256));
        strncpy(subprefix, LineBuf.prefix, end-1);

        sprintf(PrintBuf, "[Notice] -%s- %s\n", subprefix, LineBuf.param[1]);
    }
    else if (strcmp(LineBuf.command, "ERROR") == 0)
    {
        sprintf(PrintBuf, "[Error] %s %s\n", LineBuf.prefix, LineBuf.param[0]);
    }
    else if (strcmp(LineBuf.command, "PRIVMSG") == 0)
    {
        // Extract nickname from <nick>!<<hostname>
        u16 end = 1;
        while ((LineBuf.prefix[end++] != '!') && (end < 256));
        strncpy(subprefix, LineBuf.prefix, end-1);

        sprintf(PrintBuf, "%s->%s: %s\n", subprefix, LineBuf.param[0], LineBuf.param[1]);
    }
    else if (strcmp(LineBuf.command, "MODE") == 0)
    {
        sprintf(PrintBuf, "[Mode] You have set personal modes: %s\n", LineBuf.param[1]);
    }
    else
    {
        u16 cmd = atoi16(LineBuf.command);

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
                sprintf(PrintBuf, "[Welcome] %s\n", LineBuf.param[1]);
                break;
            }
            case 4:
            {
                sprintf(PrintBuf, "[Welcome] Server %s (%s), User modes: %s, Channel modes: %s\n", LineBuf.param[1], LineBuf.param[2], LineBuf.param[3], LineBuf.param[4]);
                break;
            }
            case 5:
            {
                sprintf(PrintBuf, "[Support] %s %s %s %s %s %s %s %s %s %s %s %s %s %s\n", LineBuf.param[1], LineBuf.param[2], LineBuf.param[3], LineBuf.param[4], LineBuf.param[5], LineBuf.param[6], LineBuf.param[7], 
                                                                                           LineBuf.param[8], LineBuf.param[9], LineBuf.param[10], LineBuf.param[11], LineBuf.param[12], LineBuf.param[13], LineBuf.param[14]);
                break;
            }
            case 251:
            case 253:
            case 255:
            case 265:
            case 266:
            {
                sprintf(PrintBuf, "[Users] %s\n", LineBuf.param[1]);
                break;
            }
            case 252:
            case 254:
            {
                sprintf(PrintBuf, "[Users] %s %s\n", LineBuf.param[1], LineBuf.param[2]);
                break;
            }
            case 372:
            case 375:
            case 376:
            {
                sprintf(PrintBuf, "[MOTD] %s\n", LineBuf.param[1]);
                break;
            }
        
            default:
                kprintf("Error: Unhandled IRC CMD: %u", cmd);
                return;
            break;
        }     
    }

    u16 len = strlen(PrintBuf);
    for (u16 i = 0; i < len; i++) IRC_PrintChar(PrintBuf[i]);

    //kprintf("Printing string len = %u (e-3: $%X - e-2: $%X - e-1: $%X)", len, PrintBuf[len-3], PrintBuf[len-2], PrintBuf[len-1]);
}

void IRC_ParseString()
{
    u8 bBreak = FALSE;
    u16 it = 0;
    u8 seq = 0;

    // Warning: this kprintf causes crashes for some reason
    //kprintf("RXString: \"%s\" - len: %u", RXString, strlen(RXString));

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

                    strncpy(LineBuf.prefix, RXString+1, end-2);
                    it = end-1;
                }
                else
                {
                    u16 end = it+1;
                    while (RXString[end++] != '\0');

                    strncpy(LineBuf.param[seq-1], RXString+it+1, end-it-2);
                    
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
                    strncpy(LineBuf.command, RXString+it+1, end-it-2);
                    seq++;
                    it = end-1;
                }
                else if (RXString[it+1] != ':')
                {
                    strncpy(LineBuf.param[seq-1], RXString+it+1, end-it-2);
                    //kprintf("seq: %u - strlen: %u - str: %s", seq, strlen(param[seq-1]), LineBuf.param[seq-1]);
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
                    strncpy(LineBuf.command, RXString+it, end-it-1);
                    seq++;
                    //it = end-1;
                }

                break;
            }
        }

        if (seq >= 16) break;

        it++;
    }

    #ifndef NO_LOGGING
    kprintf("Prefix: \"%s\"", LineBuf.prefix);
    kprintf("Command: \"%s\"", LineBuf.command);
    for (u8 i = 0; i < 16; i++) if (strlen(LineBuf.param[i]) > 0) kprintf("Param[%u]: \"%s\"", i, LineBuf.param[i]);
    #endif

    IRC_DoCommand();
}

bool bFirstRun = TRUE;

void IRC_ParseRX(u8 byte)
{
    RXBytes++;

    if (bFirstRun)
    {
        TTY_SendString("NICK SMDUSER\n");   // \r\n
        TTY_SendString("USER SMDUSER m68k smdnet :Mega Drive\n");   // \r\n
        //kprintf("FirstRun message sent.");
        bFirstRun = FALSE;
    }
    
    switch (byte)
    {
        case 0x0A:  //line feed, new line
            RXString[RXStringSeq++] = '\0';
            LineBuf.prefix[0] = '\0';
            LineBuf.command[0] = '\0';
            for (u8 i = 0; i < 16; i++){LineBuf.param[i][0] = '\0';}
            IRC_ParseString();
            RXStringSeq = 0;
            CR_Set = FALSE;
        break;
        case 0x0D:  //carriage return
            CR_Set = TRUE;
        break;

        default:
            if (RXStringSeq < 1023) RXString[RXStringSeq++] = byte;
            else RXStringSeq--;
        break;
    }
}
