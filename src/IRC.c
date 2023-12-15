
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

typedef struct s_strbuf
{
    char String[33];
    u8 Seq;
    u8 Max;
} StrBuf;

void SB_Reset(StrBuf *sb)
{
    sb->String[0] = '\0';
    sb->String[32] = '\0';
    sb->Seq = 0;
    sb->Max = 32;
}

void SB_PushChar(StrBuf *sb, char c)
{
    if (sb->Seq >= sizeof(sb->String)-1) return;

    sb->String[sb->Seq++] = c;
}

//static char Command[16];
//u8 CMDSeq = 0;
StrBuf Prefix;
StrBuf Command;
StrBuf Param[15];
bool CR_Set = FALSE;

static char RXString[256];
static u16 RXStringSeq = 0;

void IRC_Init()
{
    TTY_Init(TRUE);
    vNewlineConv = 1;
    UTF8_Init();
    //TTY_SetFontSize(0);
    //bWrapAround = TRUE;
    //TTY_SetColumns(D_COLUMNS_80);

    SB_Reset(&Prefix);
    SB_Reset(&Command);
    for (u8 i = 0; i < 15; i++) SB_Reset(&Param[i]);

    /*strcpy(RXString, "a b c :d");
    char n[4];

    scan("%c %c %c :%c", n[0], n[1], n[2], n[3]);

    kprintf("%c %c %c :%c", n[0], n[1], n[2], n[3]);*/
}

void IRC_ParseCommand()
{
    kprintf("Parse: \"%s\"", Command.String);

    if (strcmp("PING", Command.String) == 0)
    {
        TTY_SendString("PONG\r\n");
        kprintf("Recieved PING. Sending PONG");
    }

    SB_Reset(&Command);
}

void IRC_ParseString()
{
    u8 bBreak = FALSE;
    u8 bEndOfParam = FALSE;
    u16 StrStart[16] = {0};
    u16 StrEnd[16] = {0};
    u16 i = 0;
    u16 s = 0;
    u16 msg = 0;
    u16 strl = 0;
    u16 CMDEnd = 0;

    kprintf("RXString: \"%s\"", RXString);

    while (!bBreak)
    {
        switch (RXString[i])
        {
            case ' ':
            {
                if (CMDEnd == 0) CMDEnd = i;

                if (RXString[i+1] == ':') 
                {
                    bEndOfParam = TRUE;
                    StrEnd[s++] = i;
                }
                
                if (!bEndOfParam) 
                {
                    StrEnd[s++] = i-1;
                    StrStart[s] = i+1;
                }

                break;
            }
            case ':':
            {
                msg = i+1;
                break;
            }
            case '\0':
            {
                strl = i;
                kprintf("NUL: %u", i);
                bBreak = TRUE;
                break;
            }

            default:
            break;
        }

        i++;
    }
    
    char cmd[16];
    char str[16];
    char param[16][16];
    strncpy(cmd, RXString, CMDEnd);
    strncpy(str, RXString+msg, strl-msg);

    for (u8 j = 0; j < s; j++)
    {
        if (StrStart[j] == 0) continue;

        strncpy(param[j], RXString+StrStart[j], StrEnd[j]-StrStart[j]);
        
        kprintf("space[%u]: %u - param[%u] = \"%s\" - (%u -> %u)", j, StrStart[j], j, param[j], StrStart[j], StrEnd[j]);
    }
    
    kprintf("msg: %u -> %u = \"%s\"", msg, strl, str);
    kprintf("CMD: \"%s\"", cmd);
}

void IRC_TestStr(const char *channel, const char *message)
{
    char buf[32];
    sprintf(buf, "PRIVMSG %s :%s", channel, message);
    TTY_SendString(buf);
}

bool bFirstRun = TRUE;

void IRC_ParseRX(u8 byte)
{
    RXBytes++;

    if (bFirstRun)
    {
        TTY_SendString("NICK SMDUSER\r\n");
        TTY_SendString("USER SMDUSER m68k smdnet :Mega Drive\r\n");
        //kprintf("FirstRun message sent.");
        bFirstRun = FALSE;
    }
    
    switch (byte)
    {
        case 0x0A:  //line feed, new line
            IRC_ParseString();
            RXStringSeq = 0;
            CR_Set = FALSE;
        break;
        case 0x0D:  //carriage return
            CR_Set = TRUE;
        break;

        default:
            //SB_PushChar(&Command, byte);
            RXString[RXStringSeq++] = byte;
        break;
    }

    /*if (bUTF8)
    {
        DoUTF8(*PRX);
        return;
    }

    switch (*PRX)
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
            PAL_setColor(0, 0x666);
            waitMs(100);
            PAL_setColor(0, 0x000);
            waitMs(100);
            PAL_setColor(0, 0x666);
            waitMs(100);
            PAL_setColor(0, 0x000);
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

    if ((*PRX >= 0x20) && (*PRX <= 0x7E))
    {
        TTY_PrintChar(*PRX);
        
        return;
    }*/
}
