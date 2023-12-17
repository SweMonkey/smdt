
#include "IRC.h"
#include "Terminal.h"
#include "UTF8.h"

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
/*static struct s_linebuf
{
    char *prefix;
    char *command;
    char *param[16];
} LineBuf;*/

bool CR_Set = FALSE;

//static char RXString[1024];
static char *RXString = NULL;
static u16 RXStringSeq = 0;


void IRC_Init()
{
    TTY_Init(TRUE);
    vNewlineConv = 1;
    UTF8_Init();
    //TTY_SetFontSize(0);
    //bWrapAround = TRUE;
    //TTY_SetColumns(D_COLUMNS_80);

    RXString = (char*)MEM_alloc(1024);

    /*LineBuf.prefix = (char*)MEM_alloc(256);
    LineBuf.command = (char*)MEM_alloc(64);
    for (u8 i = 0; i < 16; i++) LineBuf.param[i] = (char*)MEM_alloc(256);*/
}

void IRC_PrintChar(u8 c)
{
    switch (c)
    {
        case 0x00:  //null
        break;
        case 0x01:  //start of heading
        break;
        case 0x02:  //start of text
        break;
        case 0x03:  //end of text
        break;
        case 0x04:  //end of transmission
        break;
        case 0x05:  //enquiry
        break;
        case 0x06:  //acknowledge
        break;
        case 0x07:  //bell
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
                sx = 0;
                TTY_MoveCursor(TTY_CURSOR_DUMMY);   // Dummy
            }

            EvenOdd = (sx % 2);
            EvenOdd = !EvenOdd;

            TTY_MoveCursor(TTY_CURSOR_DOWN, 1);
            TTY_ClearLine(sy % 32, 4);
        break;
        case 0x0B:  //vertical tab
            TTY_MoveCursor(TTY_CURSOR_DOWN, C_VTAB);
        break;
        case 0x0C:  //form feed, new page
            sx = 0;
            sy = C_YSTART;

            HScroll = D_HSCROLL;
            VScroll = D_VSCROLL;

            VDP_clearPlane(BG_A, TRUE);
            VDP_clearPlane(BG_B, TRUE);

            VDP_setVerticalScrollVSync(BG_A, VScroll);
            VDP_setVerticalScrollVSync(BG_B, VScroll);

            TTY_MoveCursor(TTY_CURSOR_DUMMY);   // Dummy
        break;
        case 0x0D:  //carriage return
            sx = 0;
            EvenOdd = (sx % 2);
            EvenOdd = !EvenOdd;
            TTY_MoveCursor(TTY_CURSOR_DUMMY);   // Dummy
        break;
        case 0xE2:  // Dumb handling of UTF8
            bUTF8 = TRUE;
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

static inline u16 atoi(char *c)
{
    /*u16 r = 0;

    for (u8 i = 0; c[i] != '\0'; ++i)
    {
        r = r * 10 + c[i] - '0';
    }

    return r;*/

    u16 value = 0;

    while (isdigit(*c)) 
    {
        value *= 10;
        value += (u16) (*c - '0');
        c++;
    }

    return value;
}

// IRC/2 cmd numbers: https://www.alien.net.au/irc/irc2numerics.html
void IRC_DoCommand()
{
    char buf[256];

    if (strcmp(LineBuf.command, "PING") == 0)
    {
        sprintf(buf, "PONG %s\n", LineBuf.param[0]);
        TTY_SendString(buf);
        //kprintf("Got ping. Response: \"%s\"", buf);
    }
    else if (strcmp(LineBuf.command, "NOTICE") == 0)
    {
        sprintf(buf, "[Notice]: %s %s\r\n", LineBuf.prefix, LineBuf.param[1]);
        u16 len = strlen(buf);
        for (u16 i = 0; i < len; i++) IRC_PrintChar(buf[i]);
    }
    else if (strcmp(LineBuf.command, "ERROR") == 0)
    {
        sprintf(buf, "[Error]: %s %s\r\n", LineBuf.prefix, LineBuf.param[0]);
        u16 len = strlen(buf);
        for (u16 i = 0; i < len; i++) IRC_PrintChar(buf[i]);
    }
    else
    {
        u16 cmd = atoi(LineBuf.command);
        //kprintf("Got CMD: %u", cmd);

        switch (cmd)
        {
            case 0:
            {
                break;
            }
            case 1:
            case 2:
            case 3:
            case 4:
            {
                sprintf(buf, "[Welcome]: %s\r\n", LineBuf.param[1]);
                break;
            }
            case 5:
            {
                sprintf(buf, "[Support]: %s\r\n", LineBuf.param[1]);
                break;
            }
            case 251:
            case 252:
            case 253:
            case 254:
            case 255:
            case 265:
            case 266:
            {
                sprintf(buf, "[Users]: %s\r\n", LineBuf.param[1]);
                break;
            }
            case 372:
            case 375:
            case 376:
            {
                sprintf(buf, "[MOTD]: %s\r\n", LineBuf.param[1]);
                break;
            }
        
            default:
                //kprintf("Error: Unhandled IRC CMD: %u", cmd);
            break;
        }
        
        u16 len = strlen(buf);
        for (u16 i = 0; i < len; i++) IRC_PrintChar(buf[i]);        
    }
}

void IRC_ParseString()
{
    u8 bBreak = FALSE;
    u16 it = 0;
    u8 seq = 0;

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

        //kprintf("RxString iterator: %u", it);
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
            //kprintf("$0A - Parsing string... Seq = %u", RXStringSeq);
            RXString[RXStringSeq++] = '\0';
            IRC_ParseString();
            RXStringSeq = 0;
            CR_Set = FALSE;
        break;
        case 0x0D:  //carriage return
            //kprintf("$0D - Returning...");
            CR_Set = TRUE;
        break;

        default:
            //SB_PushChar(&Command, byte);
            //kprintf("Defaulting... Seq = %u", RXStringSeq);
            if (RXStringSeq < 1023) RXString[RXStringSeq++] = byte;
            else RXStringSeq--;
        break;
    }
}
