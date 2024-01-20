
#include "Telnet.h"
#include "Terminal.h"
#include "UTF8.h"
#include "Utils.h"
#include "Network.h"

// https://vt100.net/docs/vt100-ug/chapter3.html
// https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
// http://www.braun-home.net/michael/info/misc/VT100_commands.htm

// https://www.iana.org/assignments/telnet-options/telnet-options.xhtml
// https://users.cs.cf.ac.uk/Dave.Marshall/Internet/node141.html
// https://www.omnisecu.com/tcpip/telnet-commands-and-options.php
// Telnet IAC commands
#define TC_IAC 255
#define TC_DONT 254
#define TC_DO 253
#define TC_WONT 252
#define TC_WILL 251 // $FB
#define TC_SB 250   // $FA Begin of subnegotiation
#define TC_GA 249
#define TC_EL 248
#define TC_EC 247
#define TC_AYT 246
#define TC_AO 245
#define TC_IP 244
#define TC_BRK 243
#define TC_DM 242
#define TC_NOP 241
#define TC_SE 240   // $F0 End of subnegotiation parameters

// Telnet IAC options
#define TO_ENV_OP 39

#define TO_ENV 36
#define TO_XDISP 35
#define TO_LINEMODE 34
#define TO_RFLOW_CTRL 33
#define TO_TERM_SPEED 32
#define TO_NAWS 31  // Negotiation About Window Size

#define TO_USER_IDENT 26
#define TO_END_REC 25
#define TO_TERM_TYPE 24 // $18
#define TO_SEND_LOCATION 23 // $17

#define TO_BYTE_MACRO 19
#define TO_LOGOUT 18
#define TO_EXT_ASCII 17
#define TO_OUT_LINEFEED_DISPOS 16
#define TO_OUT_VTAB_DISPOS 15
#define TO_OUT_VTABSTOPS 14
#define TO_OUT_FORMFEED_DISPOS 13
#define TO_OUT_HTAB_DISPOS 12
#define TO_OUT_HTABSTOPS 11
#define TO_OUT_CR_DISPOS 10 // Carriage-Return disposition
#define TO_OUT_PAGESIZE 9
#define TO_OUT_LINEWIDTH 8

#define TO_TIMING_MARK 6
#define TO_STATUS 5
#define TO_APPROX_MSG_SZ_NEGOTIATION 4
#define TO_SUPPRESS_GO_AHEAD 3
#define TO_RECONNECTION
#define TO_ECHO 1
#define TO_BIN_TRANS 0

// Telnet Linemode mode commands
#define LM_MODE 1
#define LM_FORWARDMASK 2
#define LM_SLC 3

// Telnet subnegotiations commands
#define TS_SEND 1
#define TS_IS 0

static inline void DoEscape(u8 dummy);
static inline void DoIAC(u8 dummy);

// Telnet modifiable variables
u8 vDoGA = 0;
u8 vDECOM = FALSE;  // DEC Origin Mode

// DECSTBM
static s16 DMarginTop = 0;
static s16 DMarginBottom = 0;

// Escapes [
u8 bESCAPE = FALSE;         // If true: an escape code was recieved last byte
u8 ESC_Seq = 0;
u8 ESC_Type = 0;
u8 ESC_Param[4] = {0xFF,0xFF,0xFF,0xFF};
u8 ESC_ParamSeq = 0;
char ESC_Buffer[4] = {'\0','\0','\0','\0'};
u8 ESC_BufferSeq = 0;

u8 ESC_QBuffer[6];
u8 ESC_QSeq = 0;
u16 QSeqNumber = 0; // atoi'd ESC_QBuffer

// IAC
u8 bIAC = FALSE;                        // TRUE = Currently in a "Intercept As Command" stream
u8 IAC_Command = 0;                     // Current TC_xxx command (0 = none set)
u8 IAC_Option = 0xFF;                   // Current TO_xxx option  (FF = none set)
u8 IAC_InSubNegotiation = 0;            // TRUE = Currently in a SB/SE block
u8 IAC_SubNegotiationOption = 0xFF;     // Current TO_xxx option to operate in a subnegotiation 
u8 IAC_SNSeq = 0;                       // Counter - where in "IAC_SubNegotiationBytes" we are
u8 IAC_SubNegotiationBytes[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};   // Recieved byte stream in a subnegotiation block

extern const char *TermTypeList[];


void TELNET_Init()
{
    TTY_Init(TRUE);
    UTF8_Init();

    bESCAPE = FALSE;
    ESC_Seq = 0;
    ESC_Type = 0;
    ESC_Param[0] = 0xFF;
    ESC_Param[1] = 0xFF;
    ESC_Param[2] = 0xFF;
    ESC_Param[3] = 0xFF;
    ESC_ParamSeq = 0;
    ESC_Buffer[0] = '\0';
    ESC_Buffer[1] = '\0';
    ESC_Buffer[2] = '\0';
    ESC_Buffer[3] = '\0';
    ESC_BufferSeq = 0;

    bIAC = FALSE;
    IAC_Command = 0;
    IAC_Option = 0xFF;
    IAC_InSubNegotiation = 0;
    IAC_SubNegotiationOption = 0xFF;
    IAC_SNSeq = 0;
    IAC_SubNegotiationBytes[0] = 0;
    IAC_SubNegotiationBytes[1] = 0;
    IAC_SubNegotiationBytes[2] = 0;
    IAC_SubNegotiationBytes[3] = 0;

    vDoGA = 0;
    vDECOM = FALSE;

    DMarginTop = 0;
    DMarginBottom = bPALSystem?0x1D:0x1B;

    // Variable overrides
    vDoEcho = 0;
    vLineMode = 0;
    vNewlineConv = 0;
    bWrapAround = TRUE;
}

inline void TELNET_ParseRX(u8 dummy)
{
    vu8 *PRX = &dummy;

    RXBytes++;

    if (bIAC)
    {
        DoIAC(*PRX);
        return;
    }

    if (bESCAPE)
    {
        DoEscape(*PRX);
        return;
    }

    if (bUTF8)
    {
        DoUTF8(*PRX);
        return;
    }

    switch (*PRX)
    {
        case 0x00:  // Null
        break;
        case 0x01:  // Start of heading
        break;
        case 0x02:  // Start of text
        break;
        case 0x03:  // End of text
        break;
        case 0x04:  // End of transmission
        break;
        case 0x05:  // Enquiry
        break;
        case 0x06:  // Acknowledge
        break;
        case 0x07:  // Bell
            PSG_setEnvelope(0, PSG_ENVELOPE_MAX);
            waitMs(100);
            PSG_setEnvelope(0, PSG_ENVELOPE_MIN);
            waitMs(100);
            PSG_setEnvelope(0, PSG_ENVELOPE_MAX);
            waitMs(100);
            PSG_setEnvelope(0, PSG_ENVELOPE_MIN);
            waitMs(100);
            PSG_setEnvelope(0, PSG_ENVELOPE_MAX);
            waitMs(100);
            PSG_setEnvelope(0, PSG_ENVELOPE_MIN);
            waitMs(100);
            PSG_setEnvelope(0, PSG_ENVELOPE_MAX);
            waitMs(100);
            PSG_setEnvelope(0, PSG_ENVELOPE_MIN);
        break;
        case 0x08:  // Backspace
            TTY_MoveCursor(TTY_CURSOR_LEFT, 1);
        break;
        case 0x09:  // Horizontal tab
            TTY_MoveCursor(TTY_CURSOR_RIGHT, C_HTAB);
        break;
        case 0x0A:  // Line feed (new line)
            if (vNewlineConv == 1)  // Convert \n to \n\r
            {
                TTY_SetSX(0);
            }
            TTY_MoveCursor(TTY_CURSOR_DOWN, 1);
        break;
        case 0x0B:  // Vertical tab
            TTY_MoveCursor(TTY_CURSOR_DOWN, C_VTAB);
        break;
        case 0x0C:  // Form feed (new page)
            TTY_SetSX(0);
            TTY_SetSY(C_YSTART);

            VScroll = D_VSCROLL;

            VDP_clearPlane(BG_A, TRUE);
            VDP_clearPlane(BG_B, TRUE);

            VDP_setVerticalScroll(BG_A, VScroll);
            VDP_setVerticalScroll(BG_B, VScroll);

            TTY_MoveCursor(TTY_CURSOR_DUMMY);   // Dummy
        break;
        case 0x0D:  // Carriage return
            TTY_SetSX(0);
            TTY_MoveCursor(TTY_CURSOR_DUMMY);   // Dummy
        break;
        case 0x15:  // NAK (negative acknowledge)
        break;
        case 0x1B:  // Escape 1
            bESCAPE = TRUE;
            ESC_Seq = 0;
            ESC_Type = 0;
            ESC_Param[0] = 0xFF;
            ESC_Param[1] = 0xFF;
            ESC_Param[2] = 0xFF;
            ESC_Param[3] = 0xFF;
            ESC_ParamSeq = 0;
            ESC_Buffer[0] = '\0';
            ESC_Buffer[1] = '\0';
            ESC_Buffer[2] = '\0';
            ESC_Buffer[3] = '\0';
            ESC_BufferSeq = 0;
            ESC_QBuffer[0] = 0;
            ESC_QBuffer[1] = 0;
            ESC_QBuffer[2] = 0;
            ESC_QBuffer[3] = 0;
            ESC_QBuffer[4] = 0;
            ESC_QBuffer[5] = 0;
            ESC_QSeq = 0;

            return;
        break;
        case 0xE2:  // Dumb handling of UTF8
        case 0xEF:
            bUTF8 = TRUE;
            return;
        break;
        case TC_IAC:  // IAC
            bIAC = TRUE;
            return;
        break;

        default:
        //if ((*PRX < 0x20) || (*PRX > 0x7E)) kprintf("TTY_ParseRX: Caught unhandled byte: $%X", *PRX);
        break;
    }

    /*
    Current font tiles (VRAM Tiles 0x9F-0x11F) do not match ANSI (Characters 0x80-0xFF)
    therefore only ANSI characters 0x20-0x7E is allowed for now (VRAM Tiles 0x40-0x9E)

    https://www.gaijin.at/en/infos/ascii-ansi-character-table
    http://www.alanwood.net/demos/ansi.html
    */
    if ((*PRX >= 0x20) && (*PRX <= 0x7E)) // <= 0x7E
    {
        TTY_PrintChar(*PRX);
        
        return;
    }
}

// https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
// https://en.wikipedia.org/wiki/ANSI_escape_code#CSIsection
static inline void DoEscape(u8 dummy)
{
    vu8 *PRX = &dummy;

    ESC_Seq++;

    if (ESC_Seq == 1)
    {
        ESC_Type = *PRX;    // '[' or ' '
        return;
    }

    // https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
    if (ESC_QSeq > 0)
    {
        ESC_QBuffer[ESC_QSeq-1] = *PRX;
        //kprintf("ESC_Q: $%X - '%c'", ESC_QBuffer[ESC_QSeq-1], (char)ESC_QBuffer[ESC_QSeq-1]);

        if (*PRX == 'h')
        {
            QSeqNumber = atoi16((char*)ESC_QBuffer);
            //kprintf("QSeqNumber = %u", QSeqNumber);

            switch (QSeqNumber)
            {
                case 6:   // Origin Mode (DECOM), VT100.
                vDECOM = TRUE;
                break;
            
                case 7:   // Auto-Wrap Mode (DECAWM), VT100.
                bWrapAround = TRUE;
                break;
            
                case 25:   // Shows the cursor, from the VT220. (DECTCEM)
                break;

                case 2004:   // Turn on bracketed paste mode. In bracketed paste mode, text pasted into the terminal will be surrounded by ESC [200~ and ESC [201~; programs running in the terminal should not treat characters bracketed by those sequences as commands (Vim, for example, does not treat them as commands). From xterm
                break;

                // 1000h = Send Mouse X & Y on button press and release.  See the section Mouse Tracking.  This is the X11 xterm mouse protocol.
                // 1006h = Enable SGR Mouse Mode, xterm.

                default:
                //kprintf("Got an unknown ?%ch", (char)ESC_QBuffer[0]);
                break;
            }

            goto EndEscape;
        }
        if (*PRX == 'l')
        {
            QSeqNumber = atoi16((char*)ESC_QBuffer);
            //kprintf("QSeqNumber = %u", QSeqNumber);

            switch (QSeqNumber)
            {
                case 6:   // Normal Cursor Mode (DECOM), VT100.
                vDECOM = FALSE;
                break;
            
                case 7:   // No Auto-Wrap Mode (DECAWM), VT100.
                bWrapAround = FALSE;
                break;
            
                case 25:   // Hides the cursor. (DECTCEM)
                break;

                case 2004:   // Turn off bracketed paste mode. 
                break;


                default:
                //kprintf("Got an unknown ?%cl", (char)ESC_QBuffer[0]);
                break;
            }

            goto EndEscape;
        }

        if (ESC_QSeq > 5) 
        {
            //for (u8 i = 0; i < 6; i++) kprintf("ESC_QBuffer[%u] = $%X (%c)", i, ESC_QBuffer[i], (char)ESC_QBuffer[i]);
            goto EndEscape;
        }

        ESC_QSeq++;

        return;
    }


    switch (*PRX)
    {
        case ';':
        {
            if (ESC_ParamSeq >= 4) return;

            ESC_Param[ESC_ParamSeq++] = atoi(ESC_Buffer);
            //kprintf("ESC_ParamSeq: %u = %u (%s)", (u8)ESC_ParamSeq-1, ESC_Param[ESC_ParamSeq-1], ESC_Buffer);
            //kprintf("Got an ';' : ESC_Param[%u] = $%X", ESC_ParamSeq-1, ESC_Param[ESC_ParamSeq-1]);

            ESC_Buffer[0] = '\0';
            ESC_Buffer[1] = '\0';
            ESC_Buffer[2] = '\0';
            ESC_Buffer[3] = '\0';
            ESC_BufferSeq = 0;

            return;
        }

        /*
        case '7':   // Save cursor position (DEC)
        {
            if (ESC_Type == ' ')
            {
                SavedCX = sx;
                SavedCY = sy;
                kprintf("Hit a '7' ($%X)", atoi(ESC_Buffer));
                goto EndEscape;
            }
        }

        case '8':   // Restores the cursor to the last saved position (DEC)
        {
            if (ESC_Type == ' ')
            {
                sx = SavedCX;
                sy = SavedCY;
                kprintf("Hit a '8' ($%X)", atoi(ESC_Buffer));
                goto EndEscape;
            }
        }

        case 'M':   // Moves cursor one line up, scrolling if needed
        {
            if (ESC_Type == ' ')
            {
                TTY_MoveCursor(TTY_CURSOR_UP, 1);
                kprintf("Hit a 'M' ($%X)", atoi(ESC_Buffer));
                goto EndEscape;
            }
        }
        */

        case 'c':
        {
            if (ESC_Type == ' ')    //RIS: Reset to initial state - Resets the device to its state after being powered on. 
            {
                TTY_Reset(TRUE);
                goto EndEscape;
            }
        }

        case 'h':   // Screen modes
        {
            switch (atoi(ESC_Buffer))
            {
                case 0: // 40 x 25 monochrome (text)
                case 130:   // 0 prefixed with =
                    //TTY_SetColumns(D_COLUMNS_64);
                    //PAL_setPalette(PAL2, pColorsMONO, CPU);
                break;
                case 1:     // 40 x 25 color (text)
                case 131:   // 1 prefixed with =
                    //TTY_SetColumns(D_COLUMNS_64);
                    //PAL_setPalette(PAL2, pColors, CPU);
                break;
                case 2:     // 80 x 25 monochrome (text)
                case 132:   // 2 prefixed with =
                    //TTY_SetColumns(D_COLUMNS_80);
                    //PAL_setPalette(PAL2, pColorsMONO, CPU);
                break;
                case 3:     // 80 x 25 color (text)
                case 133:   // 3 prefixed with =
                    //TTY_SetColumns(D_COLUMNS_80);
                    //PAL_setPalette(PAL2, pColors, CPU);
                break;
                case 7:     // Enables line wrapping
                case 137:   // 7 prefixed with =
                    bWrapAround = TRUE;
                break;

                case 0x19:
                case 0xF5: // Make cursor visible   ($F5 is actually ESC[?25h )
                    //PAL_setColor(2, 0x0e0);
                break;

                case 0x21:
                case 0xFD: // ??   ($FD is actually ESC[?33h )
                break;

                case 4: // 320 x 200 4-color (graphics)
                case 5: // 320 x 200 monochrome (graphics)
                case 6: // 640 x 200 monochrome (graphics)
                case 13: // 320 x 200 color (graphics)
                case 14: // 640 x 200 color (16-color graphics)
                case 15: // 640 x 350 monochrome (2-color graphics)
                case 16: // 640 x 350 color (16-color graphics)
                case 17: // 640 x 480 monochrome (2-color graphics)
                case 18: // 640 x 480 color (16-color graphics)
                case 19: // 320 x 200 color (256-color graphics)
                default:
                    //kprintf("Unimplemented screen mode '%u' ($%X)", atoi(ESC_Buffer), atoi(ESC_Buffer));
                break;
            }

            goto EndEscape;
        }

        case 'l':   // Reset screen mode
        {
            switch (atoi(ESC_Buffer))
            {
                case 7:     // Disables line wrapping
                case 137:   // 7 prefixed with =
                    bWrapAround = FALSE;    // Was TRUE, copy paste error?
                break;

                case 0x19:
                case 0xF5: // Make cursor invisible   ($F5 is actually ESC[?25l )
                    //PAL_setColor(2, 0x000); // This does not actually hide the cursor, on non black bg it will still be visible
                break;

                default:
                    //kprintf("Unimplemented reset screen mode '%u'", atoi(ESC_Buffer));
                break;
            }

            goto EndEscape;
        }

        case 'A':
        {
            TTY_SetSY_A(TTY_GetSY_A() - atoi(ESC_Buffer));
            goto EndEscape;
        }

        case 'B':
        {
            TTY_SetSY_A(TTY_GetSY_A() + atoi(ESC_Buffer));
            goto EndEscape;
        }

        case 'C':
        {
            TTY_SetSX(TTY_GetSX() + atoi(ESC_Buffer));
            goto EndEscape;
        }

        case 'D':
        {
            TTY_SetSX(TTY_GetSX() - atoi(ESC_Buffer));
            goto EndEscape;
        }

        case 'H':   // Move cursor to upper left corner if no parameters or to yy;xx
        case 'f':   // Some say its the same, other say its different...
        {
            if (ESC_Buffer[0] != '\0') ESC_Param[ESC_ParamSeq++] = atoi(ESC_Buffer);
            //for (u8 i = 0; i < ESC_ParamSeq; i++) kprintf("ESC_ParamSeq: %u = %u", (u8)i, ESC_Param[i]);

            
            if (((ESC_Param[0] == 0xFF) && (ESC_Param[1] == 0xFF)) || ((ESC_Param[0] == 0) && (ESC_Param[1] == 0)))
            {
                TTY_SetSX(0);
                TTY_SetSY_A(0);
            }
            else
            {
                TTY_SetSX(ESC_Param[1]-1);
                
                if (vDECOM)
                {
                    if ((ESC_Param[0]-1) < DMarginTop) ESC_Param[0] = DMarginTop+1;

                    if ((ESC_Param[0]-1) > DMarginBottom) ESC_Param[0] = DMarginBottom+1;
                }

                TTY_SetSY_A(ESC_Param[0]-1);
            }            

            TTY_MoveCursor(TTY_CURSOR_DUMMY);   // Dummy

            goto EndEscape;
        }

        case 'J':
        {
            u8 n = atoi(ESC_Buffer);

            switch (n)
            {
                case 1: // Clear screen from cursor up (Keep cursor position)
                    TTY_ClearLine(TTY_GetSY(), TTY_GetSY_A());
                break;

                case 2: // Clear screen (move cursor to top left only if emulating ANSI.SYS otherwise keep cursor position)
                    VDP_clearPlane(BG_A, TRUE);
                    VDP_clearPlane(BG_B, TRUE);
                break;

                case 3: // Clear screen and delete all lines saved in the scrollback buffer (Keep cursor position)
                    VDP_clearPlane(BG_A, TRUE);
                    VDP_clearPlane(BG_B, TRUE);
                break;
            
                case 0: // Clear screen from cursor down (Keep cursor position)
                default:
                    TTY_ClearLine(TTY_GetSY(), C_YMAX - TTY_GetSY_A());
                break;
            }            

            goto EndEscape;
        }

        case 'K':
        {
            u8 n = atoi(ESC_Buffer);

            switch (n)
            {
                case 1: // Erase start of line to the cursor (Keep cursor position)
                    TTY_ClearPartialLine(sy % 32, 0, TTY_GetSX());
                break;

                case 2: // Erase the entire line (Keep cursor position)
                    TTY_ClearLine(sy % 32, 1);
                break;
            
                case 0: // Erase from cursor to end of line (Keep cursor position)
                default:
                    TTY_ClearPartialLine(sy % 32, TTY_GetSX(), C_XMAX);
                break;
            }            

            goto EndEscape;
        }

        case 'm':
        {
            ESC_Param[ESC_ParamSeq++] = atoi(ESC_Buffer);

            if (ESC_Param[0] != 255) TTY_SetAttribute(ESC_Param[0]);
            if (ESC_Param[1] != 255) TTY_SetAttribute(ESC_Param[1]);
            if (ESC_Param[2] != 255) TTY_SetAttribute(ESC_Param[2]);
            if (ESC_Param[3] != 255) TTY_SetAttribute(ESC_Param[3]);

            goto EndEscape;
        }

        case 'n':   // Report cursor position
        {            
            u8 n = atoi(ESC_Buffer);
            char str[16] = {'\0'};

            switch (n)
            {
                case 6:
                    sprintf(str, "[%ld;%ldR", TTY_GetSY_A(), TTY_GetSX());
                    NET_SendString(str);
                break;

                default:
                break;
            }

            goto EndEscape;
        }

        case 'r':
        {
            ESC_Param[ESC_ParamSeq++] = atoi(ESC_Buffer);
            goto EndEscape;
        }

        case '_':
        {
            goto EndEscape;
        }

        case ' ':
        {
            return;
        }

        case '?':
        {
            QSeqNumber = 0;
            ESC_QSeq++;
            return;
        }

        default:
        break;
    }

    ESC_Buffer[ESC_BufferSeq++] = (char)*PRX;

    return;

    EndEscape:
    {
        bESCAPE = FALSE;
        return;
    }
}

static inline void IAC_SuggestNAWS()
{
    NET_SendChar(TC_IAC, TXF_NOBUFFER);
    NET_SendChar(TC_WILL, TXF_NOBUFFER);
    NET_SendChar(TO_NAWS, TXF_NOBUFFER);

    //IAC_NAWS_PENDING = TRUE;
}

static inline void IAC_SuggestEcho(u8 enable)
{
    NET_SendChar(TC_IAC, TXF_NOBUFFER);
    NET_SendChar((enable?TC_DO:TC_DONT), TXF_NOBUFFER);
    NET_SendChar(TO_ECHO, TXF_NOBUFFER);
}

static inline void DoIAC(u8 dummy)
{
    vu8 *PRX = &dummy;

    if (*PRX == TC_IAC) return; // Go away IAC...

    if ((IAC_InSubNegotiation) && (*PRX != TC_SE)) // What horror will emerge if an IAC is recieved here?
    {
        if (IAC_SNSeq > 15) return;

        IAC_SubNegotiationBytes[IAC_SNSeq++] = *PRX;
        #ifdef IAC_LOGGING
        kprintf("InSubNeg: Byte recieved: $%X (Seq: %u)", *PRX, IAC_SNSeq-1); 
        #endif
        return;
    }

    if (*PRX >= 240)
    {
        IAC_Command = *PRX;
        #ifdef IAC_LOGGING
        kprintf("Got IAC CMD: $%X", IAC_Command);
        #endif
    }

    if (*PRX <= 39)
    {
        IAC_Option = *PRX;
        #ifdef IAC_LOGGING
        kprintf("Got IAC Option: $%X", IAC_Option);
        #endif
    }

    if ((IAC_Command == 0) || (IAC_Option == 0xFF))
    {
        #ifdef IAC_LOGGING
        kprintf("IAC_Seq < 2 - Returning without resetting IAC");
        #endif
        return;
    }

    switch (IAC_Command)
    {
        case TC_SE:
        {
            IAC_InSubNegotiation = FALSE;
            #ifdef IAC_LOGGING
            kprintf("End of subneg.");
            #endif

            switch (IAC_SubNegotiationOption)
            {
                case TO_TERM_TYPE:
                {
                    if (IAC_SubNegotiationBytes[0] == TS_IS)
                    {
                        #ifdef IAC_LOGGING
                        kprintf("Got <IS TERM_TYPE> subneg. - Ignoring");
                        #endif
                    }
                    else if (IAC_SubNegotiationBytes[0] == TS_SEND)
                    {
                        #ifdef IAC_LOGGING
                        kprintf("Got <SEND TERM_TYPE> subneg.");
                        #endif
                        // Send "IAC SB TERMINAL-TYPE IS <some_terminal_type> IAC SE"
                        NET_SendChar(TC_IAC, TXF_NOBUFFER);
                        NET_SendChar(TC_SB, TXF_NOBUFFER);
                        NET_SendChar(TO_TERM_TYPE, TXF_NOBUFFER);
                        NET_SendChar(TS_IS, TXF_NOBUFFER);
                        NET_SendString(TermTypeList[vTermType]);
                        NET_SendChar(TC_IAC, TXF_NOBUFFER);
                        NET_SendChar(TC_SE, TXF_NOBUFFER);
                        #ifdef IAC_LOGGING
                        kprintf("Response: IAC SB TERM_TYPE IS %s IAC SE", TermTypeList[vTermType]);
                        #endif
                    }
                    break;
                }

                
                case TO_TERM_SPEED:
                {
                    if (IAC_SubNegotiationBytes[0] == TS_IS)
                    {
                        #ifdef IAC_LOGGING
                        kprintf("Got <IS TERM_SPEED> subneg. - Ignoring");
                        #endif
                    }
                    else if (IAC_SubNegotiationBytes[0] == TS_SEND)
                    {
                        #ifdef IAC_LOGGING
                        kprintf("Got <SEND TERM_SPEED> subneg.");
                        #endif

                        // IAC SB TERMINAL-SPEED IS ... IAC SE
                        NET_SendChar(TC_IAC, TXF_NOBUFFER);
                        NET_SendChar(TC_SB, TXF_NOBUFFER);
                        NET_SendChar(TO_TERM_SPEED, TXF_NOBUFFER);
                        NET_SendChar(TS_IS, TXF_NOBUFFER);
                        NET_SendString(vSpeed);
                        NET_SendChar(',', TXF_NOBUFFER);
                        NET_SendString(vSpeed);
                        NET_SendChar(TC_IAC, TXF_NOBUFFER);
                        NET_SendChar(TC_SE, TXF_NOBUFFER);

                        #ifdef IAC_LOGGING
                        kprintf("Response: IAC SB TERMINAL-SPEED IS %s,%s IAC SE", vSpeed, vSpeed);
                        #endif
                    }
                    break;
                }

                
                // https://datatracker.ietf.org/doc/html/rfc1116
                case TO_LINEMODE:
                {
                    switch (IAC_SubNegotiationBytes[0])
                    {
                        case LM_MODE:   // IAC SB LINEMODE MODE mask IAC SE
                        {
                            #ifdef IAC_LOGGING
                            kprintf("Got <LINEMODE MODE = %u> subneg.", IAC_SubNegotiationBytes[1]);
                            #endif

                            u8 NewLM = (IAC_SubNegotiationBytes[1] & (LMSM_EDIT | LMSM_TRAPSIG));

                            if (NewLM == vLineMode)
                            {
                                #ifdef IAC_LOGGING
                                kprintf("New linemode == current linemode; ignoring...");
                                #endif
                            }
                            else
                            {
                                vLineMode = NewLM;

                                NET_SendChar(TC_IAC, TXF_NOBUFFER);
                                NET_SendChar(TC_SB, TXF_NOBUFFER);
                                NET_SendChar(TO_LINEMODE, TXF_NOBUFFER);
                                NET_SendChar(LM_MODE, TXF_NOBUFFER);
                                NET_SendChar((vLineMode | LMSM_MODEACK), TXF_NOBUFFER);
                                NET_SendChar(TC_IAC, TXF_NOBUFFER);
                                NET_SendChar(TC_SE, TXF_NOBUFFER);

                                #ifdef IAC_LOGGING
                                kprintf("Response: IAC SB LINEMODE MODE %u IAC SE", (vLineMode | LMSM_MODEACK));
                                #endif
                            }
                            break;
                        }
                        
                        case LM_FORWARDMASK:
                        #ifdef IAC_LOGGING
                        kprintf("Got <LINEMODE FORWARDMASK = %u> subneg. NOT IMPLEMENTED", IAC_SubNegotiationBytes[1]);
                        #endif
                        break;
                        
                        case LM_SLC:
                        #ifdef IAC_LOGGING
                        kprintf("Got <LINEMODE SLC = %u> subneg. NOT IMPLEMENTED", IAC_SubNegotiationBytes[1]);
                        #endif
                        break;
                        
                        default:
                        #ifdef IAC_LOGGING
                        kprintf("Error: Unhandled Linemode. case (IAC_SubNegotiationBytes[0] = $%X)", IAC_SubNegotiationBytes[0]);
                        #endif
                        break;
                    }
                    break;
                }
            
                default:
                    #ifdef IAC_LOGGING
                    //kprintf("Got subneg. Error: Unhandled case (IAC_SubNegotiationOption = $%X)", IAC_SubNegotiationOption);
                    kprintf("Error: Unhandled subneg. case (IAC_SubNegotiationOption = $%X -- IAC_SubNegotiationBytes[0] = $%X)", IAC_SubNegotiationOption, IAC_SubNegotiationBytes[0]);
                    #endif
                break;
            }
            
            break;
        }

        case TC_SB:
        {
            IAC_InSubNegotiation = TRUE;
            IAC_SubNegotiationOption = IAC_Option;
            #ifdef IAC_LOGGING
            kprintf("Start subneg.");
            #endif
            return;
        }

        case TC_WILL:
        {
            switch (IAC_Option)
            {
                case TO_BIN_TRANS:
                {        
                    NET_SendChar(TC_IAC, TXF_NOBUFFER);
                    NET_SendChar(TC_DONT, TXF_NOBUFFER);
                    NET_SendChar(TO_BIN_TRANS, TXF_NOBUFFER);
                    
                    #ifdef IAC_LOGGING
                    kprintf("Server: IAC WILL TRANSMIT_BINARY - Response: IAC DONT TRANSMIT_BINARY - FULL IMPL. TODO");
                    #endif
                    break;
                }

                case TO_ECHO:
                    vDoEcho = 1;
                    #ifdef IAC_LOGGING
                    kprintf("Server: IAC WILL ECHO");
                    #endif
                break;

                case TO_SUPPRESS_GO_AHEAD:
                    vDoGA = 0;

                    NET_SendChar(TC_IAC, TXF_NOBUFFER);
                    NET_SendChar(TC_WILL, TXF_NOBUFFER);
                    NET_SendChar(TO_SUPPRESS_GO_AHEAD, TXF_NOBUFFER);

                    #ifdef IAC_LOGGING
                    kprintf("Server: IAC WILL SUPPRESS_GO_AHEAD - Client response: IAC WILL SUPPRESS_GO_AHEAD");
                    #endif
                break;
                
                default:
                    #ifdef IAC_LOGGING
                    kprintf("Error: Unhandled TC_WILL option case (IAC_Option = $%X)", IAC_Option);
                    #endif
                break;
            }

            break;
        }
        
        case TC_WONT:
        {
            switch (IAC_Option)
            {
                case TO_ECHO:
                    vDoEcho = 0;
                    #ifdef IAC_LOGGING
                    kprintf("Server: IAC WONT ECHO");
                    #endif
                break;
                
                default:
                    #ifdef IAC_LOGGING
                    kprintf("Error: Unhandled TC_WONT option case (IAC_Option = $%X)", IAC_Option);
                    #endif
                break;
            }

            break;
        }

        case TC_DO:
        {
            switch (IAC_Option)
            {
                case TO_BIN_TRANS:
                {        
                    NET_SendChar(TC_IAC, TXF_NOBUFFER);
                    NET_SendChar(TC_WONT, TXF_NOBUFFER);
                    NET_SendChar(TO_BIN_TRANS, TXF_NOBUFFER);
                    
                    #ifdef IAC_LOGGING
                    kprintf("Server: IAC DO TRANSMIT_BINARY - Response: IAC WONT TRANSMIT_BINARY - FULL IMPL. TODO");
                    #endif
                    break;
                }

                case TO_ECHO:
                {        
                    NET_SendChar(TC_IAC, TXF_NOBUFFER);
                    NET_SendChar(TC_WILL, TXF_NOBUFFER);
                    NET_SendChar(TO_ECHO, TXF_NOBUFFER);
                    vDoEcho = 1;
                    #ifdef IAC_LOGGING
                    kprintf("Response: IAC WILL ECHO");
                    #endif
                    break;
                }

                // Not sure if this is the proper response?
                case TO_SUPPRESS_GO_AHEAD:
                    vDoGA = 0;

                    NET_SendChar(TC_IAC, TXF_NOBUFFER);
                    NET_SendChar(TC_WILL, TXF_NOBUFFER);
                    NET_SendChar(TO_SUPPRESS_GO_AHEAD, TXF_NOBUFFER);

                    #ifdef IAC_LOGGING
                    kprintf("Response: IAC WILL SUPPRESS_GO_AHEAD");
                    #endif
                break;

                case TO_TERM_TYPE:
                {            
                    NET_SendChar(TC_IAC, TXF_NOBUFFER);
                    NET_SendChar(TC_WILL, TXF_NOBUFFER);
                    NET_SendChar(TO_TERM_TYPE, TXF_NOBUFFER);

                    #ifdef IAC_LOGGING
                    kprintf("Response: IAC WILL TERM_TYPE");
                    #endif
                    break;
                }
                
                case TO_NAWS:
                {
                    NET_SendChar(TC_IAC, TXF_NOBUFFER);
                    NET_SendChar(TC_WILL, TXF_NOBUFFER);
                    NET_SendChar(TO_NAWS, TXF_NOBUFFER);
                    
                    // IAC SB NAWS <16-bit value> <16-bit value> IAC SE
                    // Sent by the Telnet client to inform the Telnet server of the window width and height.
                    NET_SendChar(TC_IAC, TXF_NOBUFFER);
                    NET_SendChar(TC_SB, TXF_NOBUFFER);
                    NET_SendChar(0, TXF_NOBUFFER);
                    NET_SendChar((FontSize==0?0x28:0x50), TXF_NOBUFFER); // Columns - Use internal columns (D_COLUMNS_80/D_COLUMNS_40) here or use font size (4x8=80 & 8x8=40)? - Type? used to be FontSize==3
                    NET_SendChar(0, TXF_NOBUFFER);
                    NET_SendChar((bPALSystem?0x1D:0x1B), TXF_NOBUFFER); // Rows - 29=PAL - 27=NTSC
                    NET_SendChar(TC_IAC, TXF_NOBUFFER);
                    NET_SendChar(TC_SE, TXF_NOBUFFER);

                    #ifdef IAC_LOGGING
                    kprintf("Response: IAC WILL NAWS - IAC SB 0x%04X 0x%04X IAC SE", (FontSize==0?0x28:0x50), (bPALSystem?0x1D:0x1B));
                    #endif
                    break;
                }

                case TO_TERM_SPEED:
                {            
                    NET_SendChar(TC_IAC, TXF_NOBUFFER);
                    NET_SendChar(TC_WILL, TXF_NOBUFFER);
                    NET_SendChar(TO_TERM_SPEED, TXF_NOBUFFER);

                    #ifdef IAC_LOGGING
                    kprintf("Response: IAC WILL TERM_SPEED");
                    #endif
                    break;
                }

                // https://datatracker.ietf.org/doc/html/rfc779
                case TO_SEND_LOCATION:
                {            
                    /*NET_SendChar(TC_IAC, TXF_NOBUFFER);
                    NET_SendChar(TC_WILL, TXF_NOBUFFER);
                    NET_SendChar(TO_SEND_LOCATION, TXF_NOBUFFER);*/

                    // IAC SB SEND-LOCATION <location> IAC SE
                    NET_SendChar(TC_IAC, TXF_NOBUFFER);
                    NET_SendChar(TC_SB, TXF_NOBUFFER);
                    NET_SendChar(TO_SEND_LOCATION, TXF_NOBUFFER);
                    NET_SendString("MegaDriveLand");
                    NET_SendChar(TC_IAC, TXF_NOBUFFER);
                    NET_SendChar(TC_SE, TXF_NOBUFFER);

                    #ifdef IAC_LOGGING
                    kprintf("Response: IAC SB SEND-LOCATION <location> IAC SE");
                    #endif                    
                    break;
                }

                case TO_LINEMODE:
                {
                    NET_SendChar(TC_IAC, TXF_NOBUFFER);
                    NET_SendChar(TC_WILL, TXF_NOBUFFER);
                    NET_SendChar(TO_LINEMODE, TXF_NOBUFFER);
                    
                    #ifdef IAC_LOGGING
                    kprintf("Response: IAC WILL LINEMODE");
                    #endif

                    break;
                }

                // rfc1408
                case TO_ENV:
                {
                    // Just refuse for now
                    NET_SendChar(TC_IAC, TXF_NOBUFFER);
                    NET_SendChar(TC_WONT, TXF_NOBUFFER);
                    NET_SendChar(TO_ENV, TXF_NOBUFFER);
                    
                    #ifdef IAC_LOGGING
                    kprintf("Response: IAC WONT ENV");
                    #endif

                    break;
                }      

                // https://datatracker.ietf.org/doc/html/rfc1572
                case TO_ENV_OP:
                {
                    // Just refuse to send enviroment variables
                    NET_SendChar(TC_IAC, TXF_NOBUFFER);
                    NET_SendChar(TC_WONT, TXF_NOBUFFER);
                    NET_SendChar(TO_ENV_OP, TXF_NOBUFFER);
                    
                    #ifdef IAC_LOGGING
                    kprintf("Response: IAC WONT ENV_OP");
                    #endif

                    break;
                }                
                
                default:
                    #ifdef IAC_LOGGING
                    kprintf("Error: Unhandled TC_DO option case (IAC_Option = $%X)", IAC_Option);
                    #endif
                break;
            }

            break;
        }

        case TC_DONT:
        {
            switch (IAC_Option)
            {
                case TO_ECHO:
                    NET_SendChar(TC_IAC, TXF_NOBUFFER);
                    NET_SendChar(TC_WONT, TXF_NOBUFFER);
                    NET_SendChar(TO_ECHO, TXF_NOBUFFER);
                    vDoEcho = 0;
                    #ifdef IAC_LOGGING
                    kprintf("Response: IAC WONT ECHO");
                    #endif
                break;
                
                default:
                    #ifdef IAC_LOGGING
                    kprintf("Error: Unhandled TC_DONT option case (IAC_Option = $%X)", IAC_Option);
                    #endif
                break;
            }

            break;
        }
    
        default:
            #ifdef IAC_LOGGING
            kprintf("Error: Unhandled command case (IAC_Command = $%X -- IAC_Option = $%X)", IAC_Command, IAC_Option);
            #endif
        break;
    }

    #ifdef IAC_LOGGING
    kprintf("Ending IAC");
    #endif
    bIAC = FALSE;
    IAC_Command = 0;
    IAC_Option = 0xFF;
    IAC_SubNegotiationOption = 0xFF;

    return;
}
