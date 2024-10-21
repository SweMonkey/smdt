#include "StateCtrl.h"
#include "Network.h"
#include "Terminal.h"
#include "Telnet.h"
#include "Cursor.h"
#include "system/Stdout.h"
#include "Utils.h"
#include "Input.h"

#ifdef EMU_BUILD
#include "kdebug.h"
asm(".global gopherdump\ngopherdump:\n.incbin \"tmp/streams/rx_floodgap2.log\"");
extern const unsigned char gopherdump[];
#endif

// gopherrendertest.txt
// rx_gopher_floodgap.log
// rx_floodgap2.log

//#ifndef EMU_BUILD
static u8 rxdata;
static u8 rxdata1;
static u8 rxdata2;
//#endif

#define B_LINEBUF_LEN 200
#define B_PAGEROWS 180
#define B_PAGEBUF_LEN B_PAGEROWS*(B_LINEBUF_LEN-128) // xx rows * yy columns
#define B_LINK_LEN 128
#define SCR_UP 0
#define SCR_DOWN 1

static u8 bOnce = FALSE;

static char *LineBuf = NULL;    // Line buffering string
static u8 *PageBuffer = NULL;   // Page buffer, contains the entire received document
static u16 *LFPos = NULL;       // Positions of all line feeds in document
static u16 PPos = 0;            // PageBuffer head position during buffering
static u8 LFCount = 0;          // Line feed count, (=number of rows)
static s16 ScrollPos = 27;      // Page scroll position. Do not adjust here! Resets to 'ScrHeight' during init!
static s16 ScrHeight = 27;      // 27= NTSC - 29= PAL. Do not adjust here! Resets to proper value during init!
static u16 PointerY = 0;        // Mouse pointer X (in pixels, 0-304)
static u16 PointerX = 0;        // Mouse pointer Y (in tiles, 0-Page height)
static u8 PointerY_A = 0;       // Mouse pointer clipped Y (in tiles, 0-27 NTSC - 0-29 PAL)

static char glink[B_LINK_LEN];  // Global link address
static char gserv[64];          // Global server address
static char gport[8];           // Global port number

static bool bPageDone = FALSE;

void Gopher_Init();                         // Init/Reset gopher client so it is ready to render a new page
void Gopher_Scroll(u8 dir);                 // Scroll page up/down 1 row
void Gopher_GetPage();                      // Connect/Get page from server (using 'gserv', 'gport', 'glink' values)
void Gopher_GetAddress();                   // Split 'LineBuf' into 'glink', 'gserv', 'gport'
void Gopher_GetAddressFromStr(char *from);  // Split 'from' into 'glink', 'gserv', 'gport'
void Gopher_ParseString();                  // Parse gopher document line
void Gopher_BufferByte(u8 byte);            // Buffers incoming bytes into 'PageBuffer' and fills in LF positions
void Gopher_PrintLine(u16 start_line, u8 num_lines); // Print and format gopher document line(s) to screen
void Gopher_PrintString(const char *str);   // Print string to screen
void Gopher_PrintChar(u8 c);                // Print character to screen


void Enter_Gopher(u8 argc, char *argv[])
{
    sv_Font = FONT_4x8_8;
    TTY_Init(TRUE);

    // Allocate LineBuf buffer
    LineBuf = malloc(B_LINEBUF_LEN);
    if (LineBuf == NULL) 
    {
        #ifdef GOP_LOGGING
        kprintf("Failed to allocate memory for LineBuf");
        #endif

        Stdout_Push("[91mGopher Client: Failed to allocate memory\n for LineBuf![0m\n");
        RevertState();
        return;
    }
    //memset(LineBuf, 0, B_LINEBUF_LEN);

    // Allocate page buffer
    PageBuffer = malloc(B_PAGEBUF_LEN);
    if (PageBuffer == NULL) 
    {
        #ifdef GOP_LOGGING
        kprintf("Failed to allocate memory for PageBuffer");
        #endif

        Stdout_Push("[91mGopher Client: Failed to allocate memory for PageBuffer![0m\n");
        stdout_printf("[91mFree: %u - LFree: %u - Needed: %u[0m\n", MEM_getFree(), MEM_getLargestFreeBlock(), B_PAGEBUF_LEN);
        RevertState();
        return;
    }
    //memset(PageBuffer, 0, B_PAGEBUF_LEN);

    // Allocate linefeed buffer
    LFPos = malloc(B_PAGEROWS*2);
    if (LFPos == NULL) 
    {
        #ifdef GOP_LOGGING
        kprintf("Failed to allocate memory for LFPos");
        #endif

        Stdout_Push("[91mGopher Client: Failed to allocate memory\n for LFPos![0m\n");
        RevertState();
        return;
    }
    //memset(LFPos, 0, B_PAGEROWS*2);
    //LFPos[LFCount++] = 0;   // First entry should point to the start of the PageBuffer
    
    PointerY = 14;
    PointerY_A = 14;
    PointerX = 160;

    SetSprite_Y(SPRITE_ID_POINTER, (PointerY_A*8)+142);
    SetSprite_SIZELINK(SPRITE_ID_POINTER, SPR_SIZE_1x1, 0);
    SetSprite_TILE(SPRITE_ID_POINTER, 0x6017);  // 0x6000= Inverted mouse, 0x2000= Normal mouse
    SetSprite_X(SPRITE_ID_POINTER, PointerX+128);

    // Hide terminal cursor
    SetSprite_TILE(SPRITE_ID_CURSOR, 0x16);

    // Variable overrides
    vDoEcho = 0;
    vLineMode = LMSM_EDIT;
    bDoCursorBlink = FALSE;
    sv_bWrapAround = FALSE;
    C_XMAX = 127;//(sv_Font ? 254 : 126);
    ScrHeight = bPALSystem ? 29 : 27;

    Gopher_Init();
    Gopher_GetAddressFromStr(argv[1]);
    Gopher_GetPage();

    // Debug playback/timing of a logged stream
    #ifdef EMU_BUILD
    u8 data;
    u32 p = 0;
    u32 s = 5589;//5586;//241;//0x671;
    KDebug_StartTimer();
    while (p < s)
    {
        while(Buffer_Push(&RxBuffer, gopherdump[p]) != 0xFF)
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
            Gopher_BufferByte(data);
            
            if (!bOnce)
            {
                TRM_SetStatusIcon(ICO_NET_IDLE_RECV, ICO_POS_1);
                bOnce = !bOnce;
            }

            if ((data == '.') && (rxdata1 == 0xA) && (rxdata2 == 0xD))
            {
                kprintf("Final dot found @ %lu - data= '%c'", p, data);
                Gopher_PrintLine(0, ScrHeight);
            }

            rxdata2 = rxdata1;
            rxdata1 = data;
        }
    }
    //Gopher_PrintLine(0, ScrHeight);
    KDebug_StopTimer();
    #endif
}

void ReEnter_Gopher()
{
}

void Exit_Gopher()
{
    free(LineBuf);
    LineBuf = NULL;

    free(PageBuffer);
    PageBuffer = NULL;
    
    free(LFPos);
    LFPos = NULL;

    bDoCursorBlink = TRUE;
    
    // Show terminal cursor
    SetSprite_TILE(SPRITE_ID_CURSOR, LastCursor);

    //Stdout_Flush();
    Buffer_Flush(&TxBuffer);
    NET_Disconnect();
}

void Reset_Gopher()
{
}

void Run_Gopher()
{
    if (bPageDone) return;
    
    #ifndef EMU_BUILD
    while (Buffer_Pop(&RxBuffer, &rxdata) != 0xFF)
    {
        Gopher_BufferByte(rxdata);

        if (bOnce)
        {
            TRM_SetStatusIcon(ICO_NET_RECV, ICO_POS_1);
            bOnce = !bOnce;
        }

        if ((rxdata == '.') && (rxdata1 == 0xA) && (rxdata2 == 0xD))
        {
            //kprintf("Final dot found @ %lu - data= '%c'", p, data);
            Gopher_PrintLine(0, ScrHeight);
            bPageDone = TRUE;
        }

        rxdata2 = rxdata1;
        rxdata1 = rxdata;
    }

    if (!bOnce)
    {
        TRM_SetStatusIcon(ICO_NET_IDLE_RECV, ICO_POS_1);
        bOnce = !bOnce;
    }
    #endif
}

void Input_Gopher()
{
    if (!bWindowActive)
    {
        if (is_KeyDown(KEY_F5))
        {
            Gopher_PrintLine(0, ScrHeight);
        }
        if (is_KeyDown(KEY_F6))
        {
            NET_SendString("/\r\n");
        }

        if (is_KeyDown(KEY_PGUP))
        {
        }
        if (is_KeyDown(KEY_PGDN))
        {
        }

        if (is_KeyDown(KEY_UP))
        {
            if (PointerY > 0)
            {
                PointerY--;
            }

            if (PointerY_A > 0)
            {
                PointerY_A--;
                SetSprite_Y(SPRITE_ID_POINTER, (PointerY_A*8)+142);
            }
            else 
            {
                Gopher_Scroll(SCR_UP);
            }
        }
        if (is_KeyDown(KEY_DOWN))
        {
            if (PointerY < LFCount-2)
            {
                PointerY++;
            }

            if (PointerY_A < (bPALSystem?28:26)) // < 26
            {
                PointerY_A++;
                SetSprite_Y(SPRITE_ID_POINTER, (PointerY_A*8)+142);
            }
            else 
            {
                Gopher_Scroll(SCR_DOWN);
            }
        }
        if (is_KeyDown(KEY_LEFT))
        {
            if (PointerX >= 8)
            {
                PointerX -= 8;
                SetSprite_X(SPRITE_ID_POINTER, PointerX+128);
            }
        }
        if (is_KeyDown(KEY_RIGHT))
        {
            if (PointerX <= 304)
            {
                PointerX += 8;
                SetSprite_X(SPRITE_ID_POINTER, PointerX+128);
            }
        }

        if (is_KeyDown(KEY_RETURN))
        {
            u16 start = LFPos[PointerY];
            u16 end = LFPos[PointerY+1]-1;

            memset(LineBuf, 0, B_LINEBUF_LEN);
            memcpy(LineBuf, PageBuffer+start, end-start-1);

            //kprintf("PointerY= %u -- PointerY_A= %u -- SY_A= %ld -- SY= %ld -- LFCount= %u", PointerY, PointerY_A, TTY_GetSY_A(), TTY_GetSY(), LFCount);
            //kprintf("LineBuf= \"%s\"", LineBuf);

            if ((LineBuf[0] == '0') || (LineBuf[0] == '1') || (LineBuf[0] == '2') || (LineBuf[0] == '7'))
            {
                Gopher_GetAddress();
                Gopher_Init();
                Gopher_GetPage();
            }
        }
    }
}

void Gopher_Init()
{
    PPos = 0;
    LFCount = 0;
    PointerY = PointerY % ScrHeight;
    //PointerX = 0;
    //PointerY_A = 0;

    memset(LineBuf, 0, B_LINEBUF_LEN);
    memset(PageBuffer, 0, B_PAGEBUF_LEN);
    memset(LFPos, 0, B_PAGEROWS*2);
    LFPos[LFCount++] = 0;   // First entry should point to the start of the PageBuffer
    SetSprite_Y(SPRITE_ID_POINTER, (PointerY_A*8)+142);
    SetSprite_X(SPRITE_ID_POINTER, PointerX+128);
    ScrollPos = ScrHeight;

    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);

    VScroll = 0;

    // Update vertical scroll
    *((vu32*) VDP_CTRL_PORT) = 0x40000010;
    *((vu16*) VDP_DATA_PORT) = VScroll;
    *((vu32*) VDP_CTRL_PORT) = 0x40020010;
    *((vu16*) VDP_DATA_PORT) = VScroll;

    bPageDone = FALSE;
}

void Gopher_Scroll(u8 dir)
{
    s32 old_sy = TTY_GetSY();

    if (dir == SCR_UP)
    {
        if (ScrollPos <= ScrHeight) return;

        ScrollPos--;
        TTY_SetSY(old_sy + ScrollPos-(bPALSystem?26:22));   // -22
        TTY_ClearLineSingle(sy);
        VScroll -= 8;

        Gopher_PrintLine(ScrollPos-ScrHeight, 1);
    }
    else if (dir == SCR_DOWN)
    {
        if ((ScrollPos >= LFCount-1)) return;

        ScrollPos++;
        TTY_SetSY(old_sy + ScrollPos+(bPALSystem?2:4));    // +4

        if (ScrollPos > ScrHeight)
        {
            TTY_ClearLineSingle(sy);
            VScroll += 8;
        }

        Gopher_PrintLine(ScrollPos-1, 1);
    }
    else return;

    // Update vertical scroll
    *((vu32*) VDP_CTRL_PORT) = 0x40000010;
    *((vu16*) VDP_DATA_PORT) = VScroll;
    *((vu32*) VDP_CTRL_PORT) = 0x40020010;
    *((vu16*) VDP_DATA_PORT) = VScroll;

    TTY_SetSY(old_sy);
    TTY_MoveCursor(TTY_CURSOR_DUMMY);
}

void Gopher_GetPage()
{
    char TitleBuf[32];
    char addr[64];
    char link[B_LINK_LEN];
    snprintf(addr, 64, "%s:%s", gserv, gport);
    snprintf(link, B_LINK_LEN, "%s\r\n", glink);
    //kprintf("Connect addr= \"%s\" -- link= \"%s\"", addr, link);

    NET_Connect(addr);
    //if (NET_Connect(addr))
    {
        snprintf(TitleBuf, 29, "%s %-21s", STATUS_TEXT_SHORT, gserv);
        TRM_SetStatusText(TitleBuf);
        NET_SendString(link);
    }    
}

void Gopher_GetAddress()
{
    u16 it = 0;
    u16 it_link = 0;
    u16 it_serv = 0;
    u16 it_port = 0;

    while ((LineBuf[it] != 0) && (LineBuf[it] != 0x09)) it++;

    it_link = it+1;
    while ((LineBuf[it_link] != 0) && (LineBuf[it_link] != 0x09)) it_link++;
    it_serv = it_link+1;
    while ((LineBuf[it_serv] != 0) && (LineBuf[it_serv] != 0x09)) it_serv++;
    it_port = it_serv+1;
    while ((LineBuf[it_port] != 0) && (LineBuf[it_port] != 0x0D)) it_port++;

    strncpy(glink, LineBuf+it+1, it_link-it-1);
    strncpy(gserv, LineBuf+it_link+1, it_serv-it_link-1);
    strncpy(gport, LineBuf+it_serv+1, it_port-it_serv-1);

    if (strlen(glink) == 0) {strcpy(glink, "/");}
    if (strlen(gport) == 0) {strcpy(gport, "70");}

    /*kprintf("link= \"%s\"", glink);
    kprintf("serv= \"%s\"", gserv);
    kprintf("port= \"%s\"", gport);
    kprintf("full= \"%s:%s%s\"", gserv, gport, glink);*/
}

void Gopher_GetAddressFromStr(char *from)
{
    u16 it_link = 0;
    u16 it_serv = 0;
    u16 it_port = 0;

    while ((from[it_serv] != 0) && (from[it_serv] != ':') && (from[it_serv] != '/')) it_serv++;

    if (from[it_serv] == ':')
    {
        it_port = it_serv+1;
        while ((from[it_port] != 0) && (from[it_port] != '/')) it_port++;

        strncpy(gport, from+it_serv+1, it_port-it_serv-1);
    }
    else
    {
        strcpy(gport, "70");
        it_port = it_serv;
    }
    
    it_link = it_port+1;
    while (from[it_link] != 0) it_link++;

    strncpy(gserv, from, it_serv);
    strncpy(glink, from+it_port, it_link-it_port);

    if (strlen(glink) == 0) {strcpy(glink, "/");}

    /*kprintf("link= \"%s\"", glink);
    kprintf("serv= \"%s\"", gserv);
    kprintf("port= \"%s\"", gport);
    kprintf("full= \"%s:%s%s\" -- %c", gserv, gport, glink, from[it_serv]);*/
}

void Gopher_ParseString()
{
    u16 it = 1;
    u8 type = LineBuf[0];

    //kprintf("Type= %u", type);

    switch (type)
    {
        case '.':   // End of transmission
        break;

        case 'i':   // Text
            while ((LineBuf[it] != 0) && (LineBuf[it] != 0x09)) Gopher_PrintChar(LineBuf[it++]);
        break;

        case '0':   // Link (TXT)
            Gopher_PrintString("(TXT) ");
            TTY_SetAttribute(94);

            while ((LineBuf[it] != 0) && (LineBuf[it] != 0x09)) Gopher_PrintChar(LineBuf[it++]);

            TTY_SetAttribute(0);
        break;

        case '1':   // Link (DIR)
            Gopher_PrintString("(DIR) ");
            TTY_SetAttribute(94);
            
            while ((LineBuf[it] != 0) && (LineBuf[it] != 0x09)) Gopher_PrintChar(LineBuf[it++]);

            TTY_SetAttribute(0);
        break;

        case '2':   // Phonebook (PHO)
            Gopher_PrintString("(PHO) ");
            TTY_SetAttribute(94);
            
            while ((LineBuf[it] != 0) && (LineBuf[it] != 0x09)) Gopher_PrintChar(LineBuf[it++]);

            TTY_SetAttribute(0);
        break;

        case '7':   // Query (QRY)
            Gopher_PrintString("(QRY) ");
            TTY_SetAttribute(94);
            
            while ((LineBuf[it] != 0) && (LineBuf[it] != 0x09)) Gopher_PrintChar(LineBuf[it++]);

            TTY_SetAttribute(0);
        break;

        case 'h':   // HTML (HTM)
            Gopher_PrintString("(HTM) ");
            TTY_SetAttribute(91);
            
            while ((LineBuf[it] != 0) && (LineBuf[it] != 0x09)) Gopher_PrintChar(LineBuf[it++]);

            TTY_SetAttribute(0);
        break;
    
        default:
            Gopher_PrintString("( ? ) ");
            while ((LineBuf[it] != 0) && (LineBuf[it] != 0x09) && (LineBuf[it] != 0x0D)) Gopher_PrintChar(LineBuf[it++]);
        break;
    }

    memset(LineBuf, 0, B_LINEBUF_LEN);
}

void Gopher_BufferByte(u8 byte)
{
    RXBytes++;

    PageBuffer[PPos++] = byte;

    if (byte == '\n') LFPos[LFCount++] = PPos;
}

void Gopher_PrintLine(u16 start_line, u8 num_lines)
{
    s32 old_sx = TTY_GetSX();

    if (PageBuffer[0] == 0) return;

    for (u8 line = 0; line < num_lines; line++)
    {
        u16 start = LFPos[start_line+line];
        u16 end = LFPos[start_line+line+1]-1;

        memcpy(LineBuf, PageBuffer+start, end-start-1);

        TTY_SetSX(0);
        Gopher_ParseString();
        if (num_lines > 1) TTY_SetSY_A(TTY_GetSY_A()+1);
    }
    
    TTY_SetSX(old_sx);
}

void Gopher_PrintString(const char *str)
{
    u16 i = 0;
    while (str[i] != '\0') Gopher_PrintChar(str[i++]);
}

void Gopher_PrintChar(u8 c)
{
    if ((c >= 0x20) && (c <= 0x7E))
    {
        TTY_PrintChar(c);
        
        return;
    }
}

const PRG_State GopherState = 
{
    Enter_Gopher, ReEnter_Gopher, Exit_Gopher, Reset_Gopher, Run_Gopher, Input_Gopher, NULL, NULL
};

