#include "Telnet.h"
#include "Terminal.h"
#include "UTF8.h"
#include "Utils.h"
#include "Network.h"
#include "Cursor.h"
#include "DevMgr.h"

// https://vt100.net/docs/vt100-ug/chapter3.html
// https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
// http://www.braun-home.net/michael/info/misc/VT100_commands.htm

// https://www.iana.org/assignments/telnet-options/telnet-options.xhtml
// https://users.cs.cf.ac.uk/Dave.Marshall/Internet/node141.html
// https://www.omnisecu.com/tcpip/telnet-commands-and-options.php

// Telnet IAC commands
#define TC_IAC  255
#define TC_DONT 254
#define TC_DO   253
#define TC_WONT 252
#define TC_WILL 251 // $FB
#define TC_SB   250 // $FA Beginning of subnegotiation
#define TC_GA   249
#define TC_EL   248
#define TC_EC   247
#define TC_AYT  246
#define TC_AO   245
#define TC_IP   244
#define TC_BRK  243
#define TC_DM   242
#define TC_NOP  241
#define TC_SE   240 // $F0 End of subnegotiation parameters

// Telnet IAC options
#define TO_RECONNECTION
#define TO_BIN_TRANS                    0
#define TO_ECHO                         1
#define TO_SUPPRESS_GO_AHEAD            3
#define TO_APPROX_MSG_SZ_NEGOTIATION    4
#define TO_STATUS                       5
#define TO_TIMING_MARK                  6

#define TO_OUT_LINEWIDTH                8
#define TO_OUT_PAGESIZE                 9
#define TO_OUT_CR_DISPOS                10  // Carriage-Return disposition
#define TO_OUT_HTABSTOPS                11
#define TO_OUT_HTAB_DISPOS              12
#define TO_OUT_FORMFEED_DISPOS          13
#define TO_OUT_VTABSTOPS                14
#define TO_OUT_VTAB_DISPOS              15
#define TO_OUT_LINEFEED_DISPOS          16
#define TO_EXT_ASCII                    17
#define TO_LOGOUT                       18
#define TO_BYTE_MACRO                   19

#define TO_SEND_LOCATION                23  // $17
#define TO_TERM_TYPE                    24  // $18
#define TO_END_REC                      25
#define TO_USER_IDENT                   26
#define TO_NAWS                         31  // Negotiation About Window Size
#define TO_TERM_SPEED                   32
#define TO_RFLOW_CTRL                   33
#define TO_LINEMODE                     34
#define TO_XDISP                        35
#define TO_ENV                          36
#define TO_ENV_OP                       39

// Telnet Linemode mode commands
#define LM_MODE         1
#define LM_FORWARDMASK  2
#define LM_SLC          3

// Telnet subnegotiations commands
#define TS_IS   0
#define TS_SEND 1

static inline void DoEscape(u8 byte);
static inline void DoIAC(u8 byte);

// Telnet modifiable variables
static u8 vDoGA = 0;        // Use Go-Ahead
static u8 vDECOM = FALSE;   // DEC Origin Mode
static u8 vDECLRMM = 0;     // This control function defines whether or not the set left and right margins (DECSLRM) control function can set margins.

// DECSTBM
static s16 DMarginTop = 0;
static s16 DMarginBottom = 0;

// DECLRMM
static s16 DMarginLeft = 0;
static s16 DMarginRight = 0;

// Escapes [
static u8 bESCAPE = FALSE;         // If true: an escape code was recieved last byte
static u8 ESC_Seq = 0;
static u8 ESC_Type = 0;
static u8 ESC_Param[4] = {0xFF,0xFF,0xFF,0xFF};
static u8 ESC_ParamSeq = 0;
static char ESC_Buffer[4] = {'\0','\0','\0','\0'};
static u8 ESC_BufferSeq = 0;

static u8 ESC_QBuffer[6];
static u8 ESC_QSeq = 0;
static u16 QSeqNumber = 0; // atoi'd ESC_QBuffer

static char LastPrintedChar = ' ';

// This really should be cleaned up; Its only used to get/set the title text on SMDT status line
static char ESC_OSCBuffer[2] = {0xFF,'\0'};
static u8 ESC_OSCSeq = 0;
static u8 bTitle = FALSE;
static char ESC_TitleBuffer[32] = {'\0'};
static u8 ESC_TitleSeq = 0;

// IAC
static u8 bIAC = FALSE;                         // TRUE = Currently in a "Intercept As Command" stream
static u8 IAC_Command = 0;                      // Current TC_xxx command (0 = none set)
static u8 IAC_Option = 0xFF;                    // Current TO_xxx option  (FF = none set)
static u8 IAC_InSubNegotiation = 0;             // TRUE = Currently in a SB/SE block
static u8 IAC_SubNegotiationOption = 0xFF;      // Current TO_xxx option to operate in a subnegotiation 
static u8 IAC_SNSeq = 0;                        // Counter - where in "IAC_SubNegotiationBytes" we are
static u8 IAC_SubNegotiationBytes[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};   // Recieved byte stream in a subnegotiation block

static s32 Saved_sx = 0, Saved_sy = C_YSTART;   // Saved cursor position
static u8 CharMapSelection = 0;                 // Character map selection (0 = Default extended ASCII, 1 = DEC Line drawing set)

#ifdef EMU_BUILD
extern u32 StreamPos;   // Stream replay position
#endif

static const u8 CharMap[2][256] =
{
{   // Normal code page
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, // 00-0F
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, // 10-1F
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, // 20-2F
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, // 30-3F
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, // 40-4F
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, // 50-5F
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, // 60-6F
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, // 70-7F
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, // 80-8F
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, // 90-9F
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, // A0-AF
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, // B0-BF
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, // C0-CF
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, // D0-DF
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, // E0-EF
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF  // F0-FF
},
{   // DEC Special Character and Line Drawing Set
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, // 00-0F
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, // 10-1F
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, // 20-2F
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, // 30-3F
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, // 40-4F

    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x20, // 50-5F
    0x04, 0xB1, 0x09, 0x0C, 0x0D, 0x0A, 0xF8, 0xF1, 0x0A, 0x0B, 0xD9, 0xBF, 0xDA, 0xC0, 0xC5, 0xC4, // 60-6F
    0xC4, 0xC4, 0xC4, 0x5F, 0xC3, 0xB4, 0xC1, 0xC2, 0xB3, 0xF3, 0xF2, 0xE3, 0xF7, 0x9C, 0xF9, 0x7F, // 70-7F    // 7C = ≠ , using a different symbol in SMDTC ($F7)

    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, // 80-8F
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, // 90-9F
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, // A0-AF
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, // B0-BF
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, // C0-CF
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, // D0-DF
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, // E0-EF
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF  // F0-FF
}
};


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

    LastPrintedChar = ' ';
    CharMapSelection = 0;

    // Variable overrides
    vDoEcho = 0;
    vLineMode = 0;
    vNewlineConv = 0;
    sv_bWrapAround = TRUE;

    // ...
    ESC_OSCBuffer[0] = 0xFF;
    ESC_OSCBuffer[1] = '\0';
    ESC_OSCSeq = 0;
    bTitle = FALSE;
    ESC_TitleBuffer[0] = '\0';
    ESC_TitleSeq = 0;
}

inline void TELNET_ParseRX(u8 byte)
{
    //RXBytes++;

    if (bESCAPE)
    {
        DoEscape(byte);
        return;
    }
    else if (bUTF8)
    {
        DoUTF8(byte);
        return;
    }
    else if (bIAC)
    {
        DoIAC(byte);
        return;
    }

    switch (byte)
    {
        default:
            LastPrintedChar = CharMap[CharMapSelection][byte];
            TTY_PrintChar(LastPrintedChar);
            
            #ifdef TRM_LOGGING
            if ((byte < 0x20) /*|| (byte > 0x7E)*/) kprintf("TTY_ParseRX: Caught unhandled byte: $%X at position $%lX", byte, StreamPos);
            #endif
        break;
        case 0x1B:  // Escape 1
            bESCAPE = TRUE;
        break;
        case 0xE2:  // Dumb handling of UTF8
        case 0xEF:
            bUTF8 = TRUE;
        break;
        case TC_IAC:  // IAC
            bIAC = TRUE;
        break;
        case 0x0A:  // Line feed (new line)
            if (vNewlineConv == 1)  // Convert \n to \n\r
            {
                TTY_SetSX(0);
            }
            TTY_MoveCursor(TTY_CURSOR_DOWN, 1);
        break;
        case 0x0D:  // Carriage return
            TTY_SetSX(0);
            TTY_MoveCursor(TTY_CURSOR_DUMMY);   // Dummy
        break;
        case 0x08:  // Backspace
            TTY_MoveCursor(TTY_CURSOR_LEFT, 1);
        break;
        case 0x09:  // Horizontal tab
            TTY_MoveCursor(TTY_CURSOR_RIGHT, C_HTAB);
        break;
        case 0x0B:  // Vertical tab
            TTY_MoveCursor(TTY_CURSOR_DOWN, C_VTAB);
        break;
        case 0x0C:  // Form feed (new page)
            TTY_SetSX(0);
            TTY_SetSY(C_YSTART);

            VScroll = D_VSCROLL;

            TRM_FillPlane(BG_A, 0);
            TRM_FillPlane(BG_B, 0);

            *((vu32*) VDP_CTRL_PORT) = 0x40000010;
            *((vu16*) VDP_DATA_PORT) = VScroll;
            *((vu32*) VDP_CTRL_PORT) = 0x40020010;
            *((vu16*) VDP_DATA_PORT) = VScroll;

            TTY_MoveCursor(TTY_CURSOR_DUMMY);   // Dummy
        break;
        case 0x00:  // Null
        case 0x01:  // Start of heading
        case 0x02:  // Start of text
        case 0x03:  // End of text
        case 0x04:  // End of transmission
        case 0x05:  // Enquiry
        case 0x06:  // Acknowledge
        case 0x15:  // NAK (negative acknowledge)
            #ifdef TRM_LOGGING
            kprintf("Unimplemented VT100 mode single-character function: $%X", byte);
            #endif
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
    }
}

void ChangeTitle()
{
    char TitleBuf[40];

    sprintf(TitleBuf, "%s - %-21s", STATUS_TEXT, ESC_TitleBuffer);
    TRM_SetStatusText(TitleBuf);

    ESC_TitleSeq = 0;
    ESC_TitleBuffer[0] = '\0';
}

// https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
// https://en.wikipedia.org/wiki/ANSI_escape_code#CSIsection
// -------------------------------------------------------------------------------------------------
// Please ignore this hacky escape handling function...
// It was never meant to handle all the stuff it does and has ended up being quite messy
// -------------------------------------------------------------------------------------------------
static inline void DoEscape(u8 byte)
{
    ESC_Seq++;

    switch (ESC_Type)
    {
        case '[':
        {
            switch (byte)
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

                case 'c':
                {
                    if (ESC_Type == ' ')    // RIS: Reset to initial state - Resets the device to its state after being powered on. 
                    {
                        TTY_Reset(TRUE);
                        goto EndEscape;
                    }
                }

                case 'h':   // Screen modes
                {
                    switch (atoi(ESC_Buffer))
                    {
                        case 7:     // Enables line wrapping
                        case 137:   // 7 prefixed with =
                            sv_bWrapAround = TRUE;
                        break;

                        case 0x19:
                        case 0xF5: // Make cursor visible   ($F5 is actually ESC[?25h )
                            SetSprite_TILE(SPRITE_ID_CURSOR, LastCursor);
                        break;

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
                            sv_bWrapAround = FALSE;    // Was TRUE, copy paste error?
                        break;

                        case 0x19:
                        case 0xF5: // Make cursor invisible   ($F5 is actually ESC[?25l )
                            SetSprite_TILE(SPRITE_ID_CURSOR, 0x16);
                        break;

                        default:
                            //kprintf("Unimplemented reset screen mode '%u'", atoi(ESC_Buffer));
                        break;
                    }

                    goto EndEscape;
                }

                case 'A':
                {
                    u8 n = atoi(ESC_Buffer);
                    n = (n ? n : 1);
                    TTY_SetSY_A(TTY_GetSY_A() - n);
                    goto EndEscape;
                }

                case 'B':
                {
                    u8 n = atoi(ESC_Buffer);
                    n = (n ? n : 1);
                    TTY_SetSY_A(TTY_GetSY_A() + n);            
                    goto EndEscape;
                }

                case 'C':
                {
                    u8 n = atoi(ESC_Buffer);
                    n = (n ? n : 1);

                    if (vDECLRMM || vDECOM)
                    {
                        if (n > DMarginRight) n = DMarginRight;
                    }
                    else
                    {
                        n = (n > C_XMAX ? C_XMAX: n);
                    }
                    
                    TTY_SetSX(TTY_GetSX() + n);
                    goto EndEscape;
                }

                case 'D':
                {
                    u8 n = atoi(ESC_Buffer);
                    n = (n ? n : 1);

                    if (vDECLRMM || vDECOM)
                    {
                        if (n < DMarginLeft) n = DMarginLeft;
                    }
                    else
                    {
                        if ((TTY_GetSX() == 0) && (sv_bWrapAround))
                        {
                            TTY_MoveCursor(TTY_CURSOR_LEFT, 1);
                            goto EndEscape;
                        }
                        else if (TTY_GetSX() == 0)
                        {
                            n = (TTY_GetSX() == 0 ? 0: n);
                        }
                    }

                    TTY_SetSX(TTY_GetSX() - n);
                    goto EndEscape;
                }

                case 'b':   // Repeat last printed character n times
                {
                    u8 n = atoi(ESC_Buffer);

                    for (u8 i = 0; i < n; i++) TTY_PrintChar(LastPrintedChar);
                    
                    goto EndEscape;
                }

                case 's':   // Set Left and Right Margin (DECSLRM) when in DECLRMM mode, otherwise it is: Save Cursor [variant] (ansi.sys) - Same as Save Cursor (DECSC) (ESC 7)
                {
                    if (vDECLRMM)
                    {
                        ESC_Param[ESC_ParamSeq++] = atoi(ESC_Buffer);

                        if ((ESC_Param[1] == 0xFF) || (ESC_Param[1] == 0) || (ESC_Param[1] > C_XMAX))  // > or >= C_XMAX ?
                        {
                            DMarginRight = C_XMAX;
                        }
                        else DMarginRight = ESC_Param[1];

                        if (ESC_Param[0] >ESC_Param[0])
                        {
                            DMarginLeft = DMarginRight;
                        }
                        else DMarginLeft = ESC_Param[0];
                    }
                    else
                    {
                        Saved_sx = TTY_GetSX();
                        Saved_sy = TTY_GetSY();
                    }

                    goto EndEscape;
                }

                case 't':   // Window operations [DISPATCH] - https://terminalguide.namepad.de/seq/csi_st/
                {
                    u8 n = atoi(ESC_Buffer);
                    char str[16] = {'\0'};
                    //kprintf("Got ESC[%ut", n);

                    switch (n)
                    {
                        case 7:     // Refresh/Redraw Terminal Window ( Needs testing! CMD = "ESC [ 7 t" )
                        break;

                        case 8:     // Set Terminal Window Size ( Needs testing! CMD = "ESC [ 8 ; Ⓝ ; Ⓝ t"  Ⓝ = H/W in rows/columns )
                            //C_YMAX = H;
                            //C_XMAX = W;
                        break;

                        case 11:    // Report Terminal Window State (1 = non minimized - 2 = minimized)
                            NET_SendString("[1t");
                        break;

                        case 13:    // Report Terminal Window Position ( Needs testing! CMD = "ESC [ 13 ; Ⓝ t"  Ⓝ = 0/2 )
                            NET_SendString("[3;0;0t");
                        break;

                        case 14:    // Report Terminal Window Size in Pixels ( Needs testing! CMD = "ESC [ 14 ; Ⓝ t"  Ⓝ = 0/2 )
                            sprintf(str, "[4;%d;%dt", (bPALSystem?232:216), (sv_Font==FONT_8x8_16?320:640));
                            NET_SendString(str);
                        break;

                        case 15:    // Report Screen Size in Pixels ( Needs testing! CMD = "ESC [ 15 t" )
                            sprintf(str, "[5;%d;%dt", (bPALSystem?232:216), (sv_Font==FONT_8x8_16?320:640));
                            NET_SendString(str);
                        break;

                        case 16:    // Report Cell Size in Pixels ( Needs testing! CMD = "ESC [ 16 t" )
                            NET_SendString("[6;8;8t");
                        break;

                        case 18:    // Report Terminal Size ( Needs testing! CMD = "ESC [ 18 t" )
                            sprintf(str, "[8;%d;%dt", (bPALSystem?0x1D:0x1B), (sv_Font==FONT_8x8_16?0x28:0x50));
                            NET_SendString(str);
                        break;

                        case 19:    // Report Screen Size ( Needs testing! CMD = "ESC [ 19 t" )
                            sprintf(str, "[9;%d;%dt", (bPALSystem?0x1D:0x1B), (sv_Font==FONT_8x8_16?0x28:0x50));
                            NET_SendString(str);
                        break;

                        case 20:    // Get Icon Title ( Needs testing! CMD = "ESC [ 20 t" )
                            NET_SendString("]LNoIcon\\");
                        break;

                        case 21:    // Get Terminal Title ( Needs testing! CMD = "ESC [ 21 t" )
                            NET_SendString("]lNoTitle\\");
                        break;

                        case 22:    // Push Terminal Title ( Needs testing! CMD = "ESC [ 22 ; Ⓝ t" Ⓝ = 0/1/2)
                        break;

                        case 23:    // Pop Terminal Title ( Needs testing! CMD = "ESC [ 23 ; Ⓝ t" Ⓝ = 0/1/2)
                        break;

                        default:
                        break;
                    }
                    
                    goto EndEscape;
                }

                case 'u':   // Restore Cursor [variant] (ansi.sys) - Same as Restore Cursor (DECRC) (ESC 8)
                {
                    TTY_SetSX(Saved_sx);
                    TTY_SetSY(Saved_sy);
                    goto EndEscape;
                }

                case 'G':   // Alias: Cursor Horizontal Position Absolute
                {
                    u8 n = atoi(ESC_Buffer);

                    TTY_SetSX(n);

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
                        case 3: // Clear screen and delete all lines saved in the scrollback buffer (Keep cursor position)
                            TRM_FillPlane(BG_A, 0);
                            TRM_FillPlane(BG_B, 0);
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
                
                case 'X':   // Erase Character (ECH) -- ESC[ Ⓝ X
                {
                    u8 n = atoi(ESC_Buffer);

                    n = n == 0 ? 1 : n;

                    s32 oldsx = TTY_GetSX();
                    s32 oldsy = TTY_GetSY();
                    
                    TTY_MoveCursor(TTY_CURSOR_LEFT, 1);

                    for (u16 i = 0; i < n; i++)
                    {
                        TTY_PrintChar(' ');
                    }

                    TTY_SetSX(oldsx);
                    TTY_SetSY(oldsy);

                    TTY_MoveCursor(TTY_CURSOR_DUMMY);

                    goto EndEscape;
                }

                case 'd':   // Line Position Absolute [Row] (VPA)
                {
                    u8 n = atoi(ESC_Buffer);

                    TTY_SetSY_A(n-1);

                    goto EndEscape;
                }

                case 'e':   // Line Position Relative [rows] (VPR)
                {
                    u8 n = atoi(ESC_Buffer);

                    TTY_SetSY_A((TTY_GetSY_A() + n) -1);

                    goto EndEscape;
                }

                case 'm':
                {
                    ESC_Param[ESC_ParamSeq++] = atoi(ESC_Buffer);

                    if ((ESC_Param[0] == 38) && (ESC_Param[1] == 5))
                    {
                        if (ESC_Param[2] <= 7) TTY_SetAttribute(ESC_Param[2]+30);                                   //   0-  7:  standard colors (as in ESC [ 30–37 m)
                        else if ((ESC_Param[2] >= 8) && (ESC_Param[2] <= 15)) TTY_SetAttribute(ESC_Param[2]+90);    //   8- 15:  high intensity colors (as in ESC [ 90–97 m)
                                                                                                                    //  16-231:  6 × 6 × 6 cube (216 colors): 16 + 36 × r + 6 × g + b (0 ≤ r, g, b ≤ 5)
                                                                                                                    // 232-255:  grayscale from dark to light in 24 steps
                    }
                    else if ((ESC_Param[0] == 48) && (ESC_Param[1] == 5))
                    {
                        if (ESC_Param[2] <= 7) TTY_SetAttribute(ESC_Param[2]+40);                                   //   0-  7:  standard colors (as in ESC [ 40–47 m)
                        else if ((ESC_Param[2] >= 8) && (ESC_Param[2] <= 15)) TTY_SetAttribute(ESC_Param[2]+100);   //   8- 15:  high intensity colors (as in ESC [ 100–107 m)
                                                                                                                    //  16-231:  6 × 6 × 6 cube (216 colors): 16 + 36 × r + 6 × g + b (0 ≤ r, g, b ≤ 5)
                                                                                                                    // 232-255:  grayscale from dark to light in 24 steps
                    }
                    else
                    {
                        if (ESC_Param[0] != 255) TTY_SetAttribute(ESC_Param[0]);
                        if (ESC_Param[1] != 255) TTY_SetAttribute(ESC_Param[1]);
                        if (ESC_Param[2] != 255) TTY_SetAttribute(ESC_Param[2]);
                        if (ESC_Param[3] != 255) TTY_SetAttribute(ESC_Param[3]);
                    }

                    #ifdef ESC_LOGGING
                    //kprintf("TTY_SetAttribute: 0:<%u> 1:<%u> 2:<%u> 3:<%u>", ESC_Param[0], ESC_Param[1], ESC_Param[2], ESC_Param[3]);
                    #endif

                    goto EndEscape;
                }

                case 'n':   // Device Status Report [Dispatch] (DSR)
                {            
                    u8 n = atoi(ESC_Buffer);
                    char str[16] = {'\0'};

                    switch (n)
                    {
                        case 5: // Report Operating Status
                            NET_SendString("[0n");
                        break;

                        case 6: // Cursor Position Report (CPR)
                            sprintf(str, "[%ld;%ldR", TTY_GetSY_A(), TTY_GetSX());
                            NET_SendString(str);
                        break;

                        case 8: // Set Title to Terminal Name and Version.
                            // This could easily be set here, however current versions of SMDTC prefixes all titles with this information already
                        break;

                        default:
                        break;
                    }

                    goto EndEscape;
                }

                case 'p':   // Soft Reset (DECSTR)
                {            
                    u8 n = atoi(ESC_Buffer);

                    switch (n)
                    {
                        case '!': // Soft Reset.
                            TELNET_Init();
                            //TTY_Reset(TRUE);
                        break;

                        default:
                        break;
                    }

                    goto EndEscape;
                }

                case 'q':   // Select Cursor Style (DECSCUSR) - "ESC [ Ⓝ ␣ q" - (␣ = Space)
                {            
                    u8 n = atoi(ESC_Buffer);

                    switch (n)
                    {
                        case 0:
                        case 1: // Select Cursor Style Blinking Block
                        default:
                            bDoCursorBlink = TRUE;

                            if (sv_Font) LastCursor = 0x13;
                            else         LastCursor = 0x10;
                        break;
                        
                        case 2: // Select Cursor Style Steady Block
                            bDoCursorBlink = FALSE;
                            
                            if (sv_Font) LastCursor = 0x13;
                            else         LastCursor = 0x10;
                        break;
                        
                        case 3: // Select Cursor Style Blinking Underline
                            bDoCursorBlink = TRUE;

                            if (sv_Font) LastCursor = 0x14;
                            else         LastCursor = 0x11;
                        break;
                        
                        case 4: // Select Cursor Style Steady Underline
                            bDoCursorBlink = FALSE;
                            
                            if (sv_Font) LastCursor = 0x14;
                            else         LastCursor = 0x11;
                        break;
                        
                        case 5: // Select Cursor Style Blinking Bar
                            bDoCursorBlink = TRUE;

                            if (sv_Font) LastCursor = 0x15;
                            else         LastCursor = 0x12;
                        break;
                        
                        case 6: // Select Cursor Style Steady Bar
                            bDoCursorBlink = FALSE;
                            
                            if (sv_Font) LastCursor = 0x15;
                            else         LastCursor = 0x12;
                        break;
                    }

                    SetSprite_TILE(SPRITE_ID_CURSOR, LastCursor);

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

                default:
                    if ((byte >= 65) && (byte <= 122)) 
                    {
                        #ifdef EMU_BUILD
                        kprintf("Unhandled $%X  -  u8: %u  -  char: '%c'  -  EscType: %c (EscSeq: %u  -  StreamPos: $%lX)", byte, byte, (char)byte, (char)ESC_Type, ESC_BufferSeq, StreamPos);
                        #endif
                        goto EndEscape;
                    }
                break;
            }

            ESC_Buffer[ESC_BufferSeq++] = (char)byte;
            return;
        }
    
        case ']':
        {
            if (bTitle)
            {
                if (byte == 7)
                {
                    #ifdef ESC_LOGGING
                    kprintf("OSC: Change Window Title to %s", ESC_TitleBuffer);
                    #endif 

                    ChangeTitle();
                    bTitle = FALSE;
                    goto EndEscape;
                }

                if (ESC_TitleSeq < 32)
                {
                    ESC_TitleBuffer[ESC_TitleSeq++] = byte;
                }

                return;
            }

            switch (atoi(ESC_OSCBuffer))
            {
                case 2:
                    #ifdef ESC_LOGGING
                    kprintf("OSC: Change Window Title");
                    #endif

                    bTitle = TRUE;
                break;
            
                default:
                break;
            }

            switch (byte)
            {
                case ';':                    
                    #ifdef ESC_LOGGING
                    kprintf("OSC: $%X", atoi(ESC_OSCBuffer));
                    #endif

                    ESC_OSCBuffer[0] = 0xFF;
                    ESC_OSCBuffer[1] = '\0';
                    ESC_OSCSeq = 0;
                break;
            
                default:
                    if (ESC_OSCSeq < 2)
                    {
                        ESC_OSCBuffer[ESC_OSCSeq++] = byte;
                    }
                break;
            }

            return;
        }

        case '(':
        {
            switch (byte)
            {
                case '0':   // DEC Special Character and Line Drawing Set
                {
                    CharMapSelection = 1;
                    #ifdef ESC_LOGGING
                    kprintf("ESC(0: DEC Special Character and Line Drawing Set");
                    #endif
                    break;
                }
                
                case 'B':
                {
                    CharMapSelection = 0;
                    #ifdef ESC_LOGGING
                    kprintf("ESC(B: United States (USASCII), VT100");
                    #endif
                    break;
                }

                default:
                {
                    #ifdef ESC_LOGGING
                    kprintf("Unknown ESC( sequence - $%X ( %c )", byte, (char)byte);
                    #endif
                }
            }

            goto EndEscape;
        }

        
        case '?':
        {
            ESC_QBuffer[ESC_QSeq-1] = byte;
            //kprintf("ESC_Q: $%X - '%c'", ESC_QBuffer[ESC_QSeq-1], (char)ESC_QBuffer[ESC_QSeq-1]);

            if (byte == 'h')
            {
                QSeqNumber = atoi16((char*)ESC_QBuffer);
                //kprintf("QSeqNumber = %u", QSeqNumber);

                switch (QSeqNumber)
                {
                    case 6:   // Origin Mode (DECOM), VT100.
                        vDECOM = TRUE;
                    break;
                
                    case 7:   // Auto-Wrap Mode (DECAWM), VT100.
                        sv_bWrapAround = TRUE;
                    break;
                
                    case 25:   // Shows the cursor, from the VT220. (DECTCEM)
                        SetSprite_TILE(SPRITE_ID_CURSOR, LastCursor);
                    break;

                    case 69:   // DECSLRM can set margins.
                        vDECLRMM = TRUE;
                    break;

                    //case 2004:   // Turn on bracketed paste mode. In bracketed paste mode, text pasted into the terminal will be surrounded by ESC [200~ and ESC [201~; programs running in the terminal should not treat characters bracketed by those sequences as commands (Vim, for example, does not treat them as commands). From xterm
                    //break;

                    // 1000h = Send Mouse X & Y on button press and release.  See the section Mouse Tracking.  This is the X11 xterm mouse protocol.
                    // 1006h = Enable SGR Mouse Mode, xterm.

                    // Missing QEsq:
                    // ?2004h
                    // ?1049h
                    // ?1h
                    // ?25h

                    default:
                    //kprintf("Got an unknown ?%uh", QSeqNumber);//?%ch", (char)ESC_QBuffer[0]);
                    break;
                }

                goto EndEscape;
            }

            if (byte == 'l')
            {
                QSeqNumber = atoi16((char*)ESC_QBuffer);
                //kprintf("QSeqNumber = %u", QSeqNumber);

                switch (QSeqNumber)
                {
                    case 6:   // Normal Cursor Mode (DECOM), VT100.
                        vDECOM = FALSE;
                    break;
                
                    case 7:   // No Auto-Wrap Mode (DECAWM), VT100.
                        sv_bWrapAround = FALSE;
                    break;
                
                    case 25:   // Hides the cursor. (DECTCEM)
                        SetSprite_TILE(SPRITE_ID_CURSOR, 0x2016);
                    break;

                    case 69:   // DECSLRM cannot set margins.
                        vDECLRMM = FALSE;
                    break;

                    //case 2004:   // Turn off bracketed paste mode. 
                    //break;


                    default:
                    //kprintf("Got an unknown ?%cl", (char)ESC_QBuffer[0]);
                    //kprintf("Got an unknown ?%ul", QSeqNumber);//?%ch", (char)ESC_QBuffer[0]);
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

        default:
            if (ESC_Seq == 1)
            {
                ESC_Type = byte;
                
                #ifdef ESC_LOGGING
                kprintf("ESC_Type: $%X ( %c )", ESC_Type, (char)ESC_Type);
                #endif

                switch (ESC_Type)
                {        
                    case '=':   // ESC =     Application Keypad (DECKPAM).
                        goto EndEscape;
                    break;

                    case '>':   // ESC >     Normal Keypad (DECKPNM), VT100.
                        goto EndEscape;
                    break;

                    case 'M':   // ESC M     Reverse Index (RI) https://terminalguide.namepad.de/seq/a_esc_cm/  (Old note: Moves cursor one line up, scrolling if needed)
                        // Not quite right, but eh
                        TTY_MoveCursor(TTY_CURSOR_UP, 1);
                        goto EndEscape;
                    break;
                    
                    case '7':   // Save Cursor (DECSC) (ESC 7)
                    {
                        Saved_sx = TTY_GetSX();
                        Saved_sy = TTY_GetSY();
                        goto EndEscape;
                    }

                    case '8':   // Restore Cursor (DECRC) (ESC 8)
                    {
                        TTY_SetSX(Saved_sx);
                        TTY_SetSY(Saved_sy);
                        goto EndEscape;
                    }
                
                    default:
                    break;
                }

                return;
            }
        break;
    }

    return;

    EndEscape:
    {
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
        /*ESC_Buffer[2] = '\0';
        ESC_Buffer[3] = '\0';*/
        ESC_BufferSeq = 0;
        ESC_QSeq = 0;
        ESC_OSCSeq = 0;
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

static inline void DoIAC(u8 byte)
{
    if (byte == TC_IAC) return; // Go away IAC...

    if ((IAC_InSubNegotiation) && (byte != TC_SE)) // What horror will emerge if an IAC is recieved here?
    {
        if (IAC_SNSeq > 15) return;

        IAC_SubNegotiationBytes[IAC_SNSeq++] = byte;
        #ifdef IAC_LOGGING
        kprintf("InSubNeg: Byte recieved: $%X (Seq: %u)", byte, IAC_SNSeq-1); 
        #endif
        return;
    }

    if (byte >= 240)
    {
        IAC_Command = byte;
        #ifdef IAC_LOGGING
        kprintf("Got IAC CMD: $%X", IAC_Command);
        #endif
    }

    if (byte <= 39)
    {
        IAC_Option = byte;
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
                        NET_SendString(TermTypeList[sv_TermType]);
                        NET_SendChar(TC_IAC, TXF_NOBUFFER);
                        NET_SendChar(TC_SE, TXF_NOBUFFER);
                        #ifdef IAC_LOGGING
                        kprintf("Response: IAC SB TERM_TYPE IS %s IAC SE", TermTypeList[sv_TermType]);
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

                        if (bRLNetwork)
                        {
                            // IAC SB TERMINAL-SPEED IS ... IAC SE
                            NET_SendChar(TC_IAC, TXF_NOBUFFER);
                            NET_SendChar(TC_SB, TXF_NOBUFFER);
                            NET_SendChar(TO_TERM_SPEED, TXF_NOBUFFER);
                            NET_SendChar(TS_IS, TXF_NOBUFFER);
                            NET_SendString("921600");
                            NET_SendChar(',', TXF_NOBUFFER);
                            NET_SendString("921600");
                            NET_SendChar(TC_IAC, TXF_NOBUFFER);
                            NET_SendChar(TC_SE, TXF_NOBUFFER);

                            #ifdef IAC_LOGGING
                            kprintf("Response: IAC SB TERMINAL-SPEED IS 921600,921600 IAC SE");
                            #endif
                        }
                        else
                        {
                            // IAC SB TERMINAL-SPEED IS ... IAC SE
                            NET_SendChar(TC_IAC, TXF_NOBUFFER);
                            NET_SendChar(TC_SB, TXF_NOBUFFER);
                            NET_SendChar(TO_TERM_SPEED, TXF_NOBUFFER);
                            NET_SendChar(TS_IS, TXF_NOBUFFER);
                            NET_SendString(sv_Baud);
                            NET_SendChar(',', TXF_NOBUFFER);
                            NET_SendString(sv_Baud);
                            NET_SendChar(TC_IAC, TXF_NOBUFFER);
                            NET_SendChar(TC_SE, TXF_NOBUFFER);

                            #ifdef IAC_LOGGING
                            kprintf("Response: IAC SB TERMINAL-SPEED IS %s,%s IAC SE", sv_Baud, sv_Baud);
                            #endif
                        }
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

                // https://datatracker.ietf.org/doc/html/rfc859
                case TO_STATUS:
                    NET_SendChar(TC_IAC, TXF_NOBUFFER);
                    NET_SendChar(TC_DONT, TXF_NOBUFFER);
                    NET_SendChar(TO_STATUS, TXF_NOBUFFER);

                    #ifdef IAC_LOGGING
                    kprintf("Server: IAC WILL STATUS - Client response: IAC DONT STATUS");
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
                    NET_SendChar((sv_Font==FONT_8x8_16?0x28:0x50), TXF_NOBUFFER); // Columns - Use internal columns (D_COLUMNS_80/D_COLUMNS_40) here or use font size (4x8=80 & 8x8=40)? - Type? used to be sv_Font==3
                    NET_SendChar(0, TXF_NOBUFFER);
                    NET_SendChar((bPALSystem?0x1D:0x1B), TXF_NOBUFFER); // Rows - 29=PAL - 27=NTSC
                    NET_SendChar(TC_IAC, TXF_NOBUFFER);
                    NET_SendChar(TC_SE, TXF_NOBUFFER);

                    #ifdef IAC_LOGGING
                    kprintf("Response: IAC WILL NAWS - IAC SB 0x%04X 0x%04X IAC SE", (sv_Font==FONT_8x8_16?0x28:0x50), (bPALSystem?0x1D:0x1B));
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

                // https://datatracker.ietf.org/doc/html/rfc1080
                case TO_RFLOW_CTRL:
                {            
                    NET_SendChar(TC_IAC, TXF_NOBUFFER);
                    NET_SendChar(TC_WONT, TXF_NOBUFFER);
                    NET_SendChar(TO_RFLOW_CTRL, TXF_NOBUFFER);

                    #ifdef IAC_LOGGING
                    kprintf("Response: IAC WONT RFLOW_CTRL");
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

                case TO_XDISP:
                {
                    NET_SendChar(TC_IAC, TXF_NOBUFFER);
                    NET_SendChar(TC_WONT, TXF_NOBUFFER);
                    NET_SendChar(TO_XDISP, TXF_NOBUFFER);
                    
                    #ifdef IAC_LOGGING
                    kprintf("Response: IAC WONT XDISP");
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
