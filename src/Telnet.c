#include "Telnet.h"
#include "UTF8.h"
#include "Utils.h"
#include "Network.h"
#include "Cursor.h"
#include "DevMgr.h"
#include "WinMgr.h"
#include "Mouse.h"
#include "system/PseudoFile.h"
#include "system/Time.h"

// https://vt100.net/docs/vt100-ug/chapter3.html
// https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
// http://www.braun-home.net/michael/info/misc/VT100_commands.htm

// https://www.iana.org/assignments/telnet-options/telnet-options.xhtml
// https://users.cs.cf.ac.uk/Dave.Marshall/Internet/node141.html
// https://www.omnisecu.com/tcpip/telnet-commands-and-options.php

// Telnet IAC commands
#define TC_IAC  255 // $FF
#define TC_DONT 254 // $FE
#define TC_DO   253 // $FD
#define TC_WONT 252 // $FC
#define TC_WILL 251 // $FB
#define TC_SB   250 // $FA Beginning of subnegotiation
#define TC_GA   249 // $F9 Go-Ahead
#define TC_EL   248 // $F8
#define TC_EC   247 // $F7
#define TC_AYT  246 // $F6
#define TC_AO   245 // $F5
#define TC_IP   244 // $F4
#define TC_BRK  243 // $F3
#define TC_DM   242 // $F2 Data Mark
#define TC_NOP  241 // $F1
#define TC_SE   240 // $F0 End of subnegotiation parameters
#define TC_EOR  239 // $EF

// Telnet IAC options
#define TO_RECONNECTION
#define TO_BIN_TRANS                    0
#define TO_ECHO                         1
#define TO_SUPPRESS_GO_AHEAD            3
#define TO_APPROX_MSG_SZ_NEGOTIATION    4
#define TO_STATUS                       5
#define TO_TIMING_MARK                  6
//...
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
//...
#define TO_SEND_LOCATION                23  // $17
#define TO_TERM_TYPE                    24  // $18
#define TO_END_REC                      25
#define TO_USER_IDENT                   26
#define TO_NAWS                         31  // Negotiation About Window Size
#define TO_TERM_SPEED                   32
#define TO_RFLOW_CTRL                   33
#define TO_LINEMODE                     34
#define TO_XDISP                        35
#define TO_ENV                          36  // ENVIRON
#define TO_ENV_OP                       39  // NEW-ENVIRON

// Telnet Linemode mode commands
#define LM_MODE         1
#define LM_FORWARDMASK  2
#define LM_SLC          3

// Telnet subnegotiations commands
#define TS_IS   0
#define TS_SEND 1
#define TS_INFO 2

// Telnet environment codes
#define TENV_VAR     0 
#define TENV_VALUE   1 
#define TENV_ESC     2
#define TENV_USERVAR 3

// Misc
#define MAX_LABEL_STACK_SIZE 8  // 4 Window + 4 Icon
#define MAX_LABEL_SSIZE ((MAX_LABEL_STACK_SIZE/2)-1)
#define ICON_LABEL_OFFSET (MAX_LABEL_STACK_SIZE/2)

// Forward decl.
static void DoEscape(u8 byte);
static void DoIAC(u8 byte);

// Next "expected" type of byte to be received
NextCommand NextByte = NC_Data;

// Telnet modifiable variables
static u8 vDoGA = 0;            // Use Go-Ahead
static u8 vDECLRMM = 0;         // This control function defines whether or not the set left and right margins (DECSLRM) control function can set margins.
u8 vDECCKM = 0;                 // Cursor Key Format (DECCKM) - 0 = OFF - 1 = ON (TLDR: Numlock)
u8 sv_AllowRemoteEnv = FALSE;   // Allow remote server to access/modify local enviroment variables
u8 vBracketedPaste = FALSE;     // In bracketed paste mode (Unused/unimplemented)
u8 sv_EnableUTF8 = TRUE;
static u8 vMinimized = FALSE;

// DECSTBM
s16 DMarginTop = 0;      // Region top margin
s16 DMarginBottom = 0;   // Region bottom margin

// DECLRMM
s16 DMarginLeft = 0;     // Region left margin
s16 DMarginRight = 0;    // Region right margin

// DECOM
static u8 vDECOM = FALSE;       // DEC Origin Mode enabled
static s16 Saved_OrgTop[2] = {0, 0}, Saved_OrgBottom[2] = {29, 29}; // Saved cursor origin
static s16 Saved_sx[2] = {0, 0}, Saved_sy[2] = {C_YSTART, C_YSTART};        // Saved cursor position

// XTCHECKSUM
static u8 XTCHECKSUM = 1;

// Escapes [
static u8 ESC_Seq = 0;
static u8 ESC_Type = 0;
static u8 ESC_Param[10] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static u16 ESC_Param16 = 0xFFFF;
static u8 ESC_ParamSeq = 0;
static char ESC_Buffer[4] = {'\0','\0','\0','\0'};
static u8 ESC_BufferSeq = 0;

// Mode parser
static u8 ESC_QBuffer[6];
static u8 ESC_QSeq = 0;
static u8 ESC_QSeqMulti = 0;
static u16 QSeqNumber = 0; // atoi'd ESC_QBuffer

// Operating System Control (OSC)
static char ESC_OSCBuffer[2] = {0xFF,'\0'};
static u8 ESC_OSCSeq = 0;
static u16 OSC_Type = 0;
static char OSC_String[40];
static bool bOSC_GetString = FALSE;
static bool bOSC_Parse = FALSE;
static bool bOSC_GetType = TRUE;

// IAC
static u8 IAC_Command = 0;                      // Current TC_xxx command (0 = none set)
static u8 IAC_Option = 0xFF;                    // Current TO_xxx option  (FF = none set)
static u8 IAC_InSubNegotiation = 0;             // TRUE = Currently in a SB/SE block
static u8 IAC_SubNegotiationOption = 0xFF;      // Current TO_xxx option to operate in a subnegotiation 
static u8 IAC_SNSeq = 0;                        // Counter - where in "IAC_SubNegotiationBytes" we are
static u8 IAC_SubNegotiationBytes[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};   // Recieved byte stream in a subnegotiation block

// Mouse tracking
static u32 PreviousFrame = 0;   // Frame tracker to make sure we don't sent mouse events to often
static s16 MLastX, MLastY;      // Last reported X/Y mouse position
typedef enum
{
    MT_ClickOnly = 0,
    MT_DownUp    = 1,
    MT_HighLight = 2,
    MT_ClickDrag = 3,
    MT_Movement  = 4
} MT_Mode;  // Mouse tracking modes - mutually exclusive
static MT_Mode MTrackMode = MT_ClickOnly;
typedef enum
{
    MR_Default   = 0,
    MR_Multibyte = 1,
    MR_Digits    = 2,
    MR_URXVT     = 3,
} MR_Mode;  // Mouse reporting format
static MR_Mode MReportFormat = MR_Default;

// Misc
static u8 CharMapSelection = 0;         // Character map selection (0 = Default extended ASCII, 1 = DEC Line drawing set)
static char LastPrintedChar = ' ';      // Last character that was printed to screen
u8 HTS_Column[80];                      // Horizontal tab stop positions
static s16 TermPosX = 0, TermPosY = 0;  // Dummy terminal position

static char FakeWindowLabel[40], FakeIconLabel[40]; // Faked window and icon title string. This can only be set by the remote server, thus it can only contain strings which the server already knows about.
static char **LabelStack;                           // Pushed/Popped window and icon label strings. 0-3 = Window, 4-7 = Icon
static u8 WindowNum, IconNum;                       // Number of window/icon labels on stack

static const u8 CharMap1[256] =
{   // DEC Special Character and Line Drawing Set
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, // 00-0F    C0
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, // 10-1F
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, // 20-2F    GL
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, // 30-3F
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, // 40-4F

    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x20, // 50-5F
    0x04, 0xB1, 0x09, 0x0C, 0x0D, 0x0A, 0xF8, 0xF1, 0x0A, 0x0B, 0xD9, 0xBF, 0xDA, 0xC0, 0xC5, 0xC4, // 60-6F
    0xC4, 0xC4, 0xC4, 0x5F, 0xC3, 0xB4, 0xC1, 0xC2, 0xB3, 0xF3, 0xF2, 0xE3, 0xF7, 0x9C, 0xF9, 0x7F, // 70-7F    // 7C = ≠ , using a different symbol in SMDT ($F7)

    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, // 80-8F    C1
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, // 90-9F
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, // A0-AF    GR
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, // B0-BF
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, // C0-CF
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, // D0-DF
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, // E0-EF
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF  // F0-FF
};
/*static const u8 CharMap2[256] =
{   // ...This is just the default 8 bit ASCII map for now...
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, // 00-0F    C0
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, // 10-1F
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, // 20-2F    GL
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, // 30-3F
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, // 40-4F
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, // 50-5F
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, // 60-6F
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, // 70-7F
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, // 80-8F    C1
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, // 90-9F
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, // A0-AF    GR
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, // B0-BF
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, // C0-CF
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, // D0-DF
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, // E0-EF
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF  // F0-FF
};*/


void TELNET_Init(TTY_InitFlags tty_flags)
{
    TTY_Init(tty_flags);
    UTF8_Init();

    NextByte = NC_Data;

    ESC_Seq = 0;
    ESC_Type = 0;
    ESC_Param[0] = 0xFF;
    ESC_Param[1] = 0xFF;
    ESC_Param[2] = 0xFF;
    ESC_Param[3] = 0xFF;
    ESC_Param[4] = 0xFF;
    ESC_Param[5] = 0xFF;
    ESC_Param[6] = 0xFF;
    ESC_Param[7] = 0xFF;
    ESC_Param[8] = 0xFF;
    ESC_Param[9] = 0xFF;
    ESC_Param16 = 0xFFFF;
    ESC_ParamSeq = 0;
    ESC_Buffer[0] = '\0';
    ESC_Buffer[1] = '\0';
    ESC_Buffer[2] = '\0';
    ESC_Buffer[3] = '\0';
    ESC_BufferSeq = 0;

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
    vDECLRMM = 0;
    vDECCKM = 0;
    vMinimized = FALSE;

    DMarginTop = 0;
    DMarginBottom = C_YMAX;
    DMarginLeft = 0;
    DMarginRight = C_XMAX;

    Saved_OrgTop[0] = DMarginTop;
    Saved_OrgTop[1] = DMarginTop;

    Saved_OrgBottom[0] = DMarginBottom;
    Saved_OrgBottom[1] = DMarginBottom;

    LastPrintedChar = ' ';
    CharMapSelection = 0;

    memset(FakeWindowLabel, 0, 40);
    memset(FakeIconLabel, 0, 40);
    strcpy(FakeWindowLabel, "SMDT Terminal Emulator");
    strcpy(FakeIconLabel, "SMDT");

    if (LabelStack == NULL)
    {
        LabelStack = malloc(MAX_LABEL_STACK_SIZE * sizeof(char*));
        if (LabelStack != NULL)
        {
            for (u16 i = 0; i < MAX_LABEL_STACK_SIZE; i++)
            {
                LabelStack[i] = (char*)malloc(40);
                
                if (LabelStack[i] == NULL)
                {
                    #ifdef TRM_LOGGING
                    kprintf("[91mError: Failed to allocate label #%u[0m", i);
                    #endif
                }

                memset(LabelStack[i], 0, 40);
            }
        }
        else
        {
            #ifdef TRM_LOGGING
            kprintf("[91mError: Failed to allocate label stack![0m");
            #endif
        }
    }
    else
    {
        for (u16 i = 0; i < MAX_LABEL_STACK_SIZE; i++)
        {
            memset(LabelStack[i], 0, 40);
        }
    }

    // Horizontal tab stops
    memset(HTS_Column, 0, 80);
    for (u8 c = 0; c < 80; c += C_HTAB)
    {
        HTS_Column[c] = 1;
    }

    Saved_sx[0] = 0;
    Saved_sx[1] = 0;
    Saved_sy[0] = C_YSTART;
    Saved_sy[1] = C_YSTART;

    // Variable overrides
    vDoEcho = 0;
    vLineMode = 0;
    vBackspace = 0;
    vNewlineConv = 0;
    sv_bWrapAround = TRUE;
    bDoCursorBlink = TRUE;

    // OSC
    ESC_OSCBuffer[0] = '\0';
    ESC_OSCBuffer[1] = '\0';
    ESC_OSCSeq = 0;
    OSC_String[0] = '\0';
    OSC_Type = 0;
    bOSC_GetString = FALSE;
    bOSC_Parse = FALSE;
    bOSC_GetType = TRUE;

    // Mouse tracking
    MTrackMode = MT_ClickOnly;
    MReportFormat = MR_Default;
}

void Telnet_Quit()
{
    for (u16 i = 0; i < MAX_LABEL_STACK_SIZE; i++)
    {
        free(LabelStack[i]);
        LabelStack[i] = NULL;
    }

    free(LabelStack);
    LabelStack = NULL;
}

void Telnet_MouseTrack()
{
    s16 MCurX = Mouse_GetX();
    s16 MCurY = Mouse_GetY();

    if (FrameElapsed(&PreviousFrame, 1) && bMouse && ((MCurX != MLastX) || (MCurY != MLastY)))
    {
        char str[16];
        u16 len = 0;
        u8 btn = 0;
        u8 column = 0;
        u8 row = 0;

        switch (MTrackMode)
        {
            case MT_ClickOnly:
                
            break;

            case MT_DownUp:
            break;

            case MT_HighLight:
            break;

            case MT_ClickDrag:
            break;

            case MT_Movement:
                column = (MCurX / 8) + 1;
                row = (MCurY / 8) + 1;
            break;
        
            default:
            break;
        }
        
        switch (MReportFormat)
        {
            case MR_Default:        
                len = sprintf(str, "[M%u%u%u", btn, column, row);
                NET_SendStringLen(str, len);
            break;
            
            case MR_Multibyte:
            break;

            case MR_Digits:
            break;

            case MR_URXVT:
            break;
        
            default:
            break;
        }
    }

    MLastX = MCurX;
    MLastY = MCurY;
}

inline u8 Find_NextTabStop()
{
    u8 cx = (TTY_GetSX() % C_XMAX); // Current cursor column

    // Search forward, skipping the current column
    for (u8 i = cx + 1; i < C_XMAX; i++)  
    {
        if (HTS_Column[i] == 1)
        {
            return i; // Found next tab stop
        }
    }

    return 79; // Default to last column if no tab stop is found
}

inline u8 Find_LastTabStop()
{
    u8 cx = (TTY_GetSX() % C_XMAX); // Current cursor column

    // Search backward, skipping the current column
    for (u8 i = (cx > 1 ? cx - 1 : 0); i > 0; i--)  
    {
        if (HTS_Column[i] == 1)
        {
            return i; // Found previous tab stop
        }
    }

    return 0; // Default to first column if no tab stop is found
}

void TELNET_ParseRX(u8 byte)
{
    RXBytes++;

    switch (NextByte)
    {
        default: 
            goto Data; 
        break;

        case NC_SkipUTF:
            goto SkipUTF;
        break;

        case NC_UTF8:
            DoUTF8(byte);
        break;

        case NC_Escape:
            DoEscape(byte);
        break;

        case NC_IAC:
            DoIAC(byte);
        break;    
    }

    return;

    Data:

    //if (sv_EnableUTF8 == FALSE) goto SkipUTF;

    if (sv_EnableUTF8)
    {
        switch (byte & 0xF0)
        {
            case 0xC0:
                if ((byte & 0x20) != 0) break;

                UTF_Bytes = 2;  // 2 bytes - 110xxxxx yyyyyyyy
                DoUTF8(byte);

                NextByte = NC_UTF8;
            return;

            // 0xD0 - 1101xxxx - ?

            case 0xE0:
                if ((byte & 0x10) != 0) break;

                UTF_Bytes = 3;  // 3 bytes - 1110xxxx yyyyyyyy zzzzzzzz
                DoUTF8(byte);

                NextByte = NC_UTF8;
            return;

            case 0xF0:
                if ((byte & 0x8) != 0) break;

                UTF_Bytes = 4;  // 4 bytes - 11110xxx yyyyyyyy zzzzzzzz wwwwwwwww
                DoUTF8(byte);

                NextByte = NC_UTF8;
            return;

            default:
            break;
        }
    }

    SkipUTF:

    switch (byte)
    {
        default:
            switch (CharMapSelection)
            {
                default:
                    LastPrintedChar = byte;
                break;

                case 1:
                    LastPrintedChar = CharMap1[byte];
                break;

                /*case 2:
                    LastPrintedChar = CharMap2[byte];
                break;*/
            }
                        
            TTY_PrintChar(LastPrintedChar);
            
            #ifdef TRM_LOGGING
            if ((byte < 0x20) /*|| (byte > 0x7E)*/) kprintf("[91mTTY_ParseRX: Caught unhandled byte: $%X at position $%lX[0m", byte, RXBytes);
            #endif
        break;
        case 0x1B:  // Escape 1
            NextByte = NC_Escape;
        break;
        case TC_IAC:  // IAC
            NextByte = NC_IAC;
        break;
        case 0x0A:  // Line feed (new line)
        case 0x0B:  // Vertical tab
            if (vNewlineConv == 1) TTY_SetSX(0);  // Convert \n to \n\r

            TTY_MoveCursor(TTY_CURSOR_DOWN, 1);
        break;
        case 0x0D:  // Carriage return
            TTY_SetSX(0);
            TTY_MoveCursor(TTY_CURSOR_DUMMY);   // Dummy
        break;
        case 0x08:  // Backspace
            TTY_MoveCursor(TTY_CURSOR_LEFT, 1);
            //kprintf("[93mBackspace: sx = %d[0m", TTY_GetSX());
        break;
        case 0x09:  // Horizontal tab
            #ifdef TRM_LOGGING
            //kprintf("[93mHorizontal tab: %u -> %u[0m", (TTY_GetSX() % 80), Find_NextTabStop());
            #endif
            
            TTY_SetSX(Find_NextTabStop());
        break;
        case 0x0C:  // Form feed (new page)
            TTY_SetSX(0);
            TTY_SetSY(C_YSTART);

            TTY_ResetVScroll();

            TRM_FillPlane(BG_A, 0);
            TRM_FillPlane(BG_B, 0);

            TTY_MoveCursor(TTY_CURSOR_DUMMY);   // Dummy
        break;
        case 0x0E:  // Shift Out
            CharMapSelection = 1;
        break;
        case 0x0F:  // Shift In
            CharMapSelection = 0;
        break;
        case 0x00:  // Null
        break;
        case 0x05:  // Enquiry
            NET_SendString("SMDT"); // "SMDT" or ""
        break;
        case 0x07:  // Bell
            PSG_setEnvelope(0, PSG_ENVELOPE_MAX);
            waitMs(100);
            PSG_setEnvelope(0, PSG_ENVELOPE_MIN);
            waitMs(100);
            PSG_setEnvelope(0, PSG_ENVELOPE_MAX);
            waitMs(100);
            PSG_setEnvelope(0, PSG_ENVELOPE_MIN);
        break;
        case 0x01:  // Start of heading
        case 0x02:  // Start of text
        case 0x03:  // End of text
        case 0x04:  // End of transmission
        case 0x06:  // Acknowledge
        case 0x15:  // NAK (negative acknowledge)
            #ifdef TRM_LOGGING
            kprintf("[91mUnimplemented VT100 mode single-character function: $%X at $%lX[0m", byte, RXBytes);
            #endif
        break;
    }
}

void ChangeTitle(const char *str)
{
    char TitleBuf[36];

    snprintf(TitleBuf, 36, "%s - %-27s", STATUS_TEXT_SHORT, str);
    TRM_SetStatusText(TitleBuf);
}

// https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
// https://en.wikipedia.org/wiki/ANSI_escape_code#CSIsection
// https://terminalguide.namepad.de/seq/
static void DoEscape(u8 byte)
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
                    if (ESC_ParamSeq == 0) ESC_Param16 = atoi16(ESC_Buffer);

                    if (ESC_ParamSeq >= 10) return;

                    ESC_Param[ESC_ParamSeq++] = atoi(ESC_Buffer);
                    //kprintf("ESC_ParamSeq[%u] = %u (%s)", ESC_ParamSeq-1, ESC_Param[ESC_ParamSeq-1], ESC_Buffer);
                    ////kprintf("Got an ';' : ESC_Param[%u] = $%X", ESC_ParamSeq-1, ESC_Param[ESC_ParamSeq-1]);

                    ESC_Buffer[0] = '\0';
                    ESC_Buffer[1] = '\0';
                    ESC_Buffer[2] = '\0';
                    ESC_Buffer[3] = '\0';
                    ESC_BufferSeq = 0;

                    return;
                }

                case 'A':   // Cursor Up (CUU)
                {
                    u8 n = atoi(ESC_Buffer);
                    n = (n ? n : 1);
                    TTY_SetSY_A(TTY_GetSY_A() - n);
                    TTY_MoveCursor(TTY_CURSOR_DUMMY);
                    goto EndEscape;
                }

                case 'B':   // Cursor Down (CUD)
                {
                    u8 n = atoi(ESC_Buffer);
                    n = (n ? n : 1);
                    TTY_SetSY_A(TTY_GetSY_A() + n);   
                    TTY_MoveCursor(TTY_CURSOR_DUMMY);         
                    goto EndEscape;
                }

                case 'C':   // Cursor Right (CUF)
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
                    TTY_MoveCursor(TTY_CURSOR_DUMMY);
                    goto EndEscape;
                }

                case 'D':   // Cursor Left (CUB)
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
                    TTY_MoveCursor(TTY_CURSOR_DUMMY);
                    goto EndEscape;
                }
                
                case 'E':   // Cursor Next Line (CNL) - "ESC[ Ⓝ E" -- Todo: take care of margins/scroll regions
                {
                    u8 n = atoi(ESC_Buffer);
                    n = (n ? n : 1);
                    TTY_MoveCursor(TTY_CURSOR_DOWN, n);
                    TTY_SetSX(0);
                    goto EndEscape;
                }
                
                case 'F':   // Cursor Previous Line (CPL) - "ESC[ Ⓝ F" -- Todo: take care of margins/scroll regions
                {
                    u8 n = atoi(ESC_Buffer);
                    n = (n ? n : 1);
                    TTY_MoveCursor(TTY_CURSOR_UP, n);
                    TTY_SetSX(0);
                    goto EndEscape;
                }

                case 'G':   // Cursor Horizontal Position Absolute (HPA)
                {
                    u8 n = atoi(ESC_Buffer) - 1;

                    if (vDECOM == FALSE) n = (n >= C_XMAX ? C_XMAX-1 : n);   // Cap X to within the screen if origin mode is off

                    TTY_SetSX(n);

                    goto EndEscape;
                }
                
                case 'I':   // Cursor Horizontal Forward Tabulation (CHT) - "ESC[ Ⓝ I" -- Same as Horizontal Tab (TAB) times Ⓝ
                {
                    u8 n = atoi(ESC_Buffer);
                    n = (n ? n : 1);

                    #ifdef TRM_LOGGING
                    kprintf("[93mHorizontal tab %u times[0m", n);
                    #endif
                    
                    while (n--) TTY_SetSX(Find_NextTabStop());

                    goto EndEscape;
                }

                case 'J':   // Erase Display [Dispatch] (ED)
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

                case 'K':   // Erase Line [Dispatch] (EL) - "ESC[ Ⓝ K"
                {
                    u8 n = atoi(ESC_Buffer);

                    switch (n)
                    {
                        case 1: // Erase start of line to the cursor (Keep cursor position)
                            TTY_ClearPartialLine(sy % 32, 0, TTY_GetSX()+1);
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

                case 'P':   // Delete Character (DCH) - "ESC[ Ⓝ P"
                {
                    u8 n = atoi(ESC_Buffer);
                    s16 cx = TTY_GetSX();
                    n = (n ? n : 1);

                    if (vDECLRMM)
                    {
                        if ((cx + n) > DMarginRight) n = cx - DMarginRight;
                    }
                    else
                    {
                        if ((cx + n) > C_XMAX) n = cx - C_XMAX;
                    }

                    TTY_ClearPartialLine(sy % 32, cx, cx + n);

                    #if ESC_LOGGING >= 2
                    kprintf("[93mESC[%uP (Delete Character) - Not fully implemented; TODO: Move characters right of DCH to the left[0m", n);
                    #endif

                    goto EndEscape;
                }

                case 'S':   // Scroll Up (SU) - "ESC[ Ⓝ S"
                {           
                    u8 n = atoi(ESC_Buffer);
                    n = (n ? n : 1);

                    TTY_DrawScrollback(n);

                    goto EndEscape;
                }

                case 'T':   // 	Scroll Down (SD) / 	Track Mouse / Unset Title Mode
                {
                    u8 n = atoi(ESC_Buffer);
                    n = (n ? n : 1);

                    if (ESC_Buffer[0] == '>')
                    {
                        #if ESC_LOGGING >= 2
                        kprintf("[91mNot implemented: Unset Title Mode[0m");
                        #endif

                        goto EndEscape;
                    }
                    else
                    {
                        // for [T (default 1), or [xT (scroll up x times)
                        // Scroll up n times; shift all lines down n times and blank new lines at top
                    }

                    #if ESC_LOGGING >= 2
                    kprintf("[91mUnknown 'T' control character - BUFFER: 0: %u, 1: %u, 2: %u, 3: %u -- 0: '%c', 1: '%c', 2: '%c', 3: '%c'[0m", ESC_Buffer[0], ESC_Buffer[1], ESC_Buffer[2], ESC_Buffer[3], ESC_Buffer[0], ESC_Buffer[1], ESC_Buffer[2], ESC_Buffer[3] == '\0' ? ' ' : ESC_Buffer[3]);
                    #endif

                    goto EndEscape;
                }
                
                case 'X':   // Erase Character (ECH) - "ESC[ Ⓝ X"
                {
                    u8 n = atoi(ESC_Buffer);
                    n = (n ? n : 1);     // If n == 0 then adjust to n to 1

                    s16 oldsx = TTY_GetSX();
                    s16 oldsy = TTY_GetSY();
                    
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

                case 'Z':   // Cursor Horizontal Backward Tabulation (CBT) - "ESC[ Ⓝ Z"
                {
                    #ifdef TRM_LOGGING
                    u8 n = atoi(ESC_Buffer);
                    n = (n ? n : 1);
                    kprintf("[93mHorizontal tab: %u <- %u (n = %u)[0m", Find_LastTabStop(), (TTY_GetSX() % 80), n);
                    #endif

                    TTY_SetSX(Find_LastTabStop());

                    goto EndEscape;
                }

                case 'b':   // Repeat last printed character n times
                {
                    u8 n = atoi(ESC_Buffer);

                    for (u8 i = 0; i < n; i++) TTY_PrintChar(LastPrintedChar);
                    
                    goto EndEscape;
                }

                case 'c':   // ... Multiple ...
                {
                    switch (ESC_Buffer[0])
                    {
                        case '?':   // Linux Cursor Style - ""
                        {
                            break;
                        }

                        case '>':   // Secondary Device Attributes (DA2) - "ESC[ > Ⓝ c"
                        {
                            u8 model = 64;
                            u16 version = 520;
                            char str[16];
                            u16 len = 0;
        
                            len = sprintf(str, "[>%u;%u;0c", model, version);
                            NET_SendStringLen(str, len);
        
                            #ifdef ESC_LOGGING
                            kprintf("[93mDA2: p0 = %u -- Sending \"ESC[>%u;%u;0c\" regardless of p0[0m", ESC_Buffer[1], model, version);
                            #endif

                            break;
                        }

                        case '=':   // Tertiary Device Attributes (DA3) - ""
                        {
                            if (ESC_Buffer[1] == '0')
                            {
                                NET_SendString("P!|00000000\\");
        
                                #ifdef ESC_LOGGING
                                kprintf("[93mDA3: p0 = %u -- Sending \"ESCP!|00000000ESC\\\"[0m", ESC_Buffer[1]);
                                #endif
                            }
                            else
                            {        
                                #ifdef ESC_LOGGING
                                kprintf("[93mDA3: p0 = %u -- Ignoring because p0 is non zero[0m", ESC_Buffer[1]);
                                #endif
                            }

                            break;
                        }
                    
                        default:    // Primary Device Attributes (DA1) - ""
                        {
                            NET_SendString("[?1;2c");
        
                            #ifdef ESC_LOGGING
                            kprintf("[93mDA1: p0 = %u -- Sending \"ESC[?1;2c\" regardless of p0[0m", ESC_Buffer[0]);
                            #endif

                            break;
                        }
                    }

                    goto EndEscape;
                }

                case 'd':   // Cursor Vertical Position Absolute (VPA) - "ESC[ Ⓝ d"
                {
                    u8 n = atoi(ESC_Buffer);
                    //n = (n ? n : 1);

                    #if ESC_LOGGING >= 3
                    kprintf("ESC[%ud", n);
                    #endif

                    /* Why is this here?
                    if ((n >= DMarginBottom) && (DMarginBottom < 24))
                    {
                        u8 lines = n - DMarginBottom;
                        TTY_DrawScrollback(lines+1);
                    }*/

                    if (vDECOM)
                    {
                        TTY_SetSY_A(DMarginTop + (n-1));
                    }
                    else TTY_SetSY_A(n-1);

                    TTY_MoveCursor(TTY_CURSOR_DUMMY);

                    PendingWrap = FALSE;

                    goto EndEscape;
                }

                case 'e':   // Line Position Relative [rows] (VPR)
                {
                    u8 n = atoi(ESC_Buffer);
                    n = (n ? n : 1);

                    u8 new_row = TTY_GetSY_A() + n;
                    u8 max = C_SYSTEM_YMAX;

                    if (new_row > max)
                    {
                        new_row = max;
                    }

                    TTY_SetSY_A(new_row -1);

                    TTY_MoveCursor(TTY_CURSOR_DUMMY);

                    goto EndEscape;
                }

                case 'H':   // Set Cursor Position (CUP) - "ESC[ Ⓝ ; Ⓝ H" -- Move cursor to upper left corner if no parameters or to yy;xx
                case 'f':   // Alias: Set Cursor Position - "ESC[ Ⓝ ; Ⓝ f"
                {
                    if (ESC_Buffer[0] != '\0') ESC_Param[ESC_ParamSeq++] = atoi(ESC_Buffer);
                    
                    //for (u8 i = 0; i < ESC_ParamSeq; i++) kprintf("ESC_ParamSeq[%u] = %u", i, ESC_Param[i]);

                    if ((ESC_Param[0] == 0xFF) && (ESC_Param[1] == 0xFF))
                    {
                        TTY_SetSX(DMarginLeft);
                        TTY_SetSY_A(DMarginTop);
                    }
                    else
                    {
                        if (ESC_Param[0] == 0) ESC_Param[0] = 1;

                        if (ESC_Param[1] == 0) ESC_Param[1] = 1;

                        if (ESC_Param[1] != 0xFF)
                        {
                            u8 x = ESC_Param[1]-1;
                            
                            if (vDECOM)
                            {
                                if ((x >= DMarginLeft) && (x <= DMarginRight))
                                {
                                    TTY_SetSX(x);
                                }
                                else if (x >= DMarginRight)
                                {
                                    TTY_SetSX(DMarginRight);
                                }
                                else if (x <= DMarginLeft)
                                {
                                    TTY_SetSX(DMarginLeft);
                                }
                            }
                            else
                            {
                                TTY_SetSX(x);
                            }

                            /*if (ESC_Param[1]-1 < DMarginRight) TTY_SetSX(ESC_Param[1]-1);
                            else TTY_SetSX(DMarginRight);
                            
                            if (ESC_Param[1]-1 > DMarginLeft) TTY_SetSX(ESC_Param[1]-1);
                            else TTY_SetSX(DMarginLeft);*/

                            //kprintf("[92mCUP cursor X: %d - (p1: %u -- ML: %u -- MR: %u)[0m", TTY_GetSX(), ESC_Param[1], DMarginLeft, DMarginRight);
                        }
                        
                        if (ESC_Param[0] != 0xFF)
                        {
                            u8 y = ESC_Param[0]-1;

                            //if (vDECOM)
                            //{
                                if (y <= DMarginTop) y = DMarginTop;

                                if (y >= DMarginBottom) y = DMarginBottom;
                            //}

                            TTY_SetSY_A(y);

                            //kprintf("[94mCUP cursor Y: %d - (p0: %u -- MT: %u -- MB: %u)[0m", TTY_GetSY_A(), ESC_Param[0], DMarginTop, DMarginBottom);
                        }
                    }

                    TTY_MoveCursor(TTY_CURSOR_DUMMY);   // Dummy

                    goto EndEscape;
                }

                case 'g':   // Tab Clear (TBC) - "ESC[ Ⓝ g"
                {
                    u8 n = atoi(ESC_Buffer);                    
                    u8 c = (TTY_GetSX() % 80);

                    switch (n)
                    {
                        case 0:
                            HTS_Column[c] = 0;

                            #ifdef TRM_LOGGING
                                kprintf("Clearing tab stop in column %u (CMD = %u)", c, n);
                            #endif
                        break;

                        case 2:
                        case 3:
                        case 5:
                            memset(HTS_Column, 0, 80);

                            #ifdef TRM_LOGGING
                                kprintf("Clearing tab stops in all columns. (CMD = %u)", n);
                            #endif
                        break;
                    
                        default:
                            #ifdef TRM_LOGGING
                                kprintf("[91mUnknown TBC (CMD = %u)[0m", n);
                            #endif
                        break;
                    }

                    goto EndEscape;
                }

                case 'l':   // Insert Line (IL)
                {
                    u8 n = atoi(ESC_Buffer);

                    // This is not correct, but I have no good way to scroll previously printed lines other than reading back from VRAM -.-
                    // Before clearing the lines in question, they should be scrolled down n rows first!

                    TTY_ClearLine(sy % 32, n-1);

                    #ifdef ESC_LOGGING
                    kprintf("Moving/Erasing lines; %d to %d", (sy % 32), ((sy % 32)+n)-1);
                    #endif

                    goto EndEscape;
                }

                case 'm':   // Select Graphic Rendition (SGR)
                {
                    ESC_Param[ESC_ParamSeq++] = atoi(ESC_Buffer);
                    u8 rgb;
                    u8 base = 0;

                    DoSecondSet:                    
                    switch (ESC_Param[base+1])
                    {
                        case 5:     // 0-7: standard colors (as in ESC [ 30–37 m) and 8-15: high intensity colors (as in ESC [ 90–97 m)
                            if (ESC_Param[base+0] == 38)
                            {
                                if (ESC_Param[base+2] <= 15) TTY_SetAttribute(ESC_Param[base+2] + (ESC_Param[base+2] <= 7 ? 30 : 90));
                                #ifdef ESC_LOGGING
                                else
                                    kprintf("Attempted to set attribute colour > 15");
                                #endif
                            }
                            else if (ESC_Param[base+0] == 48)
                            {
                                if (ESC_Param[base+2] <= 15) TTY_SetAttribute(ESC_Param[base+2] + (ESC_Param[base+2] <= 7 ? 40 : 100));
                                #ifdef ESC_LOGGING
                                else
                                    kprintf("Attempted to set attribute colour > 15");
                                #endif
                            }
                        break;

                        case 2:     // RGB24 logic
                            rgb = (((ESC_Param[base+4] + ESC_Param[base+3] + ESC_Param[base+2]) >= 256) ? (ESC_Param[base+0] == 38 ? 90 : 100) : (ESC_Param[base+0] == 38 ? 30 : 40)) +  // Attempt to find out if this colour is bright enough to warrant using the upper 8 colours
                                 ((((ESC_Param[base+4] & 0xC0) != 0) << 2) | // B
                                  (((ESC_Param[base+3] & 0xC0) != 0) << 1) | // G
                                   ((ESC_Param[base+2] & 0xC0) != 0));       // R

                            TTY_SetAttribute(rgb);

                            #if ESC_LOGGING >= 4
                            kprintf("Attempting to set truncated RGB colour %u (R: %u  -- G: %u -- B: %u)", rgb, ESC_Param[base+2], ESC_Param[base+3], ESC_Param[base+4]);
                            #endif
                        break;
                    
                        default:
                            for (int i = 0; i < 4; i++)
                            {
                                if (ESC_Param[base+i] != 255) TTY_SetAttribute(ESC_Param[base+i]);
                            }
                        break;
                    }

                    #ifdef ESC_LOGGING
                    //kprintf("TTY_SetAttribute: 0:<%u> 1:<%u> 2:<%u> 3:<%u> 4:<%u>  at $%lX", ESC_Param[0], ESC_Param[1], ESC_Param[2], ESC_Param[3], ESC_Param[4], RXBytes);
                    #endif
                    
                    if ((ESC_ParamSeq == 10) && (base != 5))
                    {
                        base = 5;
                        goto DoSecondSet;
                    }

                    goto EndEscape;
                }

                case 'n':   // Device Status Report [Dispatch] (DSR)
                {            
                    u8 n = atoi(ESC_Buffer);
                    char str[16];
                    u16 len = 0;
                    memset(str, 0, 16);

                    switch (n)
                    {
                        case 5: // Report Operating Status
                            NET_SendString("[0n");
                        break;

                        case 6: // Cursor Position Report (CPR)
                            len = sprintf(str, "[%d;%dR", TTY_GetSY_A() + 1, TTY_GetSX() + 1);
                            NET_SendStringLen(str, len);
                            #ifdef ESC_LOGGING
                            kprintf("[93mReporting cursor position: \"ESC[%d;%dR\"[0m", TTY_GetSY_A() + 1, TTY_GetSX() + 1);
                            #endif
                        break;

                        case 8: // Set Title to Terminal Name and Version.
                            // This could easily be set here, however current versions of SMDT prefixes all titles with this information already
                        break;

                        default:
                            #ifdef ESC_LOGGING
                            kprintf("[91mDevice Status Report [Dispatch] (DSR) - Unknown command $%X at $%lX[0m", n, RXBytes);
                            #endif
                        break;
                    }

                    goto EndEscape;
                }

                case 'p':   // Soft Reset (DECSTR) / Request Mode (RQM) / Alias: Save Rendition Attributes / ??? DECSR / Select VT-XXX Conformance Level (DECSCL)
                {
                    ESC_Param[ESC_ParamSeq++] = atoi(ESC_Buffer);

                    // Soft Reset (DECSTR)
                    if (ESC_Buffer[0] == '!')
                    {
                        #ifdef ESC_LOGGING
                        kprintf("Soft Reset (DECSTR)");
                        #endif

                        TELNET_Init(TF_ClearScreen);
                    }
                    // Select VT-XXX Conformance Level (DECSCL)
                    else if (ESC_Buffer[1] == '"')
                    {
                        u8 level = ESC_Param[0];
                        // Not used, remove ifdef when its needed:
                        #ifdef ESC_LOGGING
                        u8 bit7 = ESC_Buffer[0] - 48;
                        #endif

                        if (level < 61) 
                        {
                            #ifdef ESC_LOGGING
                            kprintf("[93mDECSCL Skipping because level < 61[0m");
                            #endif
                        }
                        else
                        {
                            level -= 60;
                            
                            #ifdef ESC_LOGGING
                            kprintf("[93mDECSCL Level: %u - 7bit: %s[0m", level, bit7 ? "yes" : "no");
                            #endif
                        }

                    }
                    // Request Mode (RQM)
                    else if (ESC_Buffer[1] == '$')
                    {
                        // Not used, remove ifdef when its needed:
                        #ifdef ESC_LOGGING
                        u8 data_end = ESC_Buffer[0] - 48;
                        #endif
                        
                        #ifdef ESC_LOGGING
                        kprintf("[93mRequest Mode (RQM): %u[0m", data_end);
                        #endif
                    }
                    #ifdef ESC_LOGGING
                    else
                    {
                        kprintf("[91mUnknown ESC[p - Data:[0m");

                        for (u8 i = 0; i < ESC_ParamSeq; i++)
                        {
                            kprintf("[93mESC[p - PARAM[%i]: %u -- $%X -- '%c'[0m", i, ESC_Param[i], ESC_Param[i], ESC_Param[i]);
                        }
    
                        kprintf("[93mESC[p - BUFFER: 0: %u, 1: %u, 2: %u, 3: %u -- 0: '%c', 1: '%c', 2: '%c', 3: '%c'[0m", ESC_Buffer[0], ESC_Buffer[1], ESC_Buffer[2], ESC_Buffer[3], ESC_Buffer[0], ESC_Buffer[1], ESC_Buffer[2], ESC_Buffer[3] == '\0' ? ' ' : ESC_Buffer[3]);
    
                    }
                    #endif

                    goto EndEscape;
                }

                case 'q':   // ... Multiple ...
                {
                    u8 n = ESC_Buffer[0] - 48;

                    if (ESC_Buffer[1] == '\0')
                    {
                        if (n < 4)  // Load LEDs (DECLL) - "ESC[ Ⓝ q"
                        {                            
                            #if ESC_LOGGING >= 2
                            kprintf("[91mNot implemented: Load LEDs (DECLL) at $%lX (CMD = %u)[0m", RXBytes, n);
                            #endif
                        }
                        else if (n == '#')  // Alias: Restore Rendition Attributes - "ESC[ # q" - same as "ESC[ # }"
                        {
                            #if ESC_LOGGING >= 2
                            kprintf("[91mNot implemented: (Alias) Restore Rendition Attributes at $%lX[0m", RXBytes);
                            #endif
                        }
                        
                        goto EndEscape;
                    }

                    char type = ESC_Buffer[1];

                    switch (type)
                    {
                        case '\"':    // Select Character Protection Attribute (DECSCA) - "ESC[ Ⓝ " q"
                        {
                            #if ESC_LOGGING >= 2
                            kprintf("[91mNot implemented: Select Character Protection Attribute (DECSCA) at $%lX (CMD = %u)[0m", RXBytes, n);
                            #endif

                            break;
                        }

                        case '*':    // ??? DECSR - "ESC[ Ⓝ * q"
                        {
                            #if ESC_LOGGING >= 2
                            kprintf("[91mNot implemented: ??? DECSR at $%lX (CMD = %u)[0m", RXBytes, n);
                            #endif

                            break;
                        }

                        case ' ':   // Select Cursor Style (DECSCUSR) - "ESC[ Ⓝ ␣ q" - (␣ = Space)
                        {
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
        
                            #if ESC_LOGGING >= 3
                            kprintf("[91mSelect Cursor Style (DECSCUSR) at $%lX (CMD = %u)[0m", RXBytes, n);
                            #endif

                            break;
                        }
                        
                        default:
                        #ifdef ESC_LOGGING
                        kprintf("[91mESC[..q - Unknown type '%c' at $%lX[0m", type, RXBytes);
                        #endif
                        break;
                    }

                    goto EndEscape;
                }

                case 'r':   // Set Top and Bottom Margins (DECSTBM) - "ESC[ Ⓝ ; Ⓝ r" --- Untested quickly hacked together, probably does not work
                {
                    ESC_Param[ESC_ParamSeq++] = atoi(ESC_Buffer);

                    if ((ESC_Param[1] == 0) || (ESC_Param[1] == 255) || (ESC_Param[1] > C_YMAX)) ESC_Param[1] = C_YMAX;

                    if (ESC_Param[0] < ESC_Param[1])
                    {                        
                        DMarginTop = ESC_Param[0] - 1;
                        DMarginBottom = ESC_Param[1] - 1;

                        if (vDECOM)
                        {
                            TTY_SetSX(DMarginLeft);     // DMarginLeft or 0 ?
                            TTY_SetSY_A(DMarginTop);
                        }
                        else
                        {                            
                            TTY_SetSX(0);
                            TTY_SetSY_A(0);
                        }
                    }
                    else
                    {
                        DMarginTop = 0;
                        DMarginBottom = C_YMAX;
                    }

                    //TTY_DrawScrollback();
                    TTY_MoveCursor(TTY_CURSOR_DUMMY);

                    #if ESC_LOGGING >= 2
                    kprintf("ESC[%u;%ur -- p0<p1: %s -- vDECOM: %s -- C_YMAX: %u", ESC_Param[0], ESC_Param[1], (ESC_Param[0] < ESC_Param[1]) ? "TRUE" : "FALSE", vDECOM ? "TRUE" : "FALSE", C_YMAX);
                    #endif

                    goto EndEscape;
                }

                case 's':   // Set Left and Right Margin (DECSLRM) when in DECLRMM mode, otherwise it is: Save Cursor [variant] (ansi.sys) - Same as Save Cursor (DECSC) (ESC 7)
                {
                    if (vDECLRMM)
                    {
                        ESC_Param[ESC_ParamSeq++] = atoi(ESC_Buffer);

                        if ((ESC_Param[1] == 0xFF) || (ESC_Param[1] == 0) || (ESC_Param[1] >= C_XMAX))
                        {
                            DMarginRight = C_XMAX;
                        }
                        else DMarginRight = ESC_Param[1] - 1;

                        if (ESC_Param[0] > ESC_Param[1])
                        {
                            DMarginLeft = DMarginRight;
                        }
                        else DMarginLeft = ESC_Param[0] - 1;
                    }
                    else
                    {
                        u8 buffer = BufferSelect == 80 ? 1 : 0;
                        Saved_sx[buffer] = TTY_GetSX();
                        Saved_sy[buffer] = TTY_GetSY();
                    }

                    goto EndEscape;
                }

                case 't':   // Window operations [DISPATCH] - https://terminalguide.namepad.de/seq/csi_st/
                {                    
                    ESC_Param[ESC_ParamSeq++] = atoi(ESC_Buffer);
                    u8 n = ESC_Param[0];
                    char str[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
                    u16 len = 0;

                    // >3 -- fix this
                    // b0: 56 - b1: 48 - b2: 0 - b3: 0
                    if (ESC_Buffer[0] == 51)   // >2
                    {
                        #if ESC_LOGGING >= 2
                        kprintf("[91mNot implemented: Set Title Mode: 2 & %u[0m", ESC_Param[1]);
                        #endif

                        goto EndEscape;
                    }

                    switch (n)
                    {
                        case 1:     // Restore Terminal Window - "ESC[ 1 t"
                        {                         
                            #if ESC_LOGGING >= 3
                            kprintf("Restore Terminal Window");
                            #endif

                            vMinimized = FALSE;
                            break;
                        }

                        case 2:     // Minimize Terminal Window - "ESC[ 2 t"
                        {
                            #if ESC_LOGGING >= 3
                            kprintf("Minimize Terminal Window");
                            #endif

                            vMinimized = TRUE;
                            break;
                        }

                        case 3:     // Set Terminal Window Position - "ESC[ 3 ; Ⓝ ; Ⓝ t"   Ⓝ = posx/posy
                        {
                            if (ESC_Param[1] == 255) TermPosX = 0;
                            else TermPosX = ESC_Param[1];

                            if (ESC_Param[2] == 255) TermPosY = 0;
                            else TermPosY = ESC_Param[2];

                            #if ESC_LOGGING >= 2
                            kprintf("Set Terminal Window Position - x: %u, y: %u", ESC_Param[1], ESC_Param[2]);
                            #endif

                            break;
                        }

                        case 7:     // Refresh/Redraw Terminal Window - "ESC[ 7 t"
                        {
                            #if ESC_LOGGING >= 2
                            kprintf("[91mNot implemented: Refresh/Redraw Terminal Window. n = %u[0m", n);
                            #endif

                            break;
                        }

                        case 8:     // Set Terminal Window Size - "ESC[ 8 ; Ⓝ ; Ⓝ t"  Ⓝ = H/W in rows/columns
                        {
                            if (((ESC_Param[2] > 0) && (ESC_Param[2] != 255)) && ((ESC_Param[1] > 0) && (ESC_Param[1] != 255)))
                            {
                                u8 max_y = C_SYSTEM_YMAX;
                                C_XMAX = (ESC_Param[2] > 80 ? 80 : ESC_Param[2]);
                                C_YMAX = (ESC_Param[1] > max_y ? max_y : ESC_Param[1]);
                            }

                            #if ESC_LOGGING >= 3
                            kprintf("Set terminal size: %u x %u", ESC_Param[2], ESC_Param[1]);
                            #endif
                        
                            break;
                        }

                        case 9:     // Maximize Terminal Window - "ESC[ 9 ; Ⓝ t"
                        case 10:    // Alias: Maximize Terminal - "ESC[ 10 ; Ⓝ t" (Does not use the Ⓝ the same way as 9, fixme)
                        {
                            vMinimized = FALSE;

                            #if ESC_LOGGING >= 3
                            kprintf("Maximize Terminal Window - CMD = %u", ESC_Param[1]);
                            #endif
                            break;
                        }

                        case 11:    // Report Terminal Window State (1 = non minimized - 2 = minimized)
                        {
                            if (vMinimized) NET_SendString("[2t");
                            else NET_SendString("[1t");
                            
                            break;
                        }

                        case 13:    // Report Terminal Window Position - "ESC[ 13 ; Ⓝ t"  Ⓝ = 0/2
                        {
                            len = sprintf(str, "[3;%u;%ut", (u16)TermPosX, (u16)TermPosY);
                            NET_SendStringLen(str, len);
                        
                            break;
                        }

                        case 14:    // Report Terminal Window Size in Pixels - "ESC[ 14 ; Ⓝ t"  Ⓝ = 0/2
                        {
                            len = sprintf(str, "[4;%u;%ut", C_YMAX * 8, C_XMAX * 8);
                            NET_SendStringLen(str, len);

                            #if ESC_LOGGING >= 3
                            kprintf("Reporting window size: %u x %u", C_XMAX * 8, C_YMAX * 8);
                            #endif
                            
                            break;
                        }

                        case 15:    // Report Screen Size in Pixels - "ESC[ 15 t"
                        {
                            len = sprintf(str, "[5;%u;%ut", C_YMAX * 8, C_XMAX * 8);
                            NET_SendStringLen(str, len);

                            #if ESC_LOGGING >= 3
                            kprintf("Reporting screen size: %u x %u - Position: $%lX", C_XMAX * 8, C_YMAX * 8, RXBytes);
                            #endif
                        
                            break;
                        }

                        case 16:    // Report Cell Size in Pixels - "ESC[ 16 t"
                        {
                            NET_SendString("[6;8;8t");
                            break;
                        }

                        case 18:    // Report Terminal Size - "ESC[ 18 t"
                        {
                            len = sprintf(str, "[8;%u;%ut", C_YMAX, C_XMAX);
                            NET_SendStringLen(str, len);
                            
                            //NET_SendString("[8;25;80t");

                            #if ESC_LOGGING >= 3
                            kprintf("Reporting terminal size: %u x %u - Position: $%lX", C_XMAX, C_YMAX, RXBytes);
                            #endif
                        
                            break;
                        }

                        case 19:    // Report Screen Size - "ESC[ 19 t"
                        {
                            len = sprintf(str, "[9;%u;%ut", C_YMAX, C_XMAX);
                            NET_SendStringLen(str, len);

                            #if ESC_LOGGING >= 3
                            kprintf("Reporting screen size: %u x %u", C_XMAX, C_YMAX);
                            #endif

                            break;
                        }

                        case 20:    // Get Icon Title - "ESC[ 20 t"
                        {
                            char lstr[64];
                            u16 llen = 0;

                            llen = sprintf(lstr, "]L%s\\", FakeIconLabel);
                            NET_SendStringLen(lstr, llen); // Reply with a fake icon string; this control sequence turned out to be a security hazard
                            break;
                        }

                        case 21:    // Get Terminal Title - "ESC[ 21 t"
                        {
                            char lstr[64];
                            u16 llen = 0;

                            llen = sprintf(lstr, "]l%s\\", FakeWindowLabel);
                            NET_SendStringLen(lstr, llen); // Reply with a fake title string; this control sequence turned out to be a security hazard
                            break;
                        }

                        case 22:    // Push Terminal Title - "ESC[ 22 ; Ⓝ t" Ⓝ = 0/1/2
                        {
                            // If cmd = 0, cmd = 2 or the stack is empty saves the terminal title to the stack, otherwise duplicates the title of the top-most stack entry.

                            switch (ESC_Param[1])
                            {
                                case 0:
                                case 2:
                                    if (WindowNum < MAX_LABEL_SSIZE)
                                    {
                                        strcpy(LabelStack[WindowNum], FakeWindowLabel);
                                        WindowNum++;
                                    }

                                    #if ESC_LOGGING >= 2
                                    kprintf("Push Terminal Title \"%s\" to stack position %u", FakeWindowLabel, WindowNum-1);
                                    #endif
                                break;

                                case 1:
                                    if (IconNum < MAX_LABEL_SSIZE)
                                    {
                                        strcpy(LabelStack[ICON_LABEL_OFFSET + IconNum], FakeIconLabel);
                                        IconNum++;
                                    }

                                    #if ESC_LOGGING >= 2
                                    kprintf("Push Terminal Icon \"%s\" to stack position %u", FakeIconLabel, IconNum-1);
                                    #endif
                                break;
                            
                                default:
                                    // Duplicate here ?
                                    if (WindowNum < MAX_LABEL_SSIZE)
                                    {
                                        strcpy(LabelStack[WindowNum], LabelStack[WindowNum+1]);
                                        WindowNum++;
                                    }

                                    if (IconNum < MAX_LABEL_SSIZE)
                                    {
                                        strcpy(LabelStack[ICON_LABEL_OFFSET + IconNum], LabelStack[ICON_LABEL_OFFSET + IconNum + 1]);
                                        IconNum++;
                                    }

                                    #if ESC_LOGGING >= 2
                                    kprintf("[93mPush Terminal Title/Icon; Duplicate topmost item? n = %u[0m", ESC_Param[1]);
                                    #endif
                                break;
                            }

                            break;
                        }

                        case 23:    // Pop Terminal Title - "ESC[ 23 ; Ⓝ t" Ⓝ = 0/1/2
                        {
                            // If cmd = 0 or cmd = 2 restores and removes the terminal title from the stack, otherwise removes the saved terminal title from the stack without restoring it.

                             switch (ESC_Param[1])
                            {
                                case 0:
                                case 2:
                                    if (WindowNum > 0)
                                    {
                                        memset(LabelStack[WindowNum], 0, 40);
                                        WindowNum--;
                                        ChangeTitle(LabelStack[WindowNum]);
                                    }

                                    #if ESC_LOGGING >= 2
                                    kprintf("Pop Terminal Title, new title: \"%s\"", LabelStack[WindowNum]);
                                    #endif
                                break;

                                case 1:
                                    if (IconNum > 0)
                                    {
                                        memset(LabelStack[ICON_LABEL_OFFSET + IconNum], 0, 40);
                                        IconNum--;
                                    }

                                    #if ESC_LOGGING >= 2
                                    kprintf("Pop Terminal Icon, new icon: \"%s\"", LabelStack[IconNum]);
                                    #endif
                                break;
                            
                                default:
                                    // Remove topmost item from stack?
                                    memset(LabelStack[WindowNum], 0, 40);
                                    WindowNum--;
                                    memset(LabelStack[ICON_LABEL_OFFSET + IconNum], 0, 40);
                                    IconNum--;

                                    #if ESC_LOGGING >= 2
                                    kprintf("[93mPop Terminal Title/Icon; Remove topmost item from stack? n = %u[0m", ESC_Param[1]);
                                    #endif
                                break;
                            }
                            
                            break;
                        }

                        default:
                        {
                            if (n >= 24)    // Special case, CMD >= 24 resize the window similar to set terminal window size (ESC[8;Ⓝ;Ⓝt) but using the current width and cmd as height.
                            {
                                u8 max_y = C_SYSTEM_YMAX;
                                C_YMAX = (n > max_y ? max_y : n);
                            }
                            else
                            {
                                #ifdef ESC_LOGGING
                                kprintf("[91mUnknown Window operation [DISPATCH]; n = %u (%u %u %u %u) at $%lX[0m", n, ESC_Buffer[0], ESC_Buffer[1], ESC_Buffer[2], ESC_Buffer[3], RXBytes);
                                #endif
                            }

                            #if ESC_LOGGING >= 3
                            kprintf("Window operations [DISPATCH]: p1: %u, p2: %u, p3: %u, p4: %u", ESC_Param[0], ESC_Param[1], ESC_Param[2], ESC_Param[3]);
                            kprintf("Window operations [DISPATCH]: b1: %u, b2: %u, b3: %u, b4: %u", ESC_Buffer[0], ESC_Buffer[1], ESC_Buffer[2], ESC_Buffer[3]);
                            #endif
                        
                            break;
                        }
                    }
                    
                    goto EndEscape;
                }

                case 'u':   // Restore Cursor [variant] (ansi.sys) - Same as Restore Cursor (DECRC) (ESC 8)
                {
                    u8 buffer = BufferSelect == 80 ? 1 : 0;

                    TTY_SetSX(Saved_sx[buffer]);
                    TTY_SetSY(Saved_sy[buffer]);

                    DMarginTop = Saved_OrgTop[buffer];
                    DMarginBottom = Saved_OrgBottom[buffer];

                    goto EndEscape;
                }
                
                case 'x':   // ... Multiple ...
                {
                    char type = 0;
                    u8 c = 0;
                    u16 param4 = 0;

                    if (ESC_Buffer[0] == '$')   // Default parameters (no parameters given)
                    {
                        type = ESC_Buffer[0];
                    }
                    else if (ESC_Buffer[3] != '\0') 
                    {
                        type = ESC_Buffer[3];

                        param4 += (u8) (ESC_Buffer[0] - '0');
                        c++;
                        param4 *= 10;
                        param4 += (u8) (ESC_Buffer[1] - '0');
                        c++;
                        param4 *= 10;
                        param4 += (u8) (ESC_Buffer[2] - '0');
                    }
                    else if (ESC_Buffer[2] != '\0') 
                    {
                        type = ESC_Buffer[2];

                        param4 += (u8) (ESC_Buffer[0] - '0');
                        c++;
                        param4 *= 10;
                        param4 += (u8) (ESC_Buffer[1] - '0');
                    }
                    else if (ESC_Buffer[1] != '\0') 
                    {
                        type = ESC_Buffer[1];

                        param4 += (u8) (ESC_Buffer[0] - '0');
                    }

                    #ifdef ESC_LOGGING
                    kprintf("[93mx control: %u - %u %u %u %u %u %u %u - p4: %u - Type: '%c' - Position: $%lX[0m", ESC_Param16, ESC_Param[1], ESC_Param[2], ESC_Param[3], ESC_Param[4], ESC_Param[5], ESC_Param[6], ESC_Param[7], param4, type, RXBytes);
                    #endif

                    switch (type)
                    {
                        case '*':   // Select Attribute Change Extent (DECSACE)
                        {
                            #ifdef ESC_LOGGING
                            kprintf("[91mNot implemented: Select Attribute Change Extent (DECSACE)[0m");
                            #endif

                            break;
                        }

                        case '$':   // Fill Rectangular Area (DECFRA)
                        {
                            u8 c      = ESC_Param[0];
                            u8 top    = ESC_Param[1] - 1;
                            u8 left   = ESC_Param[2] - 1;
                            u8 bottom = ESC_Param[3] - 1;
                            u8 right  = param4 - 1;

                            top    = (top    == 254 ? 0 : top);
                            left   = (left   == 254 ? 0 : left);
                            bottom = (bottom == 254 ? 0 : bottom);
                            right  = (right  == 255 ? 0 : right);

                            if (vDECOM)
                            {                                
                                top    = (top    < DMarginTop  ? DMarginTop  : top);
                                left   = (left   < DMarginLeft ? DMarginLeft : left);
                                bottom = (bottom > C_YMAX      ? C_YMAX      : bottom);
                                right  = (right  > C_XMAX      ? C_XMAX      : right);
                            }

                            /*if (vDECOM)
                            {
                                if (top < DMarginTop){top = DMarginTop;}
                                if (bottom > C_YMAX){bottom = C_YMAX;}
                                if (left < DMarginLeft){left = DMarginLeft;}
                                if (right < C_XMAX){right = C_XMAX;}
                            }*/

                            if ((top > bottom) || (left > right) || 
                                (c <= 32 || (c > 127 && c <= 160) || c >= 255))
                            {
                                #ifdef ESC_LOGGING
                                kprintf("DECFRA: Skipping fill... T:%u L:%u B:%u R:%u", top, left, bottom, right);
                                #endif

                                break;
                            }

                            s16 tmp_sx = TTY_GetSX();
                            s16 tmp_sy = TTY_GetSY_A();

                            for (u8 y = top; y <= bottom; y++)
                            {
                                TTY_SetSY_A((s16)y);
                                for (u8 x = left; x <= right; x++)
                                {
                                    TTY_SetSX((s16)x);
                                    TTY_PrintChar(c);
                                }
                            }

                            TTY_SetSX(tmp_sx);
                            TTY_SetSY_A(tmp_sy);

                            #ifdef ESC_LOGGING
                            kprintf("[93mDECFRA: Top: %u, Bottom: %u, Left: %u, Right: %u - Char: '%c' - Should skip: %s[0m", top, bottom, left, right, c, ((top > bottom) || (left > right) || (c <= 32 || (c > 127 && c <= 160) || c >= 255)) ? "True" : "False");
                            #endif

                            break;
                        }
                        
                        default:
                        #ifdef ESC_LOGGING
                        kprintf("[91mUnknown x control character: %c[0m", type);
                        #endif

                        break;
                    }

                    goto EndEscape;
                }

                case 'y':   // ... Multiple ...
                {
                    char type = 0;
                    u8 c = 0;
                    u16 param4 = 0;

                    if (ESC_Buffer[3] != '\0') 
                    {
                        type = ESC_Buffer[3];

                        param4 += (u8) (ESC_Buffer[0] - '0');
                        c++;
                        param4 *= 10;
                        param4 += (u8) (ESC_Buffer[1] - '0');
                        c++;
                        param4 *= 10;
                        param4 += (u8) (ESC_Buffer[2] - '0');
                    }
                    else if (ESC_Buffer[2] != '\0') 
                    {
                        type = ESC_Buffer[2];

                        param4 += (u8) (ESC_Buffer[0] - '0');
                        c++;
                        param4 *= 10;
                        param4 += (u8) (ESC_Buffer[1] - '0');
                    }
                    else if (ESC_Buffer[1] != '\0') 
                    {
                        type = ESC_Buffer[1];

                        param4 += (u8) (ESC_Buffer[0] - '0');
                    }

                    #ifdef ESC_LOGGING
                    kprintf("[93my control: %u %u %u %u %u %u %u %u - p4: %u - Type: '%c' - Position: $%lX[0m", ESC_Param16, ESC_Param[1], ESC_Param[2], ESC_Param[3], ESC_Param[4], ESC_Param[5], ESC_Param[6], ESC_Param[7], param4, type, RXBytes);
                    #endif

                    u16 pid = ESC_Param16;
                    char str[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
                    u16 len = 0;
                    //memset(str, 0, 16);

                    switch (type)
                    {
                        case '*':   // Request Checksum of Rectangular Area (DECRQCRA) - "ESC[ Ⓝ ; Ⓝ ; Ⓝ ; Ⓝ ; Ⓝ ; Ⓝ * y"
                        {
                            u32 rb = 0;
                            u8 top    = ESC_Param[2] - 1;
                            u8 left   = ESC_Param[3] - 1;
                            u8 bottom = ESC_Param[4] - 1;
                            u8 right  = param4 - 1;

                            top    = (top    == 254 ? 0 : top);
                            left   = (left   == 254 ? 0 : left);
                            bottom = (bottom == 254 ? 0 : bottom);
                            right  = (right  == 255 ? 0 : right);

                            if (vDECOM)
                            {                                
                                top    = (top    < DMarginTop    ? DMarginTop    : top);
                                left   = (left   < DMarginLeft   ? DMarginLeft   : left);
                                bottom = (bottom > C_YMAX      ? C_YMAX      : bottom);
                                right  = (right  > C_XMAX      ? C_XMAX      : right);
                            }

                            for (u8 y = top; y <= bottom; y++)
                            {
                                for (u8 x = left; x <= right; x++)
                                {
                                    // Readback vram here...
                                    rb += TTY_ReadCharacter(x, y);
                                }
                            }

                            if (rb == 0)
                            {
                                //rb = 0x10000;

                                if (XTCHECKSUM == 0) len = sprintf(str, "P%u!~0000\\", pid);    // Do not negate result
                                else len = sprintf(str, "P%u!~10000\\", pid);
                            }
                            else
                            {
                                rb -= 1;

                                if (XTCHECKSUM == 0) len = sprintf(str, "P%u!~%04lX\\", pid, rb & 0xFFFF);    // Do not negate result
                                else len = sprintf(str, "P%u!~%04lX\\", pid, (~rb & 0xFFFF));
                            }

                            //NET_SendString(str);
                            NET_SendStringLen(str, len);

                            #ifdef ESC_LOGGING
                            kprintf("[93mDECRQCRA: page: %u -- T:%u, L:%u, B:%u, R:%u -- sum: %lu (negated: $%04lX)[0m", ESC_Param[1], top, left, bottom, right, rb, (~rb & 0x1FFFF));
                            #endif

                            break;
                        }

                        case '#':   // Select checksum extension (XTCHECKSUM)
                        {
                            #ifdef ESC_LOGGING
                            kprintf("[91mNot implemented: Select checksum extension (XTCHECKSUM) - Extension: %u[0m", ESC_Param[1]);
                            #endif

                            /*
                            0  ⇒  do not negate the result.
                            1  ⇒  do not report the VT100 video attributes.
                            2  ⇒  do not omit checksum for blanks.
                            3  ⇒  omit checksum for cells not explicitly initialized.
                            4  ⇒  do not mask cell value to 8 bits or ignore combining characters.
                            */

                            XTCHECKSUM = ESC_Param[1];

                            break;
                        }
                        
                        default:
                        #ifdef ESC_LOGGING
                        kprintf("[91mUnknown y control character: %c[0m", type);
                        #endif
                        break;
                    }

                    goto EndEscape;
                }

                case 'z':   // ... Multiple ...
                {
                    char type = 0;
                    u8 c = 0;
                    u16 param4 = 255;

                    if (ESC_Buffer[0] == '$')   // Default parameters (no parameters given)
                    {
                        type = ESC_Buffer[0];
                    }
                    else if (ESC_Buffer[3] != '\0') 
                    {
                        type = ESC_Buffer[3];

                        param4 += (u8) (ESC_Buffer[0] - '0');
                        c++;
                        param4 *= 10;
                        param4 += (u8) (ESC_Buffer[1] - '0');
                        c++;
                        param4 *= 10;
                        param4 += (u8) (ESC_Buffer[2] - '0');
                    }
                    else if (ESC_Buffer[2] != '\0') 
                    {
                        type = ESC_Buffer[2];

                        param4 += (u8) (ESC_Buffer[0] - '0');
                        c++;
                        param4 *= 10;
                        param4 += (u8) (ESC_Buffer[1] - '0');
                    }
                    else if (ESC_Buffer[1] != '\0') 
                    {
                        type = ESC_Buffer[1];

                        param4 += (u8) (ESC_Buffer[0] - '0');
                    }

                    #ifdef ESC_LOGGING
                    kprintf("[93mz control: %u %u %u %u %u %u %u %u - p4: %u - Type: '%c' - Position: $%lX[0m", ESC_Param16, ESC_Param[1], ESC_Param[2], ESC_Param[3], ESC_Param[4], ESC_Param[5], ESC_Param[6], ESC_Param[7], param4, type, RXBytes);
                    #endif

                    switch (type)
                    {
                        case '\'':   // Enable Locator Reporting (DECELR)
                        {
                            #ifdef ESC_LOGGING
                            kprintf("[91mNot implemented: Enable Locator Reporting (DECELR)[0m");
                            #endif

                            break;
                        }

                        case '$':   // Erase Rectangular Area (DECERA) - "ESC[ Ⓝ ; Ⓝ ; Ⓝ ; Ⓝ $ z"
                        {
                            u8 top    = ESC_Param[0] - 1;
                            u8 left   = ESC_Param[1] - 1;
                            u8 bottom = ESC_Param[2] - 1;
                            u8 right  = param4;

                            top    = (top    == 254 ? 0 : top);
                            left   = (left   == 254 ? 0 : left);
                            bottom = (bottom == 254 ? 0 : bottom);
                            right  = (right  == 255 ? 0 : right);

                            if (vDECOM)
                            {                                
                                top    = (top    < DMarginTop    ? DMarginTop    : top);
                                left   = (left   < DMarginLeft   ? DMarginLeft   : left);
                                bottom = (bottom > C_YMAX      ? C_YMAX      : bottom);
                                right  = (right  > C_XMAX      ? C_XMAX      : right);
                                //bottom = (bottom > DMarginBottom ? DMarginBottom : bottom);
                                //right  = (right  > DMarginRight  ? DMarginRight  : right);
                            }

                            if ((top > bottom) || (left > right))
                            {
                                #ifdef ESC_LOGGING
                                kprintf("DECERA: Skipping erase... T:%u L:%u B:%u R:%u", top, left, bottom, right);
                                #endif

                                break;
                            }

                            s16 tmp_sx = TTY_GetSX();
                            s16 tmp_sy = TTY_GetSY_A();

                            for (u8 y = top; y <= bottom; y++)
                            {
                                TTY_SetSY_A((s16)y);
                                for (u8 x = left; x <= right; x++)
                                {
                                    TTY_SetSX((s16)x);
                                    TTY_PrintChar(' ');
                                }
                            }

                            TTY_SetSX(tmp_sx);
                            TTY_SetSY_A(tmp_sy);

                            #ifdef ESC_LOGGING
                            kprintf("[93mDECERA: Top: %u, Bottom: %u, Left: %u, Right: %u[0m", top, bottom, left, right);
                            #endif

                            break;
                        }
                        
                        default:
                        #ifdef ESC_LOGGING
                        kprintf("[91mUnknown z control character: %c[0m", type);
                        #endif

                        break;
                    }

                    goto EndEscape;
                }

                case '{':   // ... Multiple ...
                {
                    char type = 0;
                    u8 c = 0;
                    u16 param4 = 255;

                    if (ESC_Buffer[0] == '$')   // Default parameters (no parameters given)
                    {
                        type = ESC_Buffer[0];
                    }
                    else if (ESC_Buffer[3] != '\0') 
                    {
                        type = ESC_Buffer[3];

                        param4 += (u8) (ESC_Buffer[0] - '0');
                        c++;
                        param4 *= 10;
                        param4 += (u8) (ESC_Buffer[1] - '0');
                        c++;
                        param4 *= 10;
                        param4 += (u8) (ESC_Buffer[2] - '0');
                    }
                    else if (ESC_Buffer[2] != '\0') 
                    {
                        type = ESC_Buffer[2];

                        param4 += (u8) (ESC_Buffer[0] - '0');
                        c++;
                        param4 *= 10;
                        param4 += (u8) (ESC_Buffer[1] - '0');
                    }
                    else if (ESC_Buffer[1] != '\0') 
                    {
                        type = ESC_Buffer[1];

                        param4 += (u8) (ESC_Buffer[0] - '0');
                    }

                    #ifdef ESC_LOGGING
                    kprintf("[93m{ control: %u %u %u %u %u %u %u %u - p4: %u - Type: '%c' - Position: $%lX[0m", ESC_Param16, ESC_Param[1], ESC_Param[2], ESC_Param[3], ESC_Param[4], ESC_Param[5], ESC_Param[6], ESC_Param[7], param4, type, RXBytes);
                    #endif

                    switch (type)
                    {
                        case '#':   // Save Rendition Attributes - "ESC[ [ Ⓝ ] # {"
                        {
                            #ifdef ESC_LOGGING
                            kprintf("[91mNot implemented: Save Rendition Attributes[0m");
                            #endif

                            break;
                        }

                        case '\'':   // DEC Locator Select Events - "ESC[ [ Ⓝ ] ' {"
                        {
                            #ifdef ESC_LOGGING
                            kprintf("[91mNot implemented: DEC Locator Select Events[0m");
                            #endif

                            break;
                        }

                        case '$':   // Selective erase rectangular area (DECSERA) - "ESC[ Ⓝ ; Ⓝ ; Ⓝ ; Ⓝ $ {"            -- FIXME: THIS IS JUST A COPYPASTA OF "ESC [ Ⓝ ; Ⓝ ; Ⓝ ; Ⓝ $ z"
                        {
                            u8 top    = ESC_Param[0] - 1;
                            u8 left   = ESC_Param[1] - 1;
                            u8 bottom = ESC_Param[2] - 1;
                            u8 right  = param4;

                            top    = (top    == 254 ? 0 : top);
                            left   = (left   == 254 ? 0 : left);
                            bottom = (bottom == 254 ? 0 : bottom);
                            right  = (right  == 255 ? 0 : right);

                            if (vDECOM)
                            {                                
                                top    = (top    < DMarginTop    ? DMarginTop    : top);
                                left   = (left   < DMarginLeft   ? DMarginLeft   : left);
                                bottom = (bottom > C_YMAX      ? C_YMAX      : bottom);
                                right  = (right  > C_XMAX      ? C_XMAX      : right);
                                //bottom = (bottom > DMarginBottom ? DMarginBottom : bottom);
                                //right  = (right  > DMarginRight  ? DMarginRight  : right);
                            }

                            if ((top > bottom) || (left > right))
                            {
                                #ifdef ESC_LOGGING
                                kprintf("DECSERA: Skipping erase... T:%u L:%u B:%u R:%u", top, left, bottom, right);
                                #endif

                                break;
                            }

                            s16 tmp_sx = TTY_GetSX();
                            s16 tmp_sy = TTY_GetSY_A();

                            for (u8 y = top; y <= bottom; y++)
                            {
                                TTY_SetSY_A((s16)y);
                                for (u8 x = left; x <= right; x++)
                                {
                                    TTY_SetSX((s16)x);
                                    TTY_PrintChar(' ');
                                }
                            }

                            TTY_SetSX(tmp_sx);
                            TTY_SetSY_A(tmp_sy);

                            #ifdef ESC_LOGGING
                            kprintf("[93mDECSERA: Top: %u, Bottom: %u, Left: %u, Right: %u[0m", top, bottom, left, right);
                            #endif

                            break;
                        }
                        
                        default:
                        #ifdef ESC_LOGGING
                        kprintf("[91mUnknown { control character: %c[0m", type);
                        #endif

                        break;
                    }

                    goto EndEscape;
                }

                case '_':
                {
                    #ifdef ESC_LOGGING
                    kprintf("[91mUnknown character in escape stream: \"%c\" at $%lX[0m", byte, RXBytes);
                    #endif
                    
                    goto EndEscape; // In case of weird ESC[!_ sequence just end it now...
                }
                
                case ' ':
                {
                    #ifdef ESC_LOGGING
                    kprintf("[91mUnknown character in escape stream: \"%c\" at $%lX[0m", byte, RXBytes);
                    #endif
                    
                    return;
                }

                case '?':
                {
                    ESC_Type = '?';
                    return;
                }

                default:
                    if ((byte >= 65) && (byte <= 122)) 
                    {
                        #ifdef ESC_LOGGING
                        kprintf("[91mUnhandled $%X  -  u8: %u  -  char: '%c'  -  EscType: '%c' (EscSeq: %u  -  Position: $%lX)[0m", byte, byte, (char)byte, (char)ESC_Type, ESC_BufferSeq, RXBytes);
                        #endif
                        goto EndEscape;
                    }
                break;
            }

            ESC_Buffer[ESC_BufferSeq++] = (char)byte;
            return;
        }
    
        case ']':   // Operating System Command (OSC)
        {
            //kprintf("Byte at $%lX: %u (%c)", RXBytes, byte, byte);

            // Todo: Fix this; "]11;#ffffff\\_xyz\\A"
            if (bOSC_GetType)// || byte == 0x1B)
            {
                if (byte == ' ' || byte == ';')
                {
                    OSC_Type = atoi16(ESC_Buffer);

                    ESC_Buffer[0] = '\0';
                    ESC_Buffer[1] = '\0';
                    ESC_Buffer[2] = '\0';
                    ESC_Buffer[3] = '\0';
                    ESC_BufferSeq = 0;
                    
                    #ifdef OSC_LOGGING
                    kprintf("OSC: Got OSC %u at $%lX (OSC type = $%X)", OSC_Type, RXBytes, OSC_Type);
                    #endif

                    bOSC_GetType = FALSE;
                }
                else
                {
                    ESC_Buffer[ESC_BufferSeq++] = byte;
                    return;
                }
            }

            if (bOSC_GetString)
            {
                if (byte == 0x1B)
                {                   
                    #ifdef OSC_LOGGING
                    kprintf("OSC: Skipping <ESC> at $%lX - STRINGPARSE", RXBytes);
                    #endif

                    return;
                }
                else if (byte == '\\' || byte == 7)
                {
                    OSC_String[ESC_OSCSeq] = '\0';   // -1 will be $1B Escape character, remove it

                    #ifdef OSC_LOGGING
                    kprintf("OSC: Got string end $%X at $%lX (OSC_String = \"%s\") - STRINGPARSE", byte, RXBytes, OSC_String);
                    #endif

                    ESC_OSCSeq = 0;
                    bOSC_GetString = FALSE;
                    bOSC_Parse = TRUE;
                }
                else
                {
                    OSC_String[ESC_OSCSeq++] = byte;
                    if (ESC_OSCSeq >= 40) ESC_OSCSeq--; // Dumb cap at 40 characters

                    return;
                }
            }

            // Decide what to do with OSC type before ending OSC sequence
            if (bOSC_Parse)
            {
                #ifdef OSC_LOGGING
                kprintf("OSC PARSE at $%lX", RXBytes);
                #endif

                switch (OSC_Type)
                {
                    case 0: // Set Window Title and Icon Name
                        ChangeTitle(OSC_String);

                        strncpy(FakeWindowLabel, OSC_String, 39);
                        strncpy(FakeIconLabel, OSC_String, 39);

                        #ifdef OSC_LOGGING
                        kprintf("OSC: Changed title/icon to \"%s\" - PARSE", OSC_String);
                        #endif
                    break;

                    case 1: // Change Icon Name
                        strncpy(FakeIconLabel, OSC_String, 39);

                        #ifdef OSC_LOGGING
                        kprintf("OSC: Changed icon to \"%s\" - PARSE", OSC_String);
                        #endif
                    break;

                    case 2: // Change Window title
                        ChangeTitle(OSC_String);

                        strncpy(FakeWindowLabel, OSC_String, 39);

                        #ifdef OSC_LOGGING
                        kprintf("OSC: Changed title to \"%s\" - PARSE", OSC_String);
                        #endif
                    break;

                    case 4: // Change/Read palette color
                        if (OSC_String[strlen(OSC_String) - 1] == '?') //(strcmp(OSC_String, "?"))
                        {
                            char str[16];
                            u16 len = 0;
                            memset(str, 0, 16);
                            
                            len = sprintf(str, "]4;1;rgb:%04X/%04X/%04X\\", 0, 0, 0);   // rgb:%04x/%04x/%04x
                            NET_SendStringLen(str, len);

                            #ifdef OSC_LOGGING
                            kprintf("[91mUnimplemented OSC: Read palette color \"%s\" - PARSE[0m", OSC_String);
                            #endif
                        }
                        else
                        {

                            #ifdef OSC_LOGGING
                            kprintf("[91mUnimplemented OSC: Change palette color \"%s\" - PARSE[0m", OSC_String);
                            #endif
                        }
                    break;

                    case 7:
                    #ifdef OSC_LOGGING
                        kprintf("[91mUnimplemented OSC: $%X at $%lX (Report Current Working Directory) - PARSE[0m", OSC_Type, RXBytes);
                        #endif
                    break;

                    case 10: // Change/Read Special Text Default Foreground Color
                        #ifdef OSC_LOGGING
                        kprintf("[91mUnimplemented OSC: Change/Read Special Text Default Foreground Color: \"%s\" - PARSE[0m", OSC_String);
                        #endif

                        // Parse colour from string here
                        // TTY_SetAttribute(...);
                    break;

                    case 11: // Change/Read Special Text Default Background Color
                        #ifdef OSC_LOGGING
                        kprintf("[91mUnimplemented OSC: Change/Read Special Text Default Background Color: \"%s\" - PARSE[0m", OSC_String);
                        #endif

                        // Parse colour from string here
                        // TTY_SetAttribute(...);

                        //u32 rgb = atoi32(OSC_String+1);
                    break;

                    case 14: // Change/Read Pointer Mask Color
                        #ifdef OSC_LOGGING
                        kprintf("[91mUnimplemented OSC: Change/Read Pointer Mask Color: \"%s\" - PARSE[0m", OSC_String);
                        #endif
                    break;

                    case 92: // String end marker
                        #ifdef OSC_LOGGING
                        kprintf("String end marker - String: \"%s\" - PARSE", OSC_String);
                        #endif
                    break;

                    case 104: // Reset Palette Colors
                        #ifdef OSC_LOGGING
                        kprintf("[91mUnimplemented OSC: Reset Palette Colors - String: \"%s\" - PARSE[0m", OSC_String);
                        #endif
                    break;

                    default:
                        #ifdef OSC_LOGGING
                        kprintf("[91mUnknown OSC: $%X at $%lX - PARSE[0m", OSC_Type, RXBytes);
                        #endif
                    break;
                }

                #ifdef OSC_LOGGING
                kprintf("Ending OSC at $%lX", RXBytes);
                #endif

                ESC_OSCBuffer[0] = '\0';
                ESC_OSCBuffer[1] = '\0';
                ESC_OSCSeq = 0;
                memset(OSC_String, 0, 40);
                bOSC_Parse = FALSE;
                bOSC_GetType = TRUE;
                goto EndEscape;
            }

            // Determine what to do with the following incomming bytes depending on what type of OSC was received
            switch (OSC_Type)
            {
                case 0:   // Change Window title and Icon
                case 2:   // Change Window title
                    #ifdef OSC_LOGGING
                    kprintf("OSC: Change Window title at $%lX (OSC type = $%X) - TYPE", RXBytes, OSC_Type);
                    #endif

                    ESC_OSCSeq = 0;
                    bOSC_GetString = TRUE;
                break;

                case 4:   // Change/Read palette color
                    #ifdef OSC_LOGGING
                    kprintf("OSC: Change/Read palette color at $%lX (OSC type = $%X) - TYPE", RXBytes, OSC_Type);
                    #endif

                    ESC_OSCSeq = 0;
                    bOSC_GetString = TRUE;
                break;

                case 7:
                    #ifdef OSC_LOGGING
                    kprintf("OSC: Got end marker $7 at $%lX (OSC_String = \"%s\") - TYPE", RXBytes, OSC_String);
                    #endif

                    ESC_OSCSeq = 0;
                    bOSC_GetString = FALSE;
                    bOSC_Parse = TRUE;
                break;

                case 10:  // Change/Read Special Text Default Background Color
                    #ifdef OSC_LOGGING
                    kprintf("OSC: Change/Read Special Text Default Foreground Color at $%lX (OSC type = $%X) - TYPE", RXBytes, OSC_Type);
                    #endif

                    ESC_OSCSeq = 0;
                    bOSC_GetString = TRUE;
                break;
                
                case 11:  // Change/Read Special Text Default Background Color
                    #ifdef OSC_LOGGING
                    kprintf("OSC: Change/Read Special Text Default Background Color at $%lX (OSC type = $%X) - TYPE", RXBytes, OSC_Type);
                    #endif

                    ESC_OSCSeq = 0;
                    bOSC_GetString = TRUE;
                break;

                case '\\':  // Hack for String Terminator (Remove escape $1B from OSC_String!)
                    OSC_String[ESC_OSCSeq-1] = '\0';   // -1 will be $1B Escape character, remove it

                    #ifdef OSC_LOGGING
                    kprintf("OSC: Got end marker $1B $5C at $%lX (OSC_String = \"%s\") - TYPE", RXBytes, OSC_String);
                    #endif

                    ESC_OSCSeq = 0;
                    bOSC_GetString = FALSE;
                    bOSC_Parse = TRUE;
                break;
                
                case 104:  // Reset Palette Colors
                    #ifdef OSC_LOGGING
                    kprintf("OSC: Reset Palette Colors at $%lX (OSC type = $%X) - TYPE", RXBytes, OSC_Type);
                    #endif

                    ESC_OSCSeq = 0;
                    bOSC_GetString = TRUE;
                break;
            
                default:
                {
                    #ifdef OSC_LOGGING
                    kprintf("[91mUnknown OSC: $%X at $%lX - TYPE[0m", OSC_Type, RXBytes);
                    #endif                
                    
                    ESC_OSCBuffer[0] = '\0';
                    ESC_OSCBuffer[1] = '\0';
                    ESC_OSCSeq = 0;
                    memset(OSC_String, 0, 40);
                    bOSC_Parse = FALSE;
                    bOSC_GetType = TRUE;
                    goto EndEscape;
                }
            }

            return;
        }

        case '_':   // TEMP
        {
            #ifdef ESC_LOGGING
            kprintf("[91mGot a stray \"ESC _\" ... Which is probably apart of a previous OSC. At $%lX[0m", RXBytes);
            #endif

            goto EndEscape;
        }

        case '(':   // G0 charset
        {
            switch (byte)
            {
                case '0':   // DEC Special Character and Line Drawing Set
                {
                    CharMapSelection = 1;
                    #if ESC_LOGGING >= 3
                    kprintf("ESC%c0: DEC Special Character and Line Drawing Set", ESC_Type);
                    #endif
                    break;
                }
                
                case 'B':
                {
                    CharMapSelection = 0;
                    #if ESC_LOGGING >= 4
                    kprintf("ESC%cB: United States (USASCII), VT100", ESC_Type);
                    #endif
                    break;
                }

                default:
                {
                    #ifdef ESC_LOGGING
                    kprintf("[91mUnimplemented G0 charset $%X ESC ( %c at $%lX[0m", byte, (char)byte, RXBytes);
                    #endif
                }
            }

            goto EndEscape;
        }

        case ')':   // G1 charset
        {
            switch (byte)
            {
                case '0':   // ...
                {
                    CharMapSelection = 2;
                    #if ESC_LOGGING >= 2
                    kprintf("ESC%c0: ...", ESC_Type);
                    #endif
                    break;
                }

                default:
                {
                    #ifdef ESC_LOGGING
                    kprintf("[91mUnimplemented G1 charset $%X ESC ) %c at $%lX[0m", byte, (char)byte, RXBytes);
                    #endif
                }
            }

            goto EndEscape;
        }

        case '?':   // Mode set
        {
            if (byte == 'h')
            {
                if (ESC_QSeqMulti)
                {
                    #ifdef ESC_LOGGING
                    kprintf("[91mUnimplemented multi mode \"%s\" at $%lX[0m", (char*)ESC_QBuffer, RXBytes);
                    #endif
                    goto EndEscape;
                }
                else QSeqNumber = atoi16((char*)ESC_QBuffer);
                //kprintf("QSeqNumber = %u", QSeqNumber);

                switch (QSeqNumber)
                {
                    case 1:     // Switched Cursor Key Format (DECCKM)
                        vDECCKM = TRUE;
                    break;

                    case 6:     // Origin Mode (DECOM), VT100.
                        vDECOM = TRUE;
                    break;
                
                    case 7:     // Auto-Wrap Mode (DECAWM), VT100.
                        sv_bWrapAround = TRUE;
                    break;
                
                    case 9:     // Mouse Click-Only Tracking (X10_MOUSE)
                    break;

                    case 12:    // Cursor Blinking ON (ATT610_BLINK)
                        bDoCursorBlink = TRUE;
                    break;
                
                    case 25:    // Shows the cursor, from the VT220. (DECTCEM)
                        SetSprite_TILE(SPRITE_ID_CURSOR, LastCursor);
                    break;

                    case 69:    // DECSLRM can set margins.
                        vDECLRMM = TRUE;
                    break;

                    case 1004:  // Report Focus Change
                        // When the terminal gains focus emit:  ESC [ I 
                        // When the terminal looses focus emit: ESC [ O
                        // vte: Sends current focus state on mode activation.

                        if (!WinMgr_isWindowOpen() && !vMinimized) NET_SendString("[I");
                        else NET_SendString("[O");
                    break;

                    case 1000:  // Mouse Down+Up Tracking
                        MTrackMode = MT_DownUp;
                    break;
                    
                    case 1001:  // Mouse Highlight Mode
                        MTrackMode = MT_HighLight;
                    break;
                    
                    case 1002:  // Mouse Click and Dragging Tracking
                        MTrackMode = MT_ClickDrag;
                    break;

                    case 1003:  // Mouse Tracking with Movement
                        MTrackMode = MT_Movement;
                    break;

                    case 1005:  // Mouse Report Format multibyte
                        MReportFormat = MR_Multibyte;
                    break;

                    case 1006:  // Mouse Reporting Format Digits
                        MReportFormat = MR_Digits;
                    break;

                    case 1015:  // Mouse Reporting Format URXVT
                        MReportFormat = MR_URXVT;
                        #ifdef ESC_LOGGING
                        kprintf("[93mAttempt at activating mouse related mode: %uh (Not implemented)[0m", QSeqNumber);
                        #endif
                    break;

                    case 47:    // Alternate Screen Buffer (ALTBUF)
                    case 1047:
                    case 1049:
                        if (BufferSelect == 0)
                        {
                            // Set HScroll to alternate buffer ->
                            if (!sv_Font)
                            {
                                VDP_setHorizontalScroll(BG_A, HScroll-320);
                                VDP_setHorizontalScroll(BG_B, HScroll-320);

                                BufferSelect = 40;
                            }
                            else
                            {
                                VDP_setHorizontalScroll(BG_A, (HScroll+4-320));
                                VDP_setHorizontalScroll(BG_B, (HScroll-320));

                                BufferSelect = 80;
                            }

                            // Set VScroll to 0
                            Saved_VScroll = VScroll;
                            TTY_ResetVScroll();

                            switch (QSeqNumber)
                            {
                                case 1049:                                
                                    // Save cursor position from main buffer
                                    Saved_sx[0] = TTY_GetSX();
                                    Saved_sy[0] = TTY_GetSY();

                                    // Restore cursor position to alternate buffer
                                    TTY_SetSX(Saved_sx[1]);
                                    TTY_SetSY(Saved_sy[1]);

                                    // Clear alternate buffer
                                    VDP_clearTileMapRect(BG_A, 40, 0, 40, 30);
                                    VDP_clearTileMapRect(BG_B, 40, 0, 40, 30);
                                break;
                            
                                default:
                                break;
                            }

                            #ifdef ESC_LOGGING
                            kprintf("Alternative screen buffer ON (%uh)", QSeqNumber);
                            #endif
                        }
                    break;

                    case 2004:  // Turn on bracketed paste mode. 
                    /*
                    Bracket clipboard paste contents in delimiter sequences.
                    When pasting from the (e.g. system) clipboard add ESC[200~ before the clipboard contents and ESC[201~ after the clipboard contents. This allows applications to distinguish clipboard contents from manually typed text.
                    */
                    vBracketedPaste = TRUE;
                    break;

                    default:
                    #ifdef ESC_LOGGING
                    kprintf("[91mUnimplemented mode ?%uh at $%lX", QSeqNumber, RXBytes);
                    kprintf("0=%c 1= %c - 2= %c - 3= %c", ESC_QBuffer[0], ESC_QBuffer[1], ESC_QBuffer[2], ESC_QBuffer[3]);
                    kprintf("0=%c%c - 2= %c%c", ESC_QBuffer[0], ESC_QBuffer[1], ESC_QBuffer[3], ESC_QBuffer[4]);
                    kprintf("[0m");
                    #endif
                    break;
                }

                goto EndEscape;
            }

            if (byte == 'l')
            {
                if (ESC_QSeqMulti)
                {
                    #ifdef ESC_LOGGING
                    kprintf("[91mUnimplemented multi mode \"%s\" at $%lX[0m", (char*)ESC_QBuffer, RXBytes);
                    #endif
                    goto EndEscape;
                }
                else QSeqNumber = atoi16((char*)ESC_QBuffer);
                //kprintf("QSeqNumber = %u", QSeqNumber);

                switch (QSeqNumber)
                {
                    case 1:     // Normal Cursor Key Format (DECCKM)
                        vDECCKM = FALSE;
                    break;

                    case 6:     // Normal Cursor Mode (DECOM), VT100.
                        vDECOM = FALSE;
                    break;
                
                    case 7:     // No Auto-Wrap Mode (DECAWM), VT100.
                        sv_bWrapAround = FALSE;
                    break;

                    case 12:    // Cursor Blinking OFF (ATT610_BLINK)
                        bDoCursorBlink = FALSE;
                        
                        if (sv_Font) LastCursor = 0x13;
                        else         LastCursor = 0x10;

                        SetSprite_TILE(SPRITE_ID_CURSOR, LastCursor);
                    break;
                
                    case 25:    // Hides the cursor. (DECTCEM)
                        SetSprite_TILE(SPRITE_ID_CURSOR, 0x16);
                    break;

                    case 69:    // DECSLRM cannot set margins.
                        vDECLRMM = FALSE;
                    break;

                    case 1004:  // Report Focus Change
                        // When the terminal gains focus emit:  ESC [ I 
                        // When the terminal looses focus emit: ESC [ O
                        // vte: Sends current focus state on mode activation.
                    break;

                    case 9:     // Mouse Click-Only Tracking (X10_MOUSE)
                    case 1000:  // Mouse Down+Up Tracking
                    case 1002:  // Mouse Click and Dragging Tracking
                    case 1003:  // Mouse Tracking with Movement
                    case 1005:  // Mouse Report Format multibyte
                    case 1006:  // Mouse Reporting Format Digits
                    case 1015:  // Mouse Reporting Format URXVT
                        #ifdef ESC_LOGGING
                        kprintf("[93mAttempt at deactivating mouse related mode: %ul (Not implemented)[0m", QSeqNumber);
                        #endif
                    break;

                    case 47:    // Alternate Screen Buffer (ALTBUF)
                    case 1047:
                    case 1049:
                        if (BufferSelect != 0)
                        {
                            BufferSelect = 0;

                            // Set HScroll to main buffer <-
                            if (!sv_Font)
                            {
                                VDP_setHorizontalScroll(BG_A, HScroll);
                                VDP_setHorizontalScroll(BG_B, HScroll);
                            }
                            else
                            {
                                VDP_setHorizontalScroll(BG_A, (HScroll+4));
                                VDP_setHorizontalScroll(BG_B, (HScroll  ));
                            }

                            // Set VScroll to main buffer vscroll
                            TTY_SetVScrollAbs(Saved_VScroll);

                            switch (QSeqNumber)
                            {
                                case 1047:
                                    // Clear alternate buffer
                                    VDP_clearTileMapRect(BG_A, 40, 0, 40, 30);
                                    VDP_clearTileMapRect(BG_B, 40, 0, 40, 30);
                                break;

                                case 1049:
                                    // Save cursor position from alternate buffer
                                    Saved_sx[1] = TTY_GetSX();
                                    Saved_sy[1] = TTY_GetSY();

                                    // Restore cursor position to main buffer
                                    TTY_SetSX(Saved_sx[0]);
                                    TTY_SetSY(Saved_sy[0]);
                                break;
                            
                                default:
                                break;
                            }

                            #ifdef ESC_LOGGING
                            kprintf("Alternative screen buffer OFF (%ul)", QSeqNumber);
                            #endif
                        }
                    break;

                    case 2004:  // Turn off bracketed paste mode. 
                        vBracketedPaste = FALSE;
                    break;

                    default:
                    #ifdef ESC_LOGGING
                    kprintf("[91mUnimplemented mode ?%ul at $%lX[0m", QSeqNumber, RXBytes);
                    #endif
                    break;
                }

                goto EndEscape;
            }

            if (byte == 's')
            {
                QSeqNumber = atoi16((char*)ESC_QBuffer);

                #ifdef ESC_LOGGING
                kprintf("[91mUnknown mode set and end character ?%u%c at $%lX[0m", QSeqNumber, byte, RXBytes);
                #endif

                goto EndEscape;
            }

            if (byte == 'r')
            {
                QSeqNumber = atoi16((char*)ESC_QBuffer);

                #ifdef ESC_LOGGING
                kprintf("[91mUnknown mode set and end character ?%u%c at $%lX[0m", QSeqNumber, byte, RXBytes);
                #endif

                goto EndEscape;
            }
            
            if (byte == ';') ESC_QSeqMulti = ESC_QSeq;
            else 
            {
                ESC_QBuffer[ESC_QSeq] = byte;
                //kprintf("ESC_Q: $%X - '%c'", ESC_QBuffer[ESC_QSeq-1], (char)ESC_QBuffer[ESC_QSeq-1]);

                if (ESC_QSeq > 5) 
                {
                    //for (u8 i = 0; i < 6; i++) kprintf("ESC_QBuffer[%u] = $%X (%c)", i, ESC_QBuffer[i], (char)ESC_QBuffer[i]);
                    goto EndEscape;
                }

                ESC_QSeq++;
            }

            return;
        }

        case ' ':   // ??? Escape
        {           
             switch (byte)
            {
                case 'G':   // Use 8-bit controls (S8C1T) - "ESC ␣ G"
                {
                    #if ESC_LOGGING >= 2
                    kprintf("[91mNot implemented: S8C1T -  Position: $%lX[0m", RXBytes);
                    #endif
                    goto EndEscape;
                }

                case 'F':   // Use 7-bit controls (S7C1T) - "ESC ␣ F"
                {
                    #if ESC_LOGGING >= 2
                    kprintf("[91mNot implemented: S8C1T -  Position: $%lX[0m", RXBytes);
                    #endif
                    goto EndEscape;
                }
                   
                default:
                {
                    #ifdef ESC_LOGGING
                    kprintf("[91mUnhandled $%X  -  u8: %u  -  char: '%c'  -  EscType: ␣ (EscSeq: %u  -  Position: $%lX)[0m", byte, byte, (char)byte, ESC_BufferSeq, RXBytes);
                    #endif
                    goto EndEscape;
                }
            }
            return;
        }
        
        default:
            if (ESC_Seq == 1)
            {
                ESC_Type = byte;

                switch (ESC_Type)
                {        
                    case '=':   // ESC =    Application Keypad (DECKPAM).
                        #ifdef ESC_LOGGING
                        kprintf("[91m\"ESC =\" NOT IMPLEMENTED! At $%lX[0m", RXBytes);
                        #endif

                        goto EndEscape;
                    break;

                    case '>':   // ESC >    Normal Keypad (DECKPNM), VT100.
                        #ifdef ESC_LOGGING
                        kprintf("[91m\"ESC >\" NOT IMPLEMENTED! At $%lX[0m", RXBytes);
                        #endif

                        goto EndEscape;
                    break;

                    case 'D':   // ESC D    Index (IND) - Cursor down - at bottom of region, scroll up
                    {
                        #if ESC_LOGGING >= 2
                        kprintf("ESC D - Cursor down (+scroll?)");
                        #endif

                        // Uhh... this needs testing because this just looks wrong
                        if (TTY_GetSY_A() >= DMarginBottom) TTY_DrawScrollback(1);
                        else
                        {
                            if (vNewlineConv == 1) TTY_SetSX(0);  // Convert \n to \n\r

                            TTY_MoveCursor(TTY_CURSOR_DOWN, 1);
                        }
                        
                        goto EndEscape;
                    }

                    case 'M':   // ESC M    Reverse Index (RI) https://terminalguide.namepad.de/seq/a_esc_cm/  (Old note: Moves cursor one line up, scrolling if needed)
                        #ifdef ESC_LOGGING
                        kprintf("[93m\"ESC M\" Reverse Index NOT IMPLEMENTED! At $%lX[0m", RXBytes);
                        #endif

                        if (TTY_GetSY_A() > DMarginTop) TTY_MoveCursor(TTY_CURSOR_UP, 1);
                        else
                        {
                            TTY_DrawScrollback_RI(1);
                        }

                        goto EndEscape;
                    break;

                    case 'E':   // ESC E    Next line (same as CR LF)
                    {
                        #if ESC_LOGGING >= 2
                        kprintf("ESC E");
                        #endif

                        TTY_SetSX(0);
                        TTY_MoveCursor(TTY_CURSOR_DOWN, 1);
                        goto EndEscape;
                    }

                    case 'H':   // ESC H    Horizontal Tab Set (HTS)
                    {
                        u8 c = (TTY_GetSX() % 80);
                        HTS_Column[c] = 1;

                        #if ESC_LOGGING >= 4
                        kprintf("[93mSetting column %u as tabstop[0m", c);
                        #endif

                        goto EndEscape;
                    }
                    
                    case 'N':   // ESC N    Select G2 set for next character only
                    {
                        #ifdef ESC_LOGGING
                        kprintf("[91m\"ESC N\" Select G2 set for next character only NOT IMPLEMENTED! At $%lX[0m", RXBytes);
                        #endif

                        goto EndEscape;
                    }

                    case 'O':   // ESC O    Select G2 set for next character only
                    {
                        #ifdef ESC_LOGGING
                        kprintf("[91m\"ESC O\" Select G3 set for next character only NOT IMPLEMENTED! At $%lX[0m", RXBytes);
                        #endif

                        goto EndEscape;
                    }

                    case 'P':   // ESC P    Device Control String
                    {
                        #ifdef ESC_LOGGING
                        kprintf("[91m\"ESC P\" Device Control String NOT IMPLEMENTED! At $%lX[0m", RXBytes);
                        #endif

                        goto EndEscape;
                    }

                    case 'c':   // RIS: Reset to initial state - Resets the device to its state after being powered on. 
                    {
                        TTY_Init(TF_ClearScreen);
                        goto EndEscape;
                    }
                    
                    case '\\':  // ESC \    String Terminator
                    {
                        #ifdef ESC_LOGGING
                        kprintf("[93m\"ESC \\\" String Terminator At $%lX[0m", RXBytes);
                        #endif

                        ESC_OSCBuffer[0] = '\0';
                        ESC_OSCBuffer[1] = '\0';
                        ESC_OSCSeq = 0;
                        memset(OSC_String, 0, 40);
                        bOSC_Parse = FALSE;
                        bOSC_GetType = TRUE;

                        goto EndEscape;
                    }

                    case '7':   // Save Cursor (DECSC) (ESC 7)
                    {
                        #if ESC_LOGGING >= 2
                        kprintf("ESC 7");
                        #endif

                        u8 buffer = BufferSelect == 80 ? 1 : 0;

                        Saved_sx[buffer] = TTY_GetSX();
                        Saved_sy[buffer] = TTY_GetSY();

                        Saved_OrgTop[buffer] = DMarginTop;
                        Saved_OrgBottom[buffer] = DMarginBottom;

                        goto EndEscape;
                    }

                    case '8':   // Restore Cursor (DECRC) (ESC 8)
                    {
                        #if ESC_LOGGING >= 2
                        kprintf("ESC 8");
                        #endif

                        u8 buffer = BufferSelect == 80 ? 1 : 0;

                        TTY_SetSX(Saved_sx[buffer]);
                        TTY_SetSY(Saved_sy[buffer]);
    
                        DMarginTop = Saved_OrgTop[buffer];
                        DMarginBottom = Saved_OrgBottom[buffer];

                        goto EndEscape;
                    }
                
                    default:    // By default skip this round and parse it later
                        #if ESC_LOGGING >= 2
                        if ((ESC_Type != '[') && (ESC_Type != ']') && (ESC_Type != '(') && (ESC_Type != ')')) kprintf("[91;5mSkipping: ESC %c ($%X) at $%lX[0m", ESC_Type, ESC_Type, RXBytes);
                        #endif
                    break;
                }

                return;
            }
        break;
    }

    return;

    EndEscape:
    {
        NextByte = NC_Data;
        ESC_Seq = 0;
        ESC_Type = 0;
        
        ESC_Param[0] = 0xFF;
        ESC_Param[1] = 0xFF;
        ESC_Param[2] = 0xFF;
        ESC_Param[3] = 0xFF;
        ESC_Param[4] = 0xFF;
        ESC_Param[5] = 0xFF;
        ESC_Param[6] = 0xFF;
        ESC_Param[7] = 0xFF;
        ESC_Param[8] = 0xFF;
        ESC_Param[9] = 0xFF;
        ESC_Param16 = 0xFFFF;
        ESC_ParamSeq = 0;

        ESC_Buffer[0] = '\0';
        ESC_Buffer[1] = '\0';
        ESC_Buffer[2] = '\0';
        ESC_Buffer[3] = '\0';
        ESC_BufferSeq = 0;

        ESC_QSeq = 0;
        ESC_QSeqMulti = 0;
        ESC_QBuffer[0] = '\0';
        ESC_QBuffer[1] = '\0';
        ESC_QBuffer[2] = '\0';
        ESC_QBuffer[3] = '\0';
        ESC_QBuffer[4] = '\0';
        ESC_QBuffer[5] = '\0';

        return;
    }
}

static inline void IAC_SuggestNAWS()
{
    NET_SendChar(TC_IAC);
    NET_SendChar(TC_WILL);
    NET_SendChar(TO_NAWS);

    //IAC_NAWS_PENDING = TRUE;
}

static inline void IAC_SuggestEcho(u8 enable)
{
    NET_SendChar(TC_IAC);
    NET_SendChar((enable?TC_DO:TC_DONT));
    NET_SendChar(TO_ECHO);
}

static inline void IAC_SuggestTermSpeed()
{
    NET_SendChar(TC_IAC);
    NET_SendChar(TC_DO);
    NET_SendChar(TO_TERM_SPEED);
}

static void DoIAC(u8 byte)
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

    if ((IAC_Command == TC_GA) || (IAC_Command == TC_NOP) || (IAC_Command == TC_EOR) || (IAC_Command == TC_DM)) goto skipArg;

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
        kprintf("IAC_Seq < 2 - Returning without resetting IAC (Byte = $%X)", byte);
        #endif
        return;
    }

    skipArg:

    switch (IAC_Command)
    {
        case TC_EOR:
        {
            #ifdef IAC_LOGGING
            kprintf("IAC: Got End-Of-Record (Treating as NOP)");
            #endif
            break;
        }

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
                        NET_SendChar(TC_IAC);
                        NET_SendChar(TC_SB);
                        NET_SendChar(TO_TERM_TYPE);
                        NET_SendChar(TS_IS);
                        NET_SendString(TermTypeList[sv_TermType]);
                        NET_SendChar(TC_IAC);
                        NET_SendChar(TC_SE);
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
                            NET_SendChar(TC_IAC);
                            NET_SendChar(TC_SB);
                            NET_SendChar(TO_TERM_SPEED);
                            NET_SendChar(TS_IS);
                            NET_SendString(RL_REPORT_BAUD);
                            NET_SendChar(',');
                            NET_SendString(RL_REPORT_BAUD);
                            NET_SendChar(TC_IAC);
                            NET_SendChar(TC_SE);

                            #ifdef IAC_LOGGING
                            kprintf("Response: IAC SB TERMINAL-SPEED IS %s,%s IAC SE", RL_REPORT_BAUD, RL_REPORT_BAUD);
                            #endif
                        }
                        else
                        {
                            // IAC SB TERMINAL-SPEED IS ... IAC SE
                            NET_SendChar(TC_IAC);
                            NET_SendChar(TC_SB);
                            NET_SendChar(TO_TERM_SPEED);
                            NET_SendChar(TS_IS);
                            NET_SendString(sv_Baud);
                            NET_SendChar(',');
                            NET_SendString(sv_Baud);
                            NET_SendChar(TC_IAC);
                            NET_SendChar(TC_SE);

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

                                NET_SendChar(TC_IAC);
                                NET_SendChar(TC_SB);
                                NET_SendChar(TO_LINEMODE);
                                NET_SendChar(LM_MODE);
                                NET_SendChar((vLineMode | LMSM_MODEACK));
                                NET_SendChar(TC_IAC);
                                NET_SendChar(TC_SE);

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
                        kprintf("[91mError: Unhandled Linemode. case (IAC_SubNegotiationBytes[0] = $%X)[0m", IAC_SubNegotiationBytes[0]);
                        #endif
                        break;
                    }
                    break;
                }

                case TO_ENV:
                {
                    if (IAC_SubNegotiationBytes[0] == TS_IS)
                    {
                        #ifdef IAC_LOGGING
                        kprintf("Server: IAC SB ENVIRON IS type ... [ VALUE ... ] [ type ... [ VALUE ... ] [");
                        kprintf("Response: IAC SB ... IAC SE");
                        #endif
                    }
                    else if (IAC_SubNegotiationBytes[0] == TS_SEND)
                    {
                        /*NET_SendChar(TC_IAC);
                        NET_SendChar(TC_SB);
                        NET_SendChar(TO_ENV);
                        NET_SendChar(TS_IS);
                        NET_SendChar(0);
                        NET_SendChar(TC_IAC);
                        NET_SendChar(TC_SE);*/

                        #ifdef IAC_LOGGING
                        kprintf("Server: IAC SB ENVIRON SEND [ type ... [ type ... [ ... ] ] ] IAC SE");
                        kprintf("Response: IAC SB ENVIRON IS ... IAC SE");
                        #endif
                    }
                    else if (IAC_SubNegotiationBytes[0] == TS_INFO)
                    {
                        #ifdef IAC_LOGGING
                        kprintf("Server: IAC SB ENVIRON INFO type ... [ VALUE ... ] [ type ... [ VALUE ... ] [");
                        kprintf("Response: IAC SB ... IAC SE");
                        #endif
                    }

                    break;
                }

                case TO_LOGOUT:
                {
                    #ifdef IAC_LOGGING
                    kprintf("Server: IAC SB LOGOUT xyz IAC SE");
                    kprintf("[91mResponse: NONE - NOT IMPLEMENTED![0m");
                    #endif

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

        case TC_NOP:
        {
            #ifdef IAC_LOGGING
            kprintf("IAC: Got NOP");
            #endif
            break;
        }

        case TC_GA:
        {
            if (vDoGA)
            {

            }
            // else if not vDoGA then treat this command as a NOP.

            #ifdef IAC_LOGGING
            kprintf("IAC: Got Go-Ahead - vDoGA = %s", vDoGA?"TRUE":"FALSE (Treating as NOP)");
            #endif
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
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_DONT);
                    NET_SendChar(TO_BIN_TRANS);
                    
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

                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_WILL);
                    NET_SendChar(TO_SUPPRESS_GO_AHEAD);

                    #ifdef IAC_LOGGING
                    kprintf("Server: IAC WILL SUPPRESS_GO_AHEAD - Client response: IAC WILL SUPPRESS_GO_AHEAD");
                    #endif
                break;

                // https://datatracker.ietf.org/doc/html/rfc859
                case TO_STATUS:
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_DONT);
                    NET_SendChar(TO_STATUS);

                    #ifdef IAC_LOGGING
                    kprintf("Server: IAC WILL STATUS - Client response: IAC DONT STATUS");
                    #endif
                break;

                case TO_END_REC:
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_DONT);
                    NET_SendChar(TO_END_REC);

                    #ifdef IAC_LOGGING
                    kprintf("Server: IAC WILL END_REC - Client response: IAC DONT END_REC");
                    #endif
                break;
                
                default:
                    #ifdef IAC_LOGGING
                    kprintf("[91mError: Unhandled TC_WILL option case (IAC_Option = $%X)[0m", IAC_Option);
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
                    kprintf("[91mError: Unhandled TC_WONT option case (IAC_Option = $%X)[0m", IAC_Option);
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
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_WONT);
                    NET_SendChar(TO_BIN_TRANS);
                    
                    #ifdef IAC_LOGGING
                    kprintf("Server: IAC DO TRANSMIT_BINARY - Response: IAC WONT TRANSMIT_BINARY - FULL IMPL. TODO");
                    #endif
                    break;
                }

                case TO_ECHO:
                {        
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_WILL);
                    NET_SendChar(TO_ECHO);
                    vDoEcho = 1;
                    #ifdef IAC_LOGGING
                    kprintf("Response: IAC WILL ECHO");
                    #endif
                    break;
                }

                // Not sure if this is the proper response?
                case TO_SUPPRESS_GO_AHEAD:
                {
                    vDoGA = 0;

                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_WILL);
                    NET_SendChar(TO_SUPPRESS_GO_AHEAD);

                    #ifdef IAC_LOGGING
                    kprintf("Response: IAC WILL SUPPRESS_GO_AHEAD");
                    #endif
                    break;
                }

                case TO_TERM_TYPE:
                {            
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_WILL);
                    NET_SendChar(TO_TERM_TYPE);

                    #ifdef IAC_LOGGING
                    kprintf("Response: IAC WILL TERM_TYPE");
                    #endif
                    break;
                }
                
                case TO_NAWS:
                {
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_WILL);
                    NET_SendChar(TO_NAWS);
                    
                    // IAC SB NAWS <16-bit value> <16-bit value> IAC SE
                    // Sent by the Telnet client to inform the Telnet server of the window width and height.
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_SB);
                    NET_SendChar(TO_NAWS);
                    NET_SendChar(0);
                    NET_SendChar(C_XMAX);//(sv_Font==FONT_8x8_16?40:80)); // Columns
                    NET_SendChar(0);
                    NET_SendChar(C_YMAX);//(bPALSystem?29:27)); // Rows
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_SE);

                    #ifdef IAC_LOGGING
                    kprintf("Response: IAC WILL NAWS - IAC SB 0x%04X 0x%04X IAC SE", C_XMAX, C_YMAX);//(sv_Font==FONT_8x8_16?40:80), (bPALSystem?29:27));
                    #endif
                    break;
                }

                case TO_TERM_SPEED:
                {            
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_WILL);
                    NET_SendChar(TO_TERM_SPEED);

                    #ifdef IAC_LOGGING
                    kprintf("Response: IAC WILL TERM_SPEED");
                    #endif
                    break;
                }

                // https://datatracker.ietf.org/doc/html/rfc1080
                case TO_RFLOW_CTRL:
                {            
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_WONT);
                    NET_SendChar(TO_RFLOW_CTRL);

                    #ifdef IAC_LOGGING
                    kprintf("Response: IAC WONT RFLOW_CTRL");
                    #endif
                    break;
                }

                // https://datatracker.ietf.org/doc/html/rfc779
                case TO_SEND_LOCATION:
                {            
                    /*NET_SendChar(TC_IAC);
                    NET_SendChar(TC_WILL);
                    NET_SendChar(TO_SEND_LOCATION);*/

                    // IAC SB SEND-LOCATION <location> IAC SE
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_SB);
                    NET_SendChar(TO_SEND_LOCATION);
                    NET_SendString("MegaDriveLand");
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_SE);

                    #ifdef IAC_LOGGING
                    kprintf("Response: IAC SB SEND-LOCATION <location> IAC SE");
                    #endif                    
                    break;
                }

                case TO_LINEMODE:
                {
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_WILL);
                    NET_SendChar(TO_LINEMODE);
                    
                    #ifdef IAC_LOGGING
                    kprintf("Response: IAC WILL LINEMODE");
                    #endif

                    break;
                }

                // rfc1408
                case TO_ENV:
                {
                    if (sv_AllowRemoteEnv)
                    {
                        NET_SendChar(TC_IAC);
                        NET_SendChar(TC_WILL);
                        NET_SendChar(TO_ENV);
                        
                        #ifdef IAC_LOGGING
                        kprintf("Response: IAC WILL ENV");
                        #endif
                    }
                    else
                    {
                        NET_SendChar(TC_IAC);
                        NET_SendChar(TC_WONT);
                        NET_SendChar(TO_ENV);
                        
                        #ifdef IAC_LOGGING
                        kprintf("Response: IAC WONT ENV");
                        #endif
                    }

                    break;
                }      

                // https://datatracker.ietf.org/doc/html/rfc1572
                case TO_ENV_OP:
                {
                    if (sv_AllowRemoteEnv)
                    {
                        NET_SendChar(TC_IAC);
                        NET_SendChar(TC_WILL);
                        NET_SendChar(TO_ENV_OP);
                        
                        #ifdef IAC_LOGGING
                        kprintf("Response: IAC WILL ENV_OP");
                        #endif
                    }
                    else
                    {
                        NET_SendChar(TC_IAC);
                        NET_SendChar(TC_WONT);
                        NET_SendChar(TO_ENV_OP);
                        
                        #ifdef IAC_LOGGING
                        kprintf("Response: IAC WONT ENV_OP");
                        #endif
                    }

                    break;
                }

                case TO_XDISP:
                {
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_WONT);
                    NET_SendChar(TO_XDISP);
                    
                    #ifdef IAC_LOGGING
                    kprintf("Response: IAC WONT XDISP");
                    #endif

                    break;
                }
                
                default:
                    #ifdef IAC_LOGGING
                    kprintf("[91mError: Unhandled TC_DO option case (IAC_Option = $%X)[0m", IAC_Option);
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
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_WONT);
                    NET_SendChar(TO_ECHO);
                    vDoEcho = 0;

                    #ifdef IAC_LOGGING
                    kprintf("Response: IAC WONT ECHO");
                    #endif
                break;

                case TO_BIN_TRANS:
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_WONT);
                    NET_SendChar(TO_BIN_TRANS);
                    
                    #ifdef IAC_LOGGING
                    kprintf("Response: IAC WONT BIN_TRANS");
                    #endif
                break;
                
                default:
                    #ifdef IAC_LOGGING
                    kprintf("[91mError: Unhandled TC_DONT option case (IAC_Option = $%X)[0m", IAC_Option);
                    #endif
                break;
            }

            break;
        }

        case TC_DM:
        {
            #ifdef IAC_LOGGING
            kprintf("[93mGot IAC TC_DM[0m");
            #endif
            break;
        }
    
        default:
            #ifdef IAC_LOGGING
            kprintf("[91mError: Unhandled command case (IAC_Command = $%X -- IAC_Option = $%X)[0m", IAC_Command, IAC_Option);
            #endif
        break;
    }

    #ifdef IAC_LOGGING
    kprintf("Ending IAC");
    #endif

    NextByte = NC_Data;
    IAC_Command = 0;
    IAC_Option = 0xFF;
    IAC_SubNegotiationOption = 0xFF;

    return;
}
