#include "Telnet.h"
#include "UTF8.h"
#include "Utils.h"
#include "Network.h"
#include "Cursor.h"
#include "DevMgr.h"
#include "WinMgr.h"
#include "Mouse.h"
#include "Input.h"
#include "Keyboard.h"
#include "SwRenderer.h"
#include "Palette.h"

#include "misc/VarList.h"
#include "system/PseudoFile.h"
#include "system/Time.h"
#include "system/Sprite.h"
#include "system/StatusBar.h"

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

// Telnet IAC options - https://www.iana.org/assignments/telnet-options/telnet-options.xhtml
#define TO_RECONNECTION
#define TO_BIN_TRANS                    0
#define TO_ECHO                         1
#define TO_SUPPRESS_GO_AHEAD            3
#define TO_APPROX_MSG_SZ_NEGOTIATION    4
#define TO_STATUS                       5
#define TO_TIMING_MARK                  6
#define TO_RMC_TRANS_ECHO               7   // Remote Controlled Trans and Echo [RFC726]
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
#define TO_DATA_ENTRY_TERM              20  // Data Entry Terminal [RFC1043][RFC732]
#define TO_SUPDUP                       21  // SUPDUP [RFC736][RFC734]
#define TO_SUPDUP_OUTPUT                22  // SUPDUP Output [RFC749]
#define TO_SEND_LOCATION                23  // $17
#define TO_TERM_TYPE                    24  // $18
#define TO_END_REC                      25  // End of Record [RFC885]
#define TO_USER_IDENT                   26
#define TO_NAWS                         31  // Negotiation About Window Size
#define TO_TERM_SPEED                   32
#define TO_RFLOW_CTRL                   33
#define TO_LINEMODE                     34
#define TO_XDISP                        35
#define TO_ENV                          36  // ENVIRON
#define TO_AUTH_OPTION                  37  // Authentication Option [RFC2941]
#define TO_ENCRYPTION_OPTION            38  // Encryption Option [RFC2946]
#define TO_ENV_OP                       39  // NEW-ENVIRON
/*
40 	    TN3270E 	[RFC2355]
41 	    XAUTH 	[Rob_Earhart]
42 	    CHARSET 	[RFC2066]
43 	    Telnet Remote Serial Port (RSP) 	[Robert_Barnes]
44 	    Com Port Control Option 	[RFC2217]
45 	    Telnet Suppress Local Echo 	[Wirt_Atmar]
46 	    Telnet Start TLS 	[Michael_Boe]
47 	    KERMIT 	[RFC2840]
48 	    SEND-URL 	[David_Croft]
49 	    FORWARD_X 	[Jeffrey_Altman]
50-137  Unassigned 	[IANA]
138     TELOPT PRAGMA LOGON 	[Steve_McGregory]
139     TELOPT SSPI LOGON 	[Steve_McGregory]
140     TELOPT PRAGMA HEARTBEAT 	[Steve_McGregory]
141-254 Unassigned 	
255     Extended-Options-List 	[RFC861]
*/

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
static void DoSpecialByte(u8 byte);
static void DoAPC(u8 byte);
static void DoEscape(u8 byte);
static void ResetSequence();
static void DoIAC(u8 byte);

// Next "expected" type of byte to be received
NextCommand NextByte = NC_Data;

// Telnet modifiable variables
static u8 vDoGA = 0;            // Use Go-Ahead
u8 vDECLRMM = 0;                // This control function defines whether or not the set left and right margins (DECSLRM) control function can set margins.
u8 vDECCKM = 0;                 // Cursor Key Format (DECCKM) - 0 = OFF - 1 = ON (TLDR: Numlock)
u8 sv_AllowRemoteEnv = FALSE;   // Allow remote server to access/modify local enviroment variables
u8 vBracketedPaste = FALSE;     // In bracketed paste mode (Unused/unimplemented)
bool bEnableUTF8 = TRUE;
bool bANSI_SYS_Emulation = FALSE;
static u8 vMinimized = FALSE;
static bool bNoLineModeNeg = FALSE;    // If true then no editing/signaling status may be done - Remote server may request/demand to not begin sub-negotiation of the editing/signaling status

// DECSTBM
s16 DMarginTop = 0;      // Region top margin
s16 DMarginBottom = 0;   // Region bottom margin

// DECLRMM
s16 DMarginLeft = 0;     // Region left margin
s16 DMarginRight = 0;    // Region right margin

// DECOM
u8 vDECOM = FALSE;       // DEC Origin Mode enabled
static u8 Saved_DECOM = FALSE;
static s16 Saved_OrgTop[2] = {0, 0}, Saved_OrgBottom[2] = {29, 29};     // Saved cursor origin
static s16 Saved_sx[2] = {0, 0}, Saved_sy[2] = {C_YSTART, C_YSTART};    // Saved cursor position

// XTCHECKSUM
static u8 XTCHECKSUM = 1;

// Escapes [
static u8 ESC_Seq = 0;
static u8 ESC_Type = 0;
static u8 ESC_Param[10] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static u16 ESC_Param16 = 0xFFFF;
static u8 ESC_ParamSeq = 0;
static char ESC_Buffer[5] = {'\0','\0','\0','\0', 0};
static u8 ESC_BufferSeq = 0;
static char SpecialCharacter = 0;

// Mode parser
static u8 ESC_QBuffer[6];
static u8 ESC_QSeq = 0;
static u8 ESC_QSeqMulti = 0;
static u16 QSeqNumber = 0; // atoi'd ESC_QBuffer

// Operating System Control (OSC)
static char ESC_OSCBuffer[2] = {0xFF,'\0'};
static u8 ESC_OSCSeq = 0;
static u16 OSC_Type = 0;
static char OSC_String[128];
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
static u16 MLastX = 400, MLastY = 400;  // Last reported X/Y mouse position
static u8 MLastBtn = 0;
typedef enum
{
    MT_None      = 0,
    MT_ClickOnly = 1,
    MT_DownUp    = 2,
    MT_HighLight = 3,
    MT_ClickDrag = 4,
    MT_Movement  = 5
} MT_Mode;  // Mouse tracking modes - mutually exclusive
static MT_Mode MTrackMode = MT_None;
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

static char FakeWindowLabel[128], FakeIconLabel[40]; // Faked window and icon title string. This can only be set by the remote server, thus it can only contain strings which the server already knows about.
static char **LabelStack;                           // Pushed/Popped window and icon label strings. 0-3 = Window, 4-7 = Icon
static u8 WindowNum, IconNum;                       // Number of window/icon labels on stack

static bool bDisplayControls = FALSE;   // Display Control Characters
bool bReverseColour = FALSE;            // Reverse Display Colors (DECSCNM)
u8 CharProtAttr = 0;                    // Character Protection Attribute (DECSCA)
static u8 Saved_Prot[2] = {0, 0};
static u8 C1_7Bit = 1;                         // 1 = S7C1T - 0/2 = S8C1T (7bit vs 8bit control characters)
static bool bCursorSaved[2] = {FALSE, FALSE};
static bool bMoreWorkaround = FALSE;

// Terminal functions
static inline __attribute__((always_inline)) void TF_DECSC();
static inline __attribute__((always_inline)) void TF_CUB(u8 num);
static inline __attribute__((always_inline)) void TF_CUP(u8 nx, u8 ny);

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

    Saved_DECOM = FALSE;

    Saved_OrgTop[0] = DMarginTop;
    Saved_OrgTop[1] = DMarginTop;

    Saved_OrgBottom[0] = DMarginBottom;
    Saved_OrgBottom[1] = DMarginBottom;

    LastPrintedChar = ' ';
    CharMapSelection = 0;

    memset(FakeWindowLabel, 0, 128);
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
                    ESC_ERROR("Failed to allocate label #%u", i);
                }

                memset(LabelStack[i], 0, 40);
            }
        }
        else
        {
            ESC_ERROR("Failed to allocate label stack!");
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
    bNoEcho = sv_bNoEcho;
    v_LineMode = sv_LineMode;
    v_Backspace = sv_Backspace;
    bLinefeedMode = sv_bLinefeedMode;
    bWrapMode = sv_bWrapMode;
    bEnableUTF8 = sv_bEnableUTF8;
    bDoCursorBlink = TRUE;
    bNoLineModeNeg = FALSE;

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
    MTrackMode = MT_None;
    MReportFormat = MR_Default;

    SpecialCharacter = 0;
    bDisplayControls = FALSE;
    bReverseColour = FALSE;
    CharProtAttr = 0;
    Saved_Prot[0] = 0;
    Saved_Prot[1] = 0;
    bCursorSaved[0] = FALSE;
    bCursorSaved[1] = FALSE;

    // Misc
    bMoreWorkaround = FALSE;
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
    if (MTrackMode == MT_None) return;

    u16 MCurX = (u16)Mouse_GetX();
    u16 MCurY = (u16)Mouse_GetY();
    u8 ML = get_KeyPress(MOUSE_LEFT_BTN);
    u8 MM = get_KeyPress(MOUSE_MIDDLE_BTN);
    u8 MR = get_KeyPress(MOUSE_RIGHT_BTN);
    u8 MCurrBtn = ((MR << 4) | (MM << 2) | ML);

    bool bMouseMoved  = ((MCurX != MLastX) || (MCurY != MLastY));
    bool bMouseClicked = (MCurrBtn != MLastBtn);//(MCurrBtn != (MLastBtn & 0x15));// && (MLastBtn & 0x2A == 0);

    //kprintf("X: %u - Y: %u - B: %u", MCurX != MLastX, MCurY != MLastY, MCurrBtn != MLastBtn);
    //kprintf("X: %u - Y: %u (%u) - TM: %u - RF: %u", MCurX, MCurY, MCurY/8, MTrackMode, MReportFormat);

    if (bMouse && (bMouseMoved || bMouseClicked))
    {
        char str[16];
        u16 len = 0;
        u8 btn = 0;
        u8 column = 0;
        u16 row = 0;
        bool bUp = 0;

        switch (MTrackMode)
        {
            case MT_ClickOnly:
                if ((ML == KEYSTATE_DOWN) || (MM == KEYSTATE_DOWN) || (MR == KEYSTATE_DOWN))
                {
                    //btn = ML + MM + MR;
                    // FixMe
                }
                else goto NoSend;

                column = (MCurX / 4) + 1;
                row = (MCurY / 8) + (D_VSCROLL == -8 ? 0 : 1);
            break;

            case MT_DownUp:
            {
                     if (ML == KEYSTATE_DOWN) { btn = 0; bUp = FALSE; }
                else if (ML == KEYSTATE_UP)   { btn = 3; bUp = TRUE;  }
                else if (MM == KEYSTATE_DOWN) { btn = 1; bUp = FALSE; }
                else if (MM == KEYSTATE_UP)   { btn = 3; bUp = TRUE;  }
                else if (MR == KEYSTATE_DOWN) { btn = 2; bUp = FALSE; }
                else if (MR == KEYSTATE_UP)   { btn = 3; bUp = TRUE;  }
                else goto NoSend;

                column = (MCurX / 4) + 1;
                row    = (MCurY / 8) + (D_VSCROLL == -8 ? 0 : 1);

                //kprintf("Row: %u -- (%d / 8 = %u) + (%u) == %u", row, MCurY, MCurY/8, D_VSCROLL ? 0 : 1, (MCurY / 8) + (D_VSCROLL ? 0 : 1));

                break;
            }

            case MT_HighLight:
            break;

            case MT_ClickDrag:
            break;

            case MT_Movement:
            {
                //if (MLastBtn & 0x2A) goto NoSend;   // Don't act on button presses being released last frame

                     if (ML == KEYSTATE_DOWN) { btn = 0; bUp = FALSE; }
                else if (ML == KEYSTATE_UP)   { btn = 0; bUp = TRUE;  }
                else if (MM == KEYSTATE_DOWN) { btn = 1; bUp = FALSE; }
                else if (MM == KEYSTATE_UP)   { btn = 1; bUp = TRUE;  }
                else if (MR == KEYSTATE_DOWN) { btn = 2; bUp = FALSE; }
                else if (MR == KEYSTATE_UP)   { btn = 2; bUp = TRUE;  }
                else 
                { 
                    btn = 3; 
                    bUp = FALSE;

                    //kprintf("CurrBtn: $%X - LastBtn: $%X - Mouse moved: %s - Mouse clicked: %s", MCurrBtn, MLastBtn, bMouseMoved?"Yes":"No ", bMouseClicked?"Yes":"No ");
                }
                
                if (bKB_Shift)   {btn += 4;}
                if (bKB_Alt)     {btn += 8;}
                if (bKB_Ctrl)    {btn += 16;}
                if (bMouseMoved) {btn += 32;}// else kprintf("CurrBtn: $%X - LastBtn: $%X - Mouse moved: %s - Mouse clicked: %s", MCurrBtn, MLastBtn, bMouseMoved?"Yes":"No ", bMouseClicked?"Yes":"No ");

                column = (MCurX / 4) + 1;
                row    = (MCurY / 8) + (D_VSCROLL == -8 ? 0 : 1);
                break;
            }
        
            default: goto NoSend; break;
        }
        
        switch (MReportFormat)
        {
            case MR_Default:            
                len = sprintf(str, "\e[M%u%u%u", btn, column, row);
                NET_SendStringLen(str, len);
            break;
            
            case MR_Multibyte:
            break;

            case MR_Digits:
            {
                if (bUp) len = sprintf(str, "\e[<%d;%d;%dm", btn, column, row);
                else     len = sprintf(str, "\e[<%d;%d;%dM", btn, column, row);
                
                NET_SendStringLen(str, len);

                //kprintf("Digits - Sendstr: \"%s\" - row: %u", str+1, row);
                break;
            }

            case MR_URXVT:
            break;
        
            default:
            break;
        }

        NoSend:
        {
            MLastX = MCurX;
            MLastY = MCurY;
            MLastBtn = MCurrBtn & 0x15; // Do not store button up press - it has already been processed at this point
        }
    }
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

        case NC_SpecialByte:
            DoSpecialByte(byte);
        break;

        case NC_APC:
            DoAPC(byte);
        break;
    }

    return;

    Data:

    if (bDisplayControls)
    {
        bEnableUTF8 = FALSE;

        if ((byte > 0 && byte < 8) ||   // 1-7
            (byte == 9) ||              // 9
            (byte == 11) ||             // 11
            (byte > 15 && byte < 27) || // 16-26 
            (byte > 27 && byte < 32)    // 28-31 
           )
        {
            goto print;
        }
    }
    else if (bEnableUTF8)
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
    // Only display a limited set of control characters when bDisplayControls and bEnableUTF8 are both false
    else
    {
        if ((byte > 0  && byte < 7) ||  // 1-6
            (byte > 15 && byte < 24) || // 16-23 
            (byte > 27 && byte < 32)    // 28-31 
           )
        {
            goto print;
        }
    }

    SkipUTF:

    switch (byte)
    {
        default:
            if (C1_7Bit == 0 && byte == 0x9F)
            {
                NextByte = NC_APC;
                return;
            }

            print:
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
            
            if ((byte < 0x20) /*|| (byte > 0x7E)*/) ESC_ERROR("ParseRX: Caught unhandled byte: $%X", byte);
        break;
        case 0x1B:  // Escape 1
            NextByte = NC_Escape;
        break;
        case TC_IAC:  // IAC
            NextByte = NC_IAC;
        break;
        case 0x0A:  // Line feed (new line)
        case 0x0B:  // Vertical tab
            if (bLinefeedMode) TTY_SetSX(0);  // Convert \n to \n\r

            TTY_MoveCursor(TTY_CURSOR_DOWN, 1);
            bPendingWrap = FALSE;
        break;
        case 0x0D:  // Carriage return
            if (vDECOM) TTY_SetSX(DMarginLeft); // -1?
            else TTY_SetSX(0);

            TTY_MoveCursor(TTY_CURSOR_DUMMY);   // Dummy
            bPendingWrap = FALSE;
        break;
        case 0x08:  // Backspace - Should be the same as CUB(1)
            TTY_MoveCursor(TTY_CURSOR_LEFT, 1);
            //TF_CUB(1);
        break;
        case 0x09:  // Horizontal tab
            //ESC_INFO("Horizontal tab: %u -> %u", (TTY_GetSX() % 80), Find_NextTabStop());
            
            TTY_SetSX(Find_NextTabStop());
        break;
        case 0x0C:  // Form feed (new page)
            TTY_SetSX(0);
            TTY_SetSY(C_YSTART);

            TTY_ResetVScroll();

            if (TTY_GetFont() == FONT_SOFTWARE)
            {
                SW_ClearScreen();
            }
            else
            {
                TRM_ClearPlane(BG_A);
                TRM_ClearPlane(BG_B);
            }

            TTY_MoveCursor(TTY_CURSOR_DUMMY);   // Dummy
        break;
        /*case 0x0E:  // Shift Out
            CharMapSelection = 1;
        break;
        case 0x0F:  // Shift In
            CharMapSelection = 0;
        break;*/
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
            ESC_WARN("Unimplemented VT100 mode single-character function: $%X", byte);
        break;
    }
}

void ChangeTitle(const char *str)
{
    char TitleBuf[128];

    snprintf(TitleBuf, 128, "%s> %s", STATUS_TEXT_SHORT, str);
    SB_ResetStatusText();
    SB_SetTitleMaxLen(34);
    SB_SetStatusText(TitleBuf);
}

static void DoSpecialByte(u8 byte)
{
    switch (SpecialCharacter)
    {
        case '#':
        {
            switch (byte)
            {
                case '3': // Set Double Height Line Top Half (DECDHL)
                    ESC_ERROR("Not implemented: ESC # 3");
                break;
                case '4': // Set Double Height Line Bottom Half (DECDHL)
                    ESC_ERROR("Not implemented: ESC # 4");
                break;
                case '5': // Set Single Width Line (DECSWL)
                    ESC_ERROR("Not implemented: ESC # 5");
                break;
                case '6': // Set Double Width Line (DECDWL)
                    ESC_ERROR("Not implemented: ESC # 6");
                break;
                case '8': // Fill Screen with E (DECALN)
                    vDECOM = FALSE;

                    DMarginTop = 0;
                    DMarginBottom = C_YMAX;
                    DMarginLeft = 0;
                    DMarginRight = C_XMAX;

                    if (TTY_GetFont() == FONT_SOFTWARE)
                    {
                        SW_FillScreen('E');
                    }
                    else
                    {
                        TRM_FillPlane(BG_A, 0x6000 + AVR_FONT0 + 0x45);
                        TRM_FillPlane(BG_B, 0x6000 + AVR_FONT0 + 0x45);
                    }
                    
                    //urxvt - Does not reset margins.
                    //vte - Does not set the cursor position, does not reset margins.

                    TTY_SetSX(0);
                    TTY_SetSY_A(0);  // Using C_YSTART (1) results in row being reported as 3 instead of 1
                    TTY_MoveCursor(TTY_CURSOR_DUMMY);
                break;
            
                default:
                    ESC_WARN("Not implemented: Unknown ESC # %c", byte);
                break;
            }
            break;
        }

        case '%':
        {
            switch (byte)
            {
                case '8': // Alias: Enable UTF-8 mode
                case 'G': // Enable UTF-8 mode
                    ESC_INFO("Enabling UTF-8 mode");
                    bEnableUTF8 = TRUE;
                break;
                case '@': // Disable UTF-8 mode
                    ESC_INFO("Disabling UTF-8 mode");
                    bEnableUTF8 = FALSE;
                break;
            
                default:
                    ESC_WARN("Not implemented: Unknown ESC %% %u", byte);
                break;
            }
            break;
        }

        case ' ':
        {
            switch (byte)
            {
                case 'F': // Use 7-bit controls (S7C1T)
                    C1_7Bit = 1;

                    ESC_ERROR("Not implemented: ESC ␣ F");
                break;
                case 'G': // Use 8-bit controls (S8C1T)
                    C1_7Bit = 0;

                    ESC_ERROR("Not implemented: ESC ␣ G");
                break;
                case 'L': // ANSI Charset Level 1 (ANSI_LEVEL_1)
                    ESC_ERROR("Not implemented: ESC ␣ L");
                break;
                case 'M': // ANSI Charset Level 2 (ANSI_LEVEL_2)
                    ESC_ERROR("Not implemented: ESC ␣ M");
                break;
                case 'N': // ANSI Charset Level 3 (ANSI_LEVEL_3)
                    ESC_ERROR("Not implemented: ESC ␣ N");
                break;
            
                default:
                    ESC_WARN("Not implemented: Unknown ESC # %c", byte);
                break;
            }
            break;
        }

        case '^':   // "Used by other terminals" ...
        {
            // This sequence ends in a ESC\ string terminator, wait for it...
            if (byte == '\\') 
            {
                ESC_WARN("Unknown ESC ^ ...");
                break;
            }
            else return;
        }

        case 'P':
        {
            if (byte != '\e')
            {
                ESC_INFO("Got special character '%c' ending with '%c'", SpecialCharacter, byte);
                
                ESC_Param[ESC_ParamSeq] = byte;
                ESC_ParamSeq++;
            }

            // Wait until we get a string terminator (ESC\), we won't knot what to do until we get it
            if (byte == '\\')
            {
                switch (ESC_Param[0])
                {
                    case '$':
                    {
                        switch (ESC_Param[1])
                        {
                            case 'q':   // DECRQSS - Request Selection or Setting (https://vt100.net/docs/vt510-rm/DECRQSS.html)
                            {
                                switch (ESC_Param[2])
                                {
                                    case 'm':   // SGR state
                                    {
                                        //kprintf("Send SGR state here - %d");
                                        
                                        char str[32];
                                        u16 len = 0;
                    
                                        len = sprintf(str, "\eP1$r%sm\e\\", "0");  // .P1$r48:2:1:2:3m
                                        NET_SendStringLen(str, len);

                                        break;
                                    } // case m

                                    case '"':   // ...
                                    {
                                        switch (ESC_Param[3])
                                        {
                                            case 'p':   // ...
                                            {
                                                ESC_ERROR("Not implemented: ESC P$q\"p");
                                                break;
                                            } // case p

                                            case 'q':   // ...
                                            {
                                                ESC_ERROR("Not implemented: ESC P$q\"q");
                                                break;
                                            } // case q

                                            default: 
                                            {
                                                ESC_WARN("Got Unknown P$q\" query character '%c' (Seq = 3)", byte);
                                                break;
                                            }
                                        }

                                        break;
                                    } // case "
                                
                                    default: 
                                    {
                                        ESC_WARN("Got Unknown P$q query character '%c' (Seq = 2)", byte);
                                        break;
                                    }
                                }
                                break;
                            } // case q
                        
                            default: 
                            {
                                ESC_WARN("Got Unknown P$ character '%c' (Seq = 1)", byte);
                                break;
                            }
                        }
                        break;
                    } // case $

                    case '+':
                    {
                        switch (ESC_Param[1])
                        {
                            case 'p':   // Set Terminal Description for 'tcap' Keyboard Mapping
                            {
                                ESC_ERROR("Not implemented: ESC P+p");
                                break;
                            } // case p

                            case 'q':   // Query Keyboard Mapping or Miscellaneous Information
                            {
                                ESC_ERROR("Not implemented: ESC P+q");
                                break;
                            } // case q
                        
                            default: 
                            {
                                ESC_WARN("Got Unknown P+ character '%c' (Seq = 1)", byte);
                                break;
                            }
                        }
                        break;
                    } // case +

                    default: 
                    {
                        ESC_WARN("Got Unknown P character '%c (Seq = 0)'", byte);
                        break;
                    }
                }

                break;
            }

            if (ESC_ParamSeq > 9) break;    // Should probably add some kind of failsafe here and break if we never get our string terminator...

            return;
        }
    
        default:
            ESC_WARN("Got Unknown special character '%c' ending with '%c'", SpecialCharacter, byte);
        break;
    }
    
    ResetSequence();
    return;
}

static void DoAPC(u8 byte)
{
    switch (byte)
    {
        case 0x1B:
        {
            NextByte = NC_Escape;
            break;
        }
    
        default:
            ESC_INFO("Caught APC byte: $%X", byte);
        break;
    }
}

// https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
// https://en.wikipedia.org/wiki/ANSI_escape_code#CSIsection
// https://terminalguide.namepad.de/seq/
static void DoEscape(u8 byte)
{
    u8 RestartByte = 0;
    ESC_Seq++;

    switch (ESC_Type)
    {
        case 0x9F:
        {
            ESC_INFO("Got $9F APC payload byte ($%X)", byte);

            switch (byte)
            {
                case 0x1B:
                {
                    NextByte = NC_Escape;

                    break;
                }
            
                default:
                break;
            }
            return;
        }

        case '[':
        {
            switch (byte)
            {
                case ':':
                case ';':
                {
                    if (ESC_ParamSeq == 0) ESC_Param16 = atoi16(ESC_Buffer);

                    if (ESC_ParamSeq >= 10) return;

                    ESC_Param[ESC_ParamSeq++] = atoi(ESC_Buffer);
                    //kprintf("ESC_ParamSeq[%u] = %u (%s)", ESC_ParamSeq-1, ESC_Param[ESC_ParamSeq-1], ESC_Buffer);
                    //kprintf("Got a ';' : ESC_Param[%u] = $%X", ESC_ParamSeq-1, ESC_Param[ESC_ParamSeq-1]);

                    ESC_Buffer[0] = '\0';
                    ESC_Buffer[1] = '\0';
                    ESC_Buffer[2] = '\0';
                    ESC_Buffer[3] = '\0';
                    ESC_BufferSeq = 0;

                    return;
                }

                case '@':   // Insert Blanks (ICH) - "ESC[ Ⓝ @"
                {
                    u8 n = atoi(ESC_Buffer);

                    // Default param
                    n = n == 0   ? 1 : n;
                    n = n == 255 ? 1 : n;

                    s16 x = TTY_GetSX();
                    s16 y = TTY_GetSY_A();

                    if (TTY_GetFont() == FONT_SOFTWARE) SW_ShiftRow_Right(y, x, n);

                    ESC_INFO("ESC[%u@ - Insert %d blanks at %d", n, n, x);

                    goto EndEscape;
                }

                case '}':   // Insert Column (DECIC) - "ESC[ Ⓝ ' }" -- TODO: ending } collides with "Restore Rendition Attributes", take care of both cases here!
                {
                    u8 n = atoi(ESC_Buffer);
                    n = n == 0   ? 1 : n;

                    ESC_ERROR("Not implemented: ESC[%u'} - Insert %u Columns", n, n);

                    goto EndEscape;
                }

                case '~':   // Delete Column (DECDC) - "ESC[ Ⓝ ' ~"
                {
                    u8 n = atoi(ESC_Buffer);
                    n = n == 0   ? 1 : n;

                    ESC_ERROR("Not implemented: ESC[%u'~ - Delete %u Columns", n, n);

                    goto EndEscape;
                }

                case 'A':   // Cursor Up (CUU)
                {
                    u8 n = atoi(ESC_Buffer);
                    n = (n ? n : 1);

                    if (TTY_GetSY_A() - n >= DMarginTop)
                    {
                        TTY_SetSY_A(TTY_GetSY_A() - n);
                        TTY_MoveCursor(TTY_CURSOR_DUMMY);
                    }
                    else
                    {
                        TTY_SetSY_A(DMarginTop);
                    }

                    goto EndEscape;
                }

                case 'B':   // Cursor Down (CUD)
                {
                    u8 n = atoi(ESC_Buffer);
                    n = (n ? n : 1);
                    TTY_SetSY_A(TTY_GetSY_A() + n);
                    TTY_MoveCursor(TTY_CURSOR_DUMMY);
                    
                    ESC_DEBUG("ESC[%uB - SY_A: %d", n, TTY_GetSY_A());

                    goto EndEscape;
                }

                case 'C':   // Cursor Forward (CUF)
                {
                    /*
                        A Few ANSII images expect CUF to wrap, however no where is it specified that CUF can or should wrap... quite the opposite; Its explicitly stated that it should NEVER wrap...
                        Comparing the output of SMDT and other terminals seem to match up, neither prints these ANSII images correctly.
                    */

                    u16 n = atoi16(ESC_Buffer);
                    s16 right = DMarginRight;
                    s16 cx = TTY_GetSX()+1;

                    ESC_DEBUG("ESC[%uC - SX: %d", n, TTY_GetSX());

                    n = n == 0 ? 1 : n;
                    n += cx;

                    if (bANSI_SYS_Emulation)
                    {
                        if (n >= C_XMAX)
                        {
                            bPendingWrap = TRUE;
                            u8 ax = n - C_XMAX;
                            TTY_MoveCursor(TTY_CURSOR_DOWN, 1);
                            n = ax + 1;
                        }

                    }
                    else
                    {
                        if (cx > right) right = C_XMAX;

                        if (n > right) n = right;
                    }

                    TTY_SetSX(n-1);
                    TTY_MoveCursor(TTY_CURSOR_DUMMY);

                    bPendingWrap = FALSE;

                    ESC_DEBUG("n: %u - SX: %d - right: %d", n, TTY_GetSX(), right);
                    goto EndEscape;
                }

                case 'D':   // Cursor Backward (CUB)
                {
                    /*
                    This unsets the pending wrap state without wrapping.

                    If not both of reverse wrap mode and wraparound mode are set:
                        Move the cursor amount cells left. If it would cross the left-most column of the scrolling region, stop at the left-most column of the scrolling region. If the cursor would move left of the left-most column of the screen, move to the left most column of the screen.
                    Else:
                        If the pending wrap state is set, reduce amount by one.
                    */

                    u8 n = atoi(ESC_Buffer);
                    n = n == 0 ? 1 : n;

                    TF_CUB(n);

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
                    s16 y = TTY_GetSY_A();
                    u8 n = atoi(ESC_Buffer);
                    n = (n ? n : 1);

                    ESC_WARN("ESC[%uF CPL - sy: %d - result: $%2X ($%X)", n, y, (u8)y-n, (n > y)?y:n);

                    // Cap n to avoid underflowing / going beyond top line (See test_CPL_StopsAtTopLine)
                    n = n > y ? y : n;

                    if (vDECOM)
                    {
                        if (y >= DMarginTop)
                        {
                            if (y - n < DMarginTop) n = DMarginTop - y;
                        }
                    }

                    TTY_MoveCursor(TTY_CURSOR_UP, n);
                    TTY_SetSX(0);
                    goto EndEscape;
                }

                case 0x60:  // 0x60 = ` - Cursor Horizontal Position Absolute (HPA)
                case 'G':   // Alias
                {
                    // This sequence performs cursor position (CUP) with x set to the parameterized value and y set to the current cursor position. 
                    // There is no additional or different behavior for using HPA.

                    u8 y = TTY_GetSY_A()+1;
                    u16 x = atoi16(ESC_Buffer);

                    x = x > C_XMAX ? C_XMAX : x;    // CUP does bounds checking on its own. Only check if its larger than screen width here
                    
                    ESC_INFO("HPA - X: %u (Y: %u)", x, y);

                    TF_CUP((u8)x, y);

                    goto EndEscape;
                }
                
                case 'I':   // Cursor Horizontal Forward Tabulation (CHT) - "ESC[ Ⓝ I" -- Same as Horizontal Tab (TAB) times Ⓝ
                {
                    u8 n = atoi(ESC_Buffer);
                    n = (n ? n : 1);
                    n = n == 255 ? 1 : n;

                    ESC_INFO("Horizontal tab %u times", n);
                    
                    while (n--) TTY_SetSX(Find_NextTabStop());

                    goto EndEscape;
                }

                case 'J':   // Erase Display [Dispatch] (ED)
                {
                    u8 n = atoi(ESC_Buffer);
                    n = n == 255 ? 0 : n;

                    switch (n)
                    {                    
                        case 0: // Clear screen from cursor down (Keep cursor position)
                            //TTY_ClearLine(TTY_GetSY()+2, C_YMAX - TTY_GetSY_A());
                            TTY_ClearLine(TTY_GetSY()/*+1*/, C_YMAX - TTY_GetSY_A()); // y+1 = exclusive
                            bPendingWrap = FALSE;
                        break;

                        case 1: // Clear screen from cursor up (Keep cursor position)
                            //TTY_ClearLine(TTY_GetSY(), TTY_GetSY_A());
                            TTY_ClearLine(TTY_GetSY() - TTY_GetSY_A(), TTY_GetSY_A()+1);  // linecount+1 = inclusive
                            bPendingWrap = FALSE;
                        break;

                        case 2: // Clear screen (move cursor to top left only if emulating ANSI.SYS otherwise keep cursor position)
                            if (TTY_GetFont() == FONT_SOFTWARE)
                            {
                                SW_ClearScreen();
                            }
                            else
                            {
                                TRM_ClearPlane(BG_A);
                                TRM_ClearPlane(BG_B);
                            }

                            if (bANSI_SYS_Emulation)
                            {
                                TTY_SetSX(0);
                                TTY_SetSY_A(0);
                            }

                            bPendingWrap = FALSE;
                        break;

                        case 3: // Erase only the scrollback region (Keep cursor position)
                            if (TTY_GetFont() == FONT_SOFTWARE)
                            {
                                SW_ClearScreen();
                            }
                            else
                            {
                                TRM_ClearPlane(BG_A);
                                TRM_ClearPlane(BG_B);
                            }
                        break;

                        default: break;
                    }            

                    goto EndEscape;
                }

                case 'K':   // Erase Line [Dispatch] (EL) - "ESC[ Ⓝ K"
                {
                    u8 n = atoi(ESC_Buffer);
                    n = n == 255 ? 0 : n;

                    switch (n)
                    {
                        case 0: // Erase from cursor to end of line (Keep cursor position)
                            TTY_ClearPartialLine(sy & 31, TTY_GetSX(), C_XMAX);
                            bPendingWrap = FALSE;
                        break;

                        case 1: // Erase start of line to the cursor (Keep cursor position)
                            TTY_ClearPartialLine(sy & 31, 0, TTY_GetSX()+1);
                            bPendingWrap = FALSE;
                        break;

                        case 2: // Erase the entire line (Keep cursor position)
                            TTY_ClearLine(sy & 31, 1);
                            bPendingWrap = FALSE;
                        break;

                        default: break;                    
                    }            

                    goto EndEscape;
                }

                case 'L':   // Insert Line (IL) - "ESC[ Ⓝ L"
                {
                    u8 n = atoi(ESC_Buffer);

                    // Default param
                    n = n == 0   ? 1 : n;
                    n = n == 255 ? 1 : n;

                    s16 x = TTY_GetSX()   +1;
                    s16 y = TTY_GetSY_A() +1;

                    ESC_INFO("Insert Line (IL) %u times", n);
                    ESC_INFO("x: %d - y: %d - l: %d - r: %d - t: %d - b: %d", x, y, DMarginLeft, DMarginRight, DMarginTop, DMarginBottom);


                    // If outside margins then do nothing
                    if (x < DMarginLeft || x > DMarginRight) goto EndEscape;

                    if (y < DMarginTop || y > DMarginBottom) goto EndEscape;

                    // Insert blank line / shift lines downward (only available with software renderer)
                    if (TTY_GetFont() == FONT_SOFTWARE)
                    {
                        SW_ShiftLinesDown(n);
                    }

                    // Move cursor to left margin
                    TTY_SetSX(DMarginLeft); // -1 ?

                    bPendingWrap = FALSE;

                    goto EndEscape;
                }

                case 'M':   // Delete Line (DL) - "ESC[ Ⓝ M"
                {
                    u8 max_lines = (bPALSystem?29:27);

                    if (vDECOM)
                    {
                        max_lines = DMarginBottom - DMarginTop;
                    }

                    u8 y = sy & 31;
                    u8 n = atoi(ESC_Buffer);
                    
                    n = (n ? n : 1);
                    max_lines = n > max_lines ? max_lines : n;

                    if (TTY_GetFont() == FONT_SOFTWARE)
                    {
                        SW_ClearLine(y, max_lines);
                        SW_ShiftNumLinesUp(y, max_lines);
                    }
                    else  TTY_ClearLine(y, max_lines);

                    if (vDECLRMM) TTY_SetSX(DMarginLeft);
                    else TTY_SetSX(0);

                    
                    ESC_INFO("ESC[%uM (Delete Line) - sy: %d - max_lines: %d", n, y, max_lines);
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

                    u8 end  = cx + n;
                    u16 row = sy & 31;

                    if (TTY_GetFont() == FONT_SOFTWARE) SW_ShiftRow_Left(row, end, n);

                    ESC_INFO("Moving: Y: %d - From: %d - Num: %d", row, end, n);

                    cx   = (end - n) + 1;
                    end += 1;

                    TTY_ClearPartialLine(row, cx, end);

                    ESC_INFO("ESC[%uP (Delete Character)", n);
                    ESC_INFO("Clearing: Y: %d - From: %d - To: %d", row, cx, end);

                    goto EndEscape;
                }

                case 'S':   // Scroll Up (SU) - "ESC[ Ⓝ S"
                {           
                    u8 n = atoi(ESC_Buffer);
                    n = (n ? n : 1);

                    TTY_DrawScrollback(n);

                    goto EndEscape;
                }

                case 'T':   // 	Scroll Down (SD) / Track Mouse / Unset Title Mode   -- Only handles Unset Title Mode (>..T)
                {
                    ESC_Param[ESC_ParamSeq++] = atoi(ESC_Buffer);
                    u8 n = ESC_Param[0];

                    /*
                        Param1: ESC_Buffer[1]
                        Param2: ESC_Param[1]
                    */

                    /*
                    if (ESC_Param[1] <= 9 && ESC_Buffer[1] >= 0 && ESC_Buffer[1] <= 9)
                        ESC_ERROR("Not implemented: Unset title mode: %u;%u", ESC_Buffer[1], ESC_Param[1]);
                    else
                        ESC_ERROR("Not implemented: Unset title mode: ALL");
                    */

                    switch (n)
                    {
                        // Reset all title modes
                        case 0:
                        {
                            break;
                        }

                        default:
                        {                        
                            break;
                        }
                    }

                    goto EndEscape;
                }
                
                case 'X':   // Erase Character (ECH) - "ESC[ Ⓝ X"
                {
                    u8 n = atoi(ESC_Buffer);
                    n = (n ? n : 1);     // If n == 0 then adjust to n to 1

                    s16 oldsx = TTY_GetSX();
                    s16 oldsy = TTY_GetSY_A();

                    for (u16 i = 0; i < n; i++)
                    {
                        TTY_PrintChar(' ');
                    }

                    TTY_SetSX(oldsx);
                    TTY_SetSY_A(oldsy);

                    TTY_MoveCursor(TTY_CURSOR_DUMMY);

                    ESC_INFO("Erase Character (ECH): From %u to %u", oldsx, oldsx + n);

                    bPendingWrap = FALSE;

                    goto EndEscape;
                }

                case 'Z':   // Cursor Horizontal Backward Tabulation (CBT) - "ESC[ Ⓝ Z"
                {
                    u8 n = atoi(ESC_Buffer);
                    n = (n ? n : 1);
                    n = n == 255 ? 1 : n;

                    ESC_INFO("Horizontal tab: %u <- %u (n = %u)", Find_LastTabStop(), (TTY_GetSX() % 80), n);

                    while (n--) TTY_SetSX(Find_LastTabStop());

                    goto EndEscape;
                }

                case 'a':   // Cursor Horizontal Position Relative (HPR)
                {
                    u8 n = atoi(ESC_Buffer);
                    //n = (n ? n : 1);
                    s16 x = TTY_GetSX() + n;

                    // If cursor origin mode is set the cursor row will be forced inside the current scroll region.
                    // If cursor origin mode is set and left and right margin mode is set the cursor adjusted to be on or left of the right-most column of the scrolling region.
                    if (vDECOM && vDECLRMM)
                    {
                        if (x > DMarginRight) x = DMarginRight;
                        else if (x < DMarginLeft) x = DMarginLeft;
                    }
                    else if (vDECOM)
                    {
                        if (x > C_XMAX) x = C_XMAX;
                        else if (x < 0) x = 0;
                    }
                    else
                    {
                        if (x > C_XMAX) x = C_XMAX;
                        else if (x < 0) x = 0;
                    }

                    TTY_SetSX(x);
                    bPendingWrap = FALSE;
                    
                    ESC_DEBUG("ESC[%ua - x: %d", n, x);

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
        
                            len = sprintf(str, "\e[>%u;%u;0c", model, version);
                            NET_SendStringLen(str, len);
        
                            ESC_INFO("DA2: p0 = %u - Sending \"ESC[>%u;%u;0c\" regardless of p0", ESC_Buffer[1], model, version);

                            break;
                        }

                        case '=':   // Tertiary Device Attributes (DA3) - ""
                        {
                            if (ESC_Buffer[1] == '0')
                            {
                                NET_SendString("\eP!|00000000\e\\");
        
                                ESC_INFO("DA3: p0 = %u - Sending \"ESCP!|00000000ESC\\\"", ESC_Buffer[1]);
                            }
                            else
                            {
                                ESC_INFO("DA3: p0 = %u - Ignoring because p0 is non zero", ESC_Buffer[1]);
                            }

                            break;
                        }
                    
                        default:    // Primary Device Attributes (DA1) - ""
                        {
                            NET_SendString("\e[?1;2c");

                            ESC_INFO("DA1: p0 = %u - Sending \"ESC[?1;2c\" regardless of p0", ESC_Buffer[0]);

                            break;
                        }
                    }

                    goto EndEscape;
                }

                case 'd':   // Cursor Vertical Position Absolute (VPA) - "ESC[ Ⓝ d"
                {
                    u8 n = atoi(ESC_Buffer);
                    //n = (n ? n : 1);

                    ESC_DEBUG("ESC[%ud", n);

                    if (vDECOM)
                    {
                        TTY_SetSY_A(DMarginTop + (n-1));
                    }
                    else TTY_SetSY_A(n-1);

                    TTY_MoveCursor(TTY_CURSOR_DUMMY);

                    bPendingWrap = FALSE;

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

                    u8 nx = ESC_Param[1];
                    u8 ny = ESC_Param[0];

                    TF_CUP(nx, ny);

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
                            ESC_INFO("Clearing tab stop in column %u (CMD = %u)", c, n);
                        break;

                        case 2:
                        case 3:
                        case 5:
                            memset(HTS_Column, 0, 80);
                            ESC_INFO("Clearing tab stops in all columns. (CMD = %u)", n);
                        break;
                    
                        default:
                            ESC_WARN("Unknown TBC (CMD = %u)", n);
                        break;
                    }

                    goto EndEscape;
                }

                case 'h':   // Mode set ( Not prefixed with ? )
                {
                    u8 n = atoi(ESC_Buffer);

                    switch (n)
                    {
                        case 3:     // Display Control Characters
                            bDisplayControls = TRUE;
                            ESC_INFO("Enabling display of control characters");
                        break;

                        case 4:     // Insert Mode (IRM)
                            bInsertMode = TRUE;
                            ESC_INFO("Enabling Insert Mode (IRM)");
                        break;

                        case 20:    // Linefeed mode
                            bLinefeedMode = TRUE;
                            ESC_INFO("Enabling Linefeed Mode");
                        break;

                        default:
                            ESC_WARN("Unknown mode %u (%c) enable", n, n);
                        break;
                    }
                
                    goto EndEscape;
                }

                case 'l':   // Mode reset ( Not prefixed with ? )
                {                    
                    u8 n = atoi(ESC_Buffer);

                    switch (n)
                    {
                        case 3:     // Display Control Characters
                            bDisplayControls = FALSE;
                            ESC_INFO("Disabling display of control characters");
                        break;

                        case 4:     // Insert Mode (IRM)

                            bInsertMode = FALSE;
                            ESC_INFO("Disabling Insert Mode (IRM)");
                        break;

                        case 20:    // Linefeed mode
                            bLinefeedMode = FALSE;
                            ESC_INFO("Disabling Linefeed Mode");
                        break;

                        default:
                            ESC_WARN("Unknown mode %u (%c) disable", n, n);
                        break;
                    }

                    goto EndEscape;
                }

                case 'm':   // Select Graphic Rendition (SGR)
                {
                    ESC_Param[ESC_ParamSeq++] = atoi(ESC_Buffer);
                    u8 rgb = 0;
                    u8 base = 0;

                    //kprintf("Ran into an m at $%X", RXBytes-1);

                    ATT_LOG("0:<%u> 1:<%u> 2:<%u> 3:<%u> 4:<%u> 5:<%u> 6:<%u> 7:<%u> 8:<%u> 9:<%u> - Sq: %u", ESC_Param[0], ESC_Param[1], ESC_Param[2], ESC_Param[3], ESC_Param[4], ESC_Param[5], ESC_Param[6], ESC_Param[7], ESC_Param[8], ESC_Param[9], ESC_ParamSeq);
                    ATT_LOG("0:<%u> 1:<%u> 2:<%u> 3:<%u> 4:<%u> 5:<%u> 6:<%u> - S: %u", ESC_Param[0], ESC_Param[1], ESC_Param[2], ESC_Param[3], ESC_Param[4], ESC_Param[5], ESC_Param[6], ESC_ParamSeq);

                    DoSecondSet:
                    switch (ESC_Param[base])
                    {
                        //Select foreground color
                        case 38:
                        {
                            switch (ESC_Param[base+1])
                            {
                                // 8 bit foreground
                                case 5:
                                {
                                    // 0-15: Standard 16 colours (Normal and high intensity)
                                    if (ESC_Param[base+2] <= 15) 
                                    {
                                        u8 v = ESC_Param[base+2] + (ESC_Param[base+2] <= 7 ? 30 : 82);
                                        TTY_SetAttribute(v);
                                    }
                                    // 16-231:  6 × 6 × 6 cube (216 colors)
                                    else if (ESC_Param[base+2] <= 216) 
                                    {
                                        u8 result = ColorConv_666Cube(ESC_Param[base+2], TRUE);
                                        TTY_SetAttribute(result);
                                    }
                                    // 232-255:  grayscale from dark to light in 24 steps
                                    else
                                    {
                                        u8 result = ColorConv_666Cube_Grayscale(ESC_Param[base+2], TRUE);
                                        TTY_SetAttribute(result);
                                    }

                                    break;
                                }

                                // 24 bit foreground
                                case 2:
                                {
                                    rgb = ColorConv_24bit(ESC_Param[base+2], ESC_Param[base+3], ESC_Param[base+4], TRUE);
                                    TTY_SetAttribute(rgb);
                                    break;
                                }
                            
                                default:
                                    ATT_LOG("Unknown param[1]: %d", ESC_Param[base+1]);
                                break;
                            }   // Param[1] switch

                            if ((ESC_ParamSeq == 10) && (base != 5))
                            {
                                base = 5;
                                goto DoSecondSet;
                            }
                            else if ((ESC_ParamSeq == 6) && (base != 3))
                            {
                                base = 3;
                                goto DoSecondSet;
                            }

                            goto EndEscape;
                        }   // Case 38

                        //Select background color
                        case 48:
                        {
                            switch (ESC_Param[base+1])
                            {
                                // 8 bit background
                                case 5:
                                {
                                    // 0-15: Standard 16 colours (Normal and high intensity)
                                    if (ESC_Param[base+2] <= 15) 
                                    {
                                        u8 v = ESC_Param[base+2] + (ESC_Param[base+2] <= 7 ? 40 : 92);
                                        TTY_SetAttribute(v);
                                    }
                                    // 16-231:  6 × 6 × 6 cube (216 colors)
                                    else if (ESC_Param[base+2] <= 216) 
                                    {
                                        u8 result = ColorConv_666Cube(ESC_Param[base+2], FALSE);
                                        TTY_SetAttribute(result);
                                    }
                                    // 232-255:  grayscale from dark to light in 24 steps
                                    else
                                    {
                                        u8 result = ColorConv_666Cube_Grayscale(ESC_Param[base+2], FALSE);
                                        TTY_SetAttribute(result);
                                    }

                                    break;
                                }

                                // 24 bit background
                                case 2:
                                {
                                    rgb = ColorConv_24bit(ESC_Param[base+2], ESC_Param[base+3], ESC_Param[base+4], FALSE);
                                    TTY_SetAttribute(rgb);
                                    break;
                                }
                            
                                default:
                                    ATT_LOG("Unknown param[1]: %d", ESC_Param[base+1]);
                                break;
                            }   // Param[1] switch

                            if ((ESC_ParamSeq == 10) && (base != 5))
                            {
                                base = 5;
                                goto DoSecondSet;
                            }
                            else if ((ESC_ParamSeq == 6) && (base != 3))
                            {
                                base = 3;
                                goto DoSecondSet;
                            }

                            goto EndEscape;
                        }   // Case 48
                        
                        default:
                            for (int i = 0; i < 4; i++)
                            {
                                if (ESC_Param[base+i] != 255) 
                                {
                                    //ATT_LOG("Setting default attributes: %d", ESC_Param[base+i]);
                                    TTY_SetAttribute(ESC_Param[base+i]);
                                }
                            }
                        break;
                    }   // Param[0] switch

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
                            NET_SendString("\e[0n");
                        break;

                        case 6: // Cursor Position Report (CPR)
                        {
                            s16 cx = TTY_GetSX() + 1;
                            s16 cy = TTY_GetSY_A() + 1;

                            if (vDECOM)
                            {
                                cx -= DMarginLeft;
                                cy -= DMarginTop;
                            }

                            len = sprintf(str, "\e[%d;%dR", cy, cx);
                            NET_SendStringLen(str, len);
                            
                            ESC_DEBUG("Reporting cursor position: \"ESC%s\" - (y;x)", str+1);
                        break;
                        }

                        case 8: // Set Title to Terminal Name and Version.
                            // This could easily be set here, however current versions of SMDT prefixes all titles with this information already
                        break;

                        default:
                            ESC_WARN("Device Status Report [Dispatch] (DSR) - Unknown command $%X", n);
                        break;
                    }

                    goto EndEscape;
                }

                case 0xD:   // ??? Unknown ESC[!0xD sequence
                {
                    ESC_WARN("Ran into unknown ESC[<$%X> sequence - Reinserting byte <$%X> into stream", byte, byte);

                    // Reinsert byte into stream again, stop escape parser and rerun
                    RestartByte = byte;
                    goto Restart;
                }

                case 'p':   // Soft Reset (DECSTR) / Request Mode (RQM) / Alias: Save Rendition Attributes / ??? DECSR / Select VT-XXX Conformance Level (DECSCL)
                {
                    ESC_Param[ESC_ParamSeq++] = atoi(ESC_Buffer);

                    // Soft Reset (DECSTR)
                    if (ESC_Buffer[0] == '!')
                    {
                        ESC_INFO("Soft Reset (DECSTR)");
                        TELNET_Init(TF_ClearScreen || TF_ResetVariables);
                    }
                    // Select VT-XXX Conformance Level (DECSCL)
                    else if (ESC_Buffer[1] == '"')
                    {
                        u8 level = ESC_Param[0];
                        C1_7Bit = ESC_Buffer[0] - 48;

                        if (level < 61) 
                        {
                            ESC_INFO("DECSCL Skipping because level < 61");
                        }
                        else
                        {
                            level -= 60;
                            ESC_INFO("DECSCL Level: %u - 7bit: %s", level, C1_7Bit ? "yes" : "no");
                        }
                    }
                    // Request Mode (RQM)
                    else if (ESC_Buffer[1] == '$')
                    {
                        u8 data_end = ESC_Buffer[0] - 48;
                        ESC_INFO("Request Mode (RQM): %u", data_end);
                    }
                    else
                    {
                        ESC_WARN("Unknown ESC[p - Data:");

                        for (u8 i = 0; i < ESC_ParamSeq; i++)
                        {
                            ESC_WARN("ESC[p - PARAM[%i]: %u -- $%X -- '%c'", i, ESC_Param[i], ESC_Param[i], ESC_Param[i]);
                        }
    
                        ESC_WARN("ESC[p - BUFFER: 0: %u, 1: %u, 2: %u, 3: %u -- 0: '%c', 1: '%c', 2: '%c', 3: '%c'", ESC_Buffer[0], ESC_Buffer[1], ESC_Buffer[2], ESC_Buffer[3], ESC_Buffer[0], ESC_Buffer[1], ESC_Buffer[2], ESC_Buffer[3] == '\0' ? ' ' : ESC_Buffer[3]);
    
                    }

                    goto EndEscape;
                }

                case 'q':   // ... Multiple ...
                {
                    u8 n = ESC_Buffer[0] - 48;

                    if (ESC_Buffer[1] == '\0')
                    {
                        if (n < 4)  // Load LEDs (DECLL) - "ESC[ Ⓝ q"
                        {                            
                            ESC_ERROR("Not implemented: Load LEDs (DECLL) (CMD = %u)", n);
                        }
                        else if (n == '#')  // Alias: Restore Rendition Attributes - "ESC[ # q" - same as "ESC[ # }"
                        {
                            ESC_ERROR("Not implemented: (Alias) Restore Rendition Attributes");
                        }
                        
                        goto EndEscape;
                    }

                    char type = ESC_Buffer[1];

                    switch (type)
                    {
                        case '\"':    // Select Character Protection Attribute (DECSCA) - "ESC[ Ⓝ " q"
                        {                            
                            switch (n)
                            {
                                case 0:
                                case 2:
                                    CharProtAttr = 0;
                                break;

                                case 1:
                                    CharProtAttr = 1;
                                break;

                                default:
                                break;
                            }

                            ESC_INFO("Select Character Protection Attribute (DECSCA) (CMD = %u)", n);
                            break;
                        }

                        case '*':    // ??? DECSR - "ESC[ Ⓝ * q"
                        {
                            ESC_ERROR("Not implemented: ??? DECSR (CMD = %u)", n);
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
        
                                    if (TTY_GetFont()) LastCursor = 0x13;
                                    else         LastCursor = 0x10;
                                break;
                                
                                case 2: // Select Cursor Style Steady Block
                                    bDoCursorBlink = FALSE;
                                    
                                    if (TTY_GetFont()) LastCursor = 0x13;
                                    else         LastCursor = 0x10;
                                break;
                                
                                case 3: // Select Cursor Style Blinking Underline
                                    bDoCursorBlink = TRUE;
        
                                    if (TTY_GetFont()) LastCursor = 0x14;
                                    else         LastCursor = 0x11;
                                break;
                                
                                case 4: // Select Cursor Style Steady Underline
                                    bDoCursorBlink = FALSE;
                                    
                                    if (TTY_GetFont()) LastCursor = 0x14;
                                    else         LastCursor = 0x11;
                                break;
                                
                                case 5: // Select Cursor Style Blinking Bar
                                    bDoCursorBlink = TRUE;
        
                                    if (TTY_GetFont()) LastCursor = 0x15;
                                    else         LastCursor = 0x12;
                                break;
                                
                                case 6: // Select Cursor Style Steady Bar
                                    bDoCursorBlink = FALSE;
                                    
                                    if (TTY_GetFont()) LastCursor = 0x15;
                                    else         LastCursor = 0x12;
                                break;
                            }
        
                            SetSprite_TILE(SPRITE_CURSOR, LastCursor);
        
                            ESC_INFO("Select Cursor Style (DECSCUSR) (CMD = %u)", n);
                            break;
                        }
                        
                        default:
                        ESC_WARN("ESC[..q - Unknown type '%c'", type);
                        break;
                    }

                    goto EndEscape;
                }

                case 'r':   // Set Top and Bottom Margins (DECSTBM) - "ESC[ Ⓝ ; Ⓝ r"
                {
                    ESC_Param[ESC_ParamSeq++] = atoi(ESC_Buffer);

                    u8 top = ESC_Param[0];
                    u8 bottom = ESC_Param[1];

                    top    = top    == 0   ? 1      : top;
                    top    = top    == 255 ? 1      : top;
                    bottom = bottom == 255 ? C_YMAX : bottom;
                    bottom = bottom == 0   ? 1      : bottom;

                    if (top < bottom)
                    {
                        DMarginTop    = top;
                        DMarginBottom = bottom;

                        if (vDECOM)
                        {
                            TTY_SetSX(DMarginLeft);
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
                        DMarginTop    = 0;
                        DMarginBottom = C_YMAX;
                    }

                    ESC_INFO("Top: %u - Bottom: %u - vDECOM: %s", DMarginTop, DMarginBottom, vDECOM?"True":"False");
                    goto EndEscape;
                }

                case 's':   // Set Left and Right Margin (DECSLRM) when in DECLRMM mode, otherwise it is: Save Cursor [variant] (ansi.sys) - Same as Save Cursor (DECSC) (ESC 7)
                {
                    if (vDECLRMM)
                    {
                        ESC_Param[ESC_ParamSeq++] = atoi(ESC_Buffer);
                        u8 left  = ESC_Param[0];
                        u8 right = ESC_Param[1];
                        
                        left  = left  == 255 ? 1      : left;
                        right = right == 255 ? C_XMAX : right;
                        left  = left  == 0   ? 1      : left;
                        right = right == 0   ? C_XMAX : right;

                        s16 cx = 0;
                        s16 cy = 0;

                        if (left < right)
                        {
                            DMarginLeft  = left;
                            DMarginRight = right;
                            
                            ESC_INFO("Setting margin left  to %d", DMarginLeft);
                            ESC_INFO("Setting margin right to %d", DMarginRight);

                            if (vDECOM)
                            {
                                cx = DMarginLeft;
                                cy = DMarginTop;
                            }
                            else
                            {
                                cx = 0;
                                cy = 0;
                            }

                            ESC_INFO("Setting cx to %d (Current: %d)", cx, TTY_GetSX());
                            ESC_INFO("Setting cy to %d (Current: %d)", cy, TTY_GetSY_A());

                            TTY_SetSX(cx);
                            TTY_SetSY_A(cy);

                            bPendingWrap = FALSE;
                        }
                    }
                    else    // Invoke save cursor if DECLRMM is disabled
                    {
                        TF_DECSC();
                    }

                    goto EndEscape;
                }

                case 't':   // Window operations [DISPATCH] - https://terminalguide.namepad.de/seq/csi_st/
                {
                    // TODO: Make sure this only filters out nonsense "delay" sequences and not proper window operations, which should only have less than 3 parameters
                    if (ESC_ParamSeq >= 3) 
                    {
                        ESC_WARN("Skipping window operation - too many parameters (delay sequence?)");
                        goto EndEscape;
                    }
                    
                    ESC_Param[ESC_ParamSeq++] = atoi(ESC_Buffer);
                    u8 n = ESC_Param[0];
                    char str[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
                    u16 len = 0;

                    // >3 -- fix this
                    // b0: 56 - b1: 48 - b2: 0 - b3: 0
                    if (ESC_Buffer[0] == 51)   // >2
                    {
                        ESC_DEBUG("Not implemented: Set Title Mode: 2 & %u", ESC_Param[1]);

                        /*
                            0	Title is set encoded in hex.
                            1	Title is read-back encoded in hex.
                            2	Title is set in utf-8
                            3	Title is read-back encoded in utf8

                            2 & 3 = Set and read in utf-8
                        */

                        goto EndEscape;
                    }

                    switch (n)
                    {
                        case 1:     // Restore Terminal Window - "ESC[ 1 t"
                        {                         
                            ESC_INFO("Restore Terminal Window");
                            vMinimized = FALSE;
                            break;
                        }

                        case 2:     // Minimize Terminal Window - "ESC[ 2 t"
                        {
                            ESC_INFO("Minimize Terminal Window");
                            vMinimized = TRUE;
                            break;
                        }

                        case 3:     // Set Terminal Window Position - "ESC[ 3 ; Ⓝ ; Ⓝ t"   Ⓝ = posx/posy
                        {
                            if (ESC_Param[1] == 255) TermPosX = 0;
                            else TermPosX = ESC_Param[1];

                            if (ESC_Param[2] == 255) TermPosY = 0;
                            else TermPosY = ESC_Param[2];

                            ESC_INFO("Set Terminal Window Position - x: %u, y: %u", ESC_Param[1], ESC_Param[2]);
                            break;
                        }

                        case 7:     // Refresh/Redraw Terminal Window - "ESC[ 7 t"
                        {
                            ESC_ERROR("Not implemented: Refresh/Redraw Terminal Window. n = %u", n);
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

                            ESC_INFO("Set terminal size: %u x %u", ESC_Param[2], ESC_Param[1]);                        
                            break;
                        }

                        case 9:     // Maximize Terminal Window - "ESC[ 9 ; Ⓝ t"
                        case 10:    // Alias: Maximize Terminal - "ESC[ 10 ; Ⓝ t" (Does not use the Ⓝ the same way as 9, fixme)
                        {
                            vMinimized = FALSE;
                            ESC_INFO("Maximize Terminal Window - CMD = %u", ESC_Param[1]);
                            break;
                        }

                        case 11:    // Report Terminal Window State (1 = non minimized - 2 = minimized)
                        {
                            if (vMinimized) NET_SendString("\e[2t");
                            else NET_SendString("\e[1t");
                            
                            break;
                        }

                        case 13:    // Report Terminal Window Position - "ESC[ 13 ; Ⓝ t"  Ⓝ = 0/2
                        {
                            len = sprintf(str, "\e[3;%u;%ut", (u16)TermPosX, (u16)TermPosY);
                            NET_SendStringLen(str, len);
                        
                            break;
                        }

                        case 14:    // Report Terminal Window Size in Pixels - "ESC[ 14 ; Ⓝ t"  Ⓝ = 0/2
                        {
                            len = sprintf(str, "\e[4;%u;%ut", C_YMAX * 8, C_XMAX * 8);
                            NET_SendStringLen(str, len);

                            ESC_INFO("Reporting window size: %u x %u", C_XMAX * 8, C_YMAX * 8);                            
                            break;
                        }

                        case 15:    // Report Screen Size in Pixels - "ESC[ 15 t"
                        {
                            len = sprintf(str, "\e[5;%u;%ut", C_YMAX * 8, C_XMAX * 8);
                            NET_SendStringLen(str, len);

                            ESC_INFO("Reporting screen size: %u x %u", C_XMAX * 8, C_YMAX * 8);                        
                            break;
                        }

                        case 16:    // Report Cell Size in Pixels - "ESC[ 16 t"
                        {
                            NET_SendString("\e[6;8;8t");
                            break;
                        }

                        case 18:    // Report Terminal Size - "ESC[ 18 t"
                        {
                            len = sprintf(str, "\e[8;%u;%ut", C_YMAX, C_XMAX);
                            NET_SendStringLen(str, len);

                            ESC_INFO("Reporting terminal size: %u x %u", C_XMAX, C_YMAX);
                            break;
                        }

                        case 19:    // Report Screen Size - "ESC[ 19 t"
                        {
                            len = sprintf(str, "\e[9;%u;%ut", C_YMAX, C_XMAX);
                            NET_SendStringLen(str, len);

                            ESC_INFO("Reporting screen size: %u x %u", C_XMAX, C_YMAX);
                            break;
                        }

                        case 20:    // Get Icon Title - "ESC[ 20 t"
                        {
                            char lstr[128];
                            u16 llen = 0;

                            llen = sprintf(lstr, "\e]L%s\e\\", FakeIconLabel);
                            NET_SendStringLen(lstr, llen); // Reply with a fake icon string; this control sequence turned out to be a security hazard
                            break;
                        }

                        case 21:    // Get Terminal Title - "ESC[ 21 t"
                        {
                            char lstr[128];
                            u16 llen = 0;

                            llen = sprintf(lstr, "\e]l%s\e\\", FakeWindowLabel);
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

                                    ESC_INFO("Push Terminal Title \"%s\" to stack position %u", FakeWindowLabel, WindowNum-1);
                                break;

                                case 1:
                                    if (IconNum < MAX_LABEL_SSIZE)
                                    {
                                        strcpy(LabelStack[ICON_LABEL_OFFSET + IconNum], FakeIconLabel);
                                        IconNum++;
                                    }

                                    ESC_INFO("Push Terminal Icon \"%s\" to stack position %u", FakeIconLabel, IconNum-1);
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

                                    ESC_INFO("Push Terminal Title/Icon; Duplicate topmost item? n = %u", ESC_Param[1]);
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

                                    ESC_INFO("Pop Terminal Title, new title: \"%s\"", LabelStack[WindowNum]);
                                break;

                                case 1:
                                    if (IconNum > 0)
                                    {
                                        memset(LabelStack[ICON_LABEL_OFFSET + IconNum], 0, 40);
                                        IconNum--;
                                    }

                                    ESC_INFO("Pop Terminal Icon, new icon: \"%s\"", LabelStack[IconNum]);
                                break;
                            
                                default:
                                    // Remove topmost item from stack?
                                    memset(LabelStack[WindowNum], 0, 40);
                                    WindowNum--;
                                    memset(LabelStack[ICON_LABEL_OFFSET + IconNum], 0, 40);
                                    IconNum--;

                                    ESC_INFO("Pop Terminal Title/Icon; Remove topmost item from stack? n = %u", ESC_Param[1]);
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
                                ESC_WARN("Unknown Window operation [DISPATCH]; n = %u (%u %u %u %u)", n, ESC_Buffer[0], ESC_Buffer[1], ESC_Buffer[2], ESC_Buffer[3]);
                            }

                            ESC_WARN("Window operations [DISPATCH]: p1: %u, p2: %u, p3: %u, p4: %u", ESC_Param[0], ESC_Param[1], ESC_Param[2], ESC_Param[3]);
                            ESC_WARN("Window operations [DISPATCH]: b1: %u, b2: %u, b3: %u, b4: %u", ESC_Buffer[0], ESC_Buffer[1], ESC_Buffer[2], ESC_Buffer[3]);
                        
                            break;
                        }
                    }
                    
                    goto EndEscape;
                }

                DECRC:
                case 'u':   // Restore Cursor [variant] (ansi.sys) - Same as Restore Cursor (DECRC) (ESC 8)
                {
                    u8 buffer = BufferSelect == 80 ? 1 : 0;

                    ESC_INFO("Cursor restored (was saved? %s)", bCursorSaved[buffer] ? "yes" : "no");

                    if (bCursorSaved[buffer])
                    {

                        TTY_SetSX(Saved_sx[buffer]);
                        TTY_SetSY_A(Saved_sy[buffer]);

                        ESC_INFO("Restored cursor: s_sx: %d - s_sy: %d", Saved_sx[buffer], Saved_sy[buffer]);

                        vDECOM = Saved_DECOM;//FALSE;//   // xterm does not save DECOM ?
                        //DMarginTop = 0;
                        //DMarginBottom = C_YMAX;

                        vDECLRMM = FALSE;   // Test - Don't think I should touch this?

                        CharProtAttr = Saved_Prot[buffer];

                        ///bPendingWrap = FALSE;

                        TTY_MoveCursor(TTY_CURSOR_DUMMY);   // Dummy

                        DMarginTop = Saved_OrgTop[buffer];
                        DMarginBottom = Saved_OrgBottom[buffer];

                        bCursorSaved[buffer] = FALSE;
                    }
                    else
                    {
                        TTY_SetSX(0);
                        TTY_SetSY_A(0);//C_YSTART);

                        vDECOM = FALSE;
                        CharProtAttr = 0;

                        vDECLRMM = FALSE;   // Test - Don't think I should touch this?

                        TTY_MoveCursor(TTY_CURSOR_DUMMY);   // Dummy

                        DMarginTop = 0;
                        DMarginBottom = C_YMAX;
                    }

                    goto EndEscape;
                }
                
                case 'v':   // TODO: look this one up, just dummy'ing it out for now to stop it from gobbling up data...
                {
                    ESC_ERROR("Not implemented:ESC[...v");
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

                    ESC_DEBUG("x control: %u - %u %u %u %u %u %u %u - p4: %u - Type: '%c'", ESC_Param16, ESC_Param[1], ESC_Param[2], ESC_Param[3], ESC_Param[4], ESC_Param[5], ESC_Param[6], ESC_Param[7], param4, type);

                    switch (type)
                    {
                        case '*':   // Select Attribute Change Extent (DECSACE)
                        {
                            ESC_ERROR("Not implemented: Select Attribute Change Extent (DECSACE)");
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
                                ESC_DEBUG("DECFRA: Skipping fill... T:%u L:%u B:%u R:%u", top, left, bottom, right);
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

                            ESC_DEBUG("DECFRA: Top: %u, Bottom: %u, Left: %u, Right: %u - Char: '%c' - Should skip: %s", top, bottom, left, right, c, ((top > bottom) || (left > right) || (c <= 32 || (c > 127 && c <= 160) || c >= 255)) ? "True" : "False");
                            break;
                        }
                        
                        default:
                        ESC_WARN("Unknown x control character: %c", type);
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

                    ESC_DEBUG("y control: %u %u %u %u %u %u %u %u - p4: %u - Type: '%c'", ESC_Param16, ESC_Param[1], ESC_Param[2], ESC_Param[3], ESC_Param[4], ESC_Param[5], ESC_Param[6], ESC_Param[7], param4, type);

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
                                top    = (top    < DMarginTop  ? DMarginTop  : top);
                                left   = (left   < DMarginLeft ? DMarginLeft : left);
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

                                if (XTCHECKSUM == 0) len = sprintf(str, "\eP%u!~0000\e\\", pid);    // Do not negate result
                                else len = sprintf(str, "\eP%u!~10000\e\\", pid);
                            }
                            else
                            {
                                rb -= 1;

                                if (XTCHECKSUM == 0) len = sprintf(str, "\eP%u!~%04lX\e\\", pid, rb & 0xFFFF);    // Do not negate result
                                else len = sprintf(str, "\eP%u!~%04lX\e\\", pid, (~rb & 0xFFFF));
                            }

                            //NET_SendString(str);
                            NET_SendStringLen(str, len);

                            ESC_DEBUG("DECRQCRA: page: %u -- T:%u, L:%u, B:%u, R:%u -- sum: %lu (negated: $%04lX)", ESC_Param[1], top, left, bottom, right, rb, (~rb & 0x1FFFF));
                            break;
                        }

                        case '#':   // Select checksum extension (XTCHECKSUM)
                        {
                            ESC_ERROR("Not implemented: Select checksum extension (XTCHECKSUM) - Extension: %u", ESC_Param[1]);

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
                        ESC_WARN("Unknown y control character: %c", type);
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

                    ESC_DEBUG("z control: %u %u %u %u %u %u %u %u - p4: %u - Type: '%c'", ESC_Param16, ESC_Param[1], ESC_Param[2], ESC_Param[3], ESC_Param[4], ESC_Param[5], ESC_Param[6], ESC_Param[7], param4, type);

                    switch (type)
                    {
                        case '\'':   // Enable Locator Reporting (DECELR)
                        {
                            ESC_ERROR("Not implemented: Enable Locator Reporting (DECELR)");
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
                                ESC_DEBUG("DECERA: Skipping erase... T:%u L:%u B:%u R:%u", top, left, bottom, right);
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

                            ESC_DEBUG("DECERA: Top: %u, Bottom: %u, Left: %u, Right: %u", top, bottom, left, right);
                            break;
                        }
                        
                        default:
                        ESC_WARN("Unknown z control character: %c", type);
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

                    ESC_DEBUG("{ control: %u %u %u %u %u %u %u %u - p4: %u - Type: '%c'", ESC_Param16, ESC_Param[1], ESC_Param[2], ESC_Param[3], ESC_Param[4], ESC_Param[5], ESC_Param[6], ESC_Param[7], param4, type);

                    switch (type)
                    {
                        case '#':   // Save Rendition Attributes - "ESC[ [ Ⓝ ] # {"
                        {
                            ESC_ERROR("Not implemented: Save Rendition Attributes");
                            break;
                        }

                        case '\'':   // DEC Locator Select Events - "ESC[ [ Ⓝ ] ' {"
                        {
                            ESC_ERROR("Not implemented: DEC Locator Select Events");
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
                                ESC_DEBUG("DECSERA: Skipping erase... T:%u L:%u B:%u R:%u", top, left, bottom, right);
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

                            ESC_DEBUG("DECSERA: Top: %u, Bottom: %u, Left: %u, Right: %u", top, bottom, left, right);
                            break;
                        }
                        
                        default:
                        ESC_WARN("Unknown { control character: %c", type);
                        break;
                    }

                    goto EndEscape;
                }

                case '_':
                {
                    /*
                    Hmm ?
                    _: Represents the APC, which allows a string to be passed to the terminal emulator without being interpreted directly, frequently used for setting terminal titles or status bars. 
                    */
                    ESC_WARN("Unknown character in escape stream: \"%c\" (APC byte?)", byte);                    
                    goto EndEscape; // In case of weird ESC[!_ sequence just end it now...
                }
                
                case ' ':
                {
                    ESC_WARN("Unknown character in escape stream: \"%c\"", byte);                    
                    return;
                }

                case '?':
                {
                    ESC_Type = '?';
                    return;
                }

                case 0x1B:
                {
                    ESC_WARN("Ran into a rouge escape byte! Restarting escape sequence (EscSeq: %u)", ESC_BufferSeq);

                    // Reinsert byte into stream again, stop escape parser and rerun
                    RestartByte = byte;
                    goto Restart;
                    return;
                }

                default:
                    if ((byte >= 65) && (byte <= 122)) 
                    {
                        ESC_WARN("Unhandled $%X - u8: %u - char: '%c' - EscType: '%c' (EscSeq: %u)", byte, byte, (char)byte, (char)ESC_Type, ESC_BufferSeq);
                        goto EndEscape;
                    }
                break;
            }

            // leading zeroes
            if ((ESC_BufferSeq == 1) && (ESC_Buffer[0] == '0') && (byte == '0')) 
            {
                ESC_BufferSeq = 0;
            }
            
            ESC_INFO("Adding byte <$%X> (%c) to ESC_Buffer @ pos %u", (char)byte, (char)byte, ESC_BufferSeq);
            ESC_Buffer[ESC_BufferSeq++] = (char)byte;
            
            return;
        }
    
        case ']':   // Operating System Command (OSC)
        {
            //kprintf("Byte at $%lX: %u (%c)", RXBytes-1, byte, byte);

            // Todo: Fix this; "\e]11;#ffffff\e\\\e_xyz\e\\A"
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
                    
                    OSC_INFO("Got OSC %u (OSC type = $%X)", OSC_Type, OSC_Type);

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
                    OSC_INFO("Skipping <ESC> - STRINGPARSE");

                    return;
                }
                else if (byte == '\\' || byte == 7)
                {
                    OSC_String[ESC_OSCSeq] = '\0';   // -1 will be $1B Escape character, remove it

                    OSC_INFO("Got string end $%X (OSC_String = \"%s\") - STRINGPARSE", byte, OSC_String);

                    ESC_OSCSeq = 0;
                    bOSC_GetString = FALSE;
                    bOSC_Parse = TRUE;
                }
                else
                {
                    OSC_String[ESC_OSCSeq++] = byte;
                    if (ESC_OSCSeq >= 128) ESC_OSCSeq--; // Dumb cap at 128 characters

                    return;
                }
            }

            // Decide what to do with OSC type before ending OSC sequence
            if (bOSC_Parse)
            {
                OSC_INFO("OSC PARSE");

                switch (OSC_Type)
                {
                    case 0: // Set Window Title and Icon Name
                        ChangeTitle(OSC_String);

                        strncpy(FakeWindowLabel, OSC_String, 128);
                        strncpy(FakeIconLabel, OSC_String, 39);

                        OSC_INFO("Changed title/icon to \"%s\" - PARSE", OSC_String);
                    break;

                    case 1: // Change Icon Name
                        strncpy(FakeIconLabel, OSC_String, 39);

                        OSC_INFO("Changed icon to \"%s\" - PARSE", OSC_String);
                    break;

                    case 2: // Change Window title
                        ChangeTitle(OSC_String);

                        strncpy(FakeWindowLabel, OSC_String, 128);

                        OSC_INFO("Changed title to \"%s\" - PARSE", OSC_String);
                    break;

                    case 4: // Change/Read palette color
                        if (OSC_String[strlen(OSC_String) - 1] == '?') //(strcmp(OSC_String, "?"))
                        {
                            char str[16];
                            u16 len = 0;
                            memset(str, 0, 16);
                            
                            len = sprintf(str, "\e]4;1;rgb:%04X/%04X/%04X\e\\", 0, 0, 0);   // rgb:%04x/%04x/%04x
                            NET_SendStringLen(str, len);

                            OSC_ERROR("Not implemented: Read palette color \"%s\" - PARSE", OSC_String);
                        }
                        else
                        {
                            OSC_ERROR("Not implemented: Change palette color \"%s\" - PARSE", OSC_String);

                            u16 slen = strlen(OSC_String);
                            u16 c = 0;
                            char osc_str1[20];
                            char osc_str2[20];
                            u8 idx = 0xFF;
                            u16 colour = 0xFFFF;
                            while (c < slen)
                            {
                                if (OSC_String[c] == ';')
                                {
                                    strncat(osc_str1, OSC_String, c);
                                    strncat(osc_str2, OSC_String+c+1, slen-c-1);
                                    idx = atoi(osc_str1);
                                    colour = ColorConv_Text(osc_str2);
                                    OSC_ERROR("Multiple OSC strings! osc_str1: \"%s\" - osc_str2: \"%s\" - idx: %u - colour: $%04X", osc_str1, osc_str2, idx, colour);                                    
                                    break;
                                }
                                c++;
                            }

                            if (idx != 0xFF && colour != 0xFFFF)
                            {
                                PAL_setColor(idx, colour);
                            }
                        }
                    break;

                    case 7:
                        OSC_ERROR("Not implemented: $%X (Report Current Working Directory) - PARSE", OSC_Type);
                    break;

                    case 10: // Change/Read Special Text Default Foreground Color
                        OSC_ERROR("Not implemented: Change/Read Special Text Default Foreground Color: \"%s\" - PARSE", OSC_String);

                        // If OSC_String is given as a single character ?, then the terminal will respond with the color specification of the dynamic color at index OSC_Type (?). 
                        // If OSC_String is a valid color specification, then the dynamic color is changed according to it.

                        // Parse colour from string here
                        // TTY_SetAttribute(...);

                        if (strcmp(OSC_String, "?") == 0)
                        {
                            char str[16];
                            u16 len = 0;
                            len = sprintf(str, "\e]10;%u;rgb:%04X/%04X/%04X\e\\", 1, 0xFFFF, 0xFFFF, 0xFFFF);   // Index, R, G, B
                            NET_SendStringLen(str, len);
                            break;
                        }

                        u16 slen = strlen(OSC_String);
                        u16 c = 0;
                        char osc_str1[20] = "\0";
                        char osc_str2[20] = "\0";
                        while (c < slen)
                        {
                            if (OSC_String[c] == ';')
                            {
                                strncat(osc_str1, OSC_String, c);
                                strncat(osc_str2, OSC_String+c+1, slen-c-1);
                                OSC_ERROR("Multiple OSC strings! OSC_String for this case: \"%s\" - Passing on \"%s\" as new OSC_String", osc_str1, osc_str2);

                                // Pass OSC_String+c as the new OSC_String and move onto the next OSC number case (in this case 11)
                                // While keeping OSC_String up to c-1 as OSC_String for this OSC number case (10)

                                break;
                            }
                            c++;
                        }

                        if (osc_str1[0] != '\0')
                        {
                            strcpy(OSC_String, osc_str1);
                        }

                        // Parse colour here
                        u16 ttc = ColorConv_Text(OSC_String);
                        if (ttc != 0xFFFF)
                        {
                            OSC_ERROR("Got colour $%04X from string \"%s\"", ttc, OSC_String);

                            // Set dynamic colour index to ttc here

                            break;
                        }

                        // If multiple osc strings then goto DoOSC11
                        if (osc_str2[0] != '\0')
                        {
                            strcpy(OSC_String, osc_str2);
                            goto DoOSC11;
                        }
                    break;

                    DoOSC11:
                    case 11: // Change/Read Special Text Default Background Color
                        OSC_ERROR("Not implemented: Change/Read Special Text Default Background Color: \"%s\" - PARSE", OSC_String);

                        // Do the same text splitting and goto here that is in OSC10
                    break;

                    case 14: // Change/Read Pointer Mask Color
                        OSC_ERROR("Not implemented: Change/Read Pointer Mask Color: \"%s\" - PARSE", OSC_String);
                    break;

                    case 92: // String end marker
                        OSC_INFO("String end marker - String: \"%s\" - PARSE", OSC_String);
                    break;

                    case 104: // Reset Palette Colors
                        OSC_ERROR("Not implemented: Reset Palette Colors - String: \"%s\" - PARSE", OSC_String);
                    break;

                    default:
                        OSC_ERROR("Unknown OSC: $%X at $%lX - PARSE", OSC_Type, RXBytes-1);
                    break;
                }

                OSC_INFO("Ending OSC");

                ESC_OSCBuffer[0] = '\0';
                ESC_OSCBuffer[1] = '\0';
                ESC_OSCSeq = 0;
                memset(OSC_String, 0, 128);
                bOSC_Parse = FALSE;
                bOSC_GetType = TRUE;
                goto EndEscape;
            }

            // Determine what to do with the following incomming bytes depending on what type of OSC was received
            switch (OSC_Type)
            {
                case 0:   // Change Window title and Icon
                case 2:   // Change Window title
                    OSC_INFO("Change Window title (OSC type = $%X) - TYPE", OSC_Type);

                    ESC_OSCSeq = 0;
                    bOSC_GetString = TRUE;
                break;

                case 4:   // Change/Read palette color
                    OSC_INFO("Change/Read palette color (OSC type = $%X) - TYPE", OSC_Type);

                    ESC_OSCSeq = 0;
                    bOSC_GetString = TRUE;
                break;

                case 7:
                    OSC_INFO("Got end marker $7 (OSC_String = \"%s\") - TYPE", OSC_String);

                    ESC_OSCSeq = 0;
                    bOSC_GetString = FALSE;
                    bOSC_Parse = TRUE;
                break;

                case 10:  // Change/Read Special Text Default Background Color
                    OSC_INFO("Change/Read Special Text Default Foreground Color (OSC type = $%X) - TYPE", OSC_Type);

                    ESC_OSCSeq = 0;
                    bOSC_GetString = TRUE;
                break;
                
                case 11:  // Change/Read Special Text Default Background Color
                    OSC_INFO("Change/Read Special Text Default Background Color (OSC type = $%X) - TYPE", OSC_Type);

                    ESC_OSCSeq = 0;
                    bOSC_GetString = TRUE;
                break;

                case '\\':  // Hack for String Terminator (Remove escape $1B from OSC_String!)
                    OSC_String[ESC_OSCSeq-1] = '\0';   // -1 will be $1B Escape character, remove it

                    OSC_INFO("Got end marker $1B $5C (OSC_String = \"%s\") - TYPE", OSC_String);

                    ESC_OSCSeq = 0;
                    bOSC_GetString = FALSE;
                    bOSC_Parse = TRUE;
                break;
                
                case 104:  // Reset Palette Colors
                    OSC_INFO("Reset Palette Colors (OSC type = $%X) - TYPE", OSC_Type);

                    ESC_OSCSeq = 0;
                    bOSC_GetString = TRUE;
                break;
            
                default:
                {
                    OSC_ERROR("Unknown OSC: $%X - TYPE", OSC_Type);
                    
                    ESC_OSCBuffer[0] = '\0';
                    ESC_OSCBuffer[1] = '\0';
                    ESC_OSCSeq = 0;
                    memset(OSC_String, 0, 128);
                    bOSC_Parse = FALSE;
                    bOSC_GetType = TRUE;
                    goto EndEscape;
                }
            }

            return;
        }

        case '_':   // TEMP
        {
            ESC_WARN("Got a stray \"ESC _\" ... Which is probably apart of a previous OSC");
            goto EndEscape;
        }

        case '(':   // G0 charset
        {
            switch (byte)
            {
                case '0':   // DEC Special Character and Line Drawing Set
                {
                    CharMapSelection = 1;
                    ESC_INFO("ESC%c0: DEC Special Character and Line Drawing Set", ESC_Type);
                    break;
                }
                
                case 'B':
                {
                    CharMapSelection = 0;
                    ESC_INFO("ESC%cB: United States (USASCII), VT100", ESC_Type);
                    break;
                }

                default:
                {
                    ESC_ERROR("Not implemented: G0 charset $%X ESC ( %c", byte, (char)byte);
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
                    ESC_INFO("ESC%c0: ...", ESC_Type);
                    break;
                }

                default:
                {
                    ESC_ERROR("Not implemented: G1 charset $%X ESC ) %c", byte, (char)byte);
                }
            }

            goto EndEscape;
        }

        case '*':   // G2 charset
        {
            ESC_ERROR("Not implemented: G2 charset $%X ESC * %c", byte, (char)byte);
            goto EndEscape;
        }

        case '+':   // G3 charset
        {
            ESC_ERROR("Not implemented: G3 charset $%X ESC + %c", byte, (char)byte);
            goto EndEscape;
        }

        case '-':   // G1 charset
        {
            ESC_ERROR("Not implemented: G1 charset $%X ESC - %c", byte, (char)byte);
            goto EndEscape;
        }

        case '.':   // G2 charset
        {
            ESC_ERROR("Not implemented: G2 charset $%X ESC . %c", byte, (char)byte);
            goto EndEscape;
        }

        case '/':   // G3 charset
        {
            ESC_ERROR("Not implemented: G3 charset $%X ESC / %c", byte, (char)byte);
            goto EndEscape;
        }

        case '?':   // Mode set
        {
            if (byte == 'h')
            {
                if (ESC_QSeqMulti)
                {
                    ESC_ERROR("Not implemented: multi mode setting \"%s\" (enable)", (char*)ESC_QBuffer);
                    goto EndEscape;
                }
                else QSeqNumber = atoi16((char*)ESC_QBuffer);
                //kprintf("QSeqNumber = %u", QSeqNumber);

                switch (QSeqNumber)
                {
                    case 1:     // Switched Cursor Key Format (DECCKM)
                        vDECCKM = TRUE;
                    break;

                    case 40:    // Enable Support for Mode ?3 (132COLS)
                    case 3:     // 132 Column Mode (DECCOLM)
                        ESC_WARN("Not implemented: Enabling 132 column mode");
                    break;
                
                    case 4:    // Insert Mode (IRM)
                        bInsertMode = TRUE;
                        ESC_INFO("Enabling insert mode");
                    break;
                
                    case 5:    // Reverse Display Colors (DECSCNM)
                        bReverseColour = TRUE;
                        ESC_INFO("Enabling reverse colour");
                    break;

                    case 6:     // Origin Mode (DECOM), VT100.
                        vDECOM = TRUE;

                        TTY_SetSX(DMarginLeft);
                        TTY_SetSY_A(DMarginTop);

                        ESC_INFO("Enabling DECOM (Left: %d - Top: %d)", DMarginLeft, DMarginTop);
                    break;
                
                    case 7:     // Auto-Wrap Mode (DECAWM), VT100.
                        bWrapMode = TRUE;
                        ESC_INFO("Enabling Auto-Wrap mode (%s)", bWrapMode?"TRUE":"FALSE");
                    break;
                
                    case 8:    // Repeat Held Keys
                        ESC_ERROR("Not implemented: Repeat Held Keys enable (?8h)");
                    break;
                
                    case 9:     // Mouse Click-Only Tracking (X10_MOUSE)
                        MTrackMode = MT_ClickOnly;
                        ESC_INFO("Enabling Mouse Click-Only Tracking (X10_MOUSE)");
                    break;

                    case 12:    // Cursor Blinking ON (ATT610_BLINK)
                        bDoCursorBlink = TRUE;
                    break;
                
                    case 25:    // Shows the cursor, from the VT220. (DECTCEM)
                        if (TTY_GetFont()) LastCursor = 0x13;
                        else         LastCursor = 0x10;

                        SetSprite_TILE(SPRITE_CURSOR, LastCursor);
                        ESC_INFO("Showing cursor (DECTCEM)");
                    break;
                
                    case 41:    // XTerm more(1) workaround
                        bMoreWorkaround = TRUE;
                        ESC_INFO("Enabling XTerm more(1) workaround (%s)", bMoreWorkaround?"TRUE":"FALSE");
                    break;

                    case 45:    // Reverse Wrap Mode (REVERSEWRAP)
                        bReverseWrap = TRUE;
                        ESC_INFO("Enabling Reverse Wrap Mode (%s)", bReverseWrap?"TRUE":"FALSE");
                    break;

                    case 69:    // DECSLRM can set margins.
                        vDECLRMM = TRUE;
                    break;

                    case 1004:  // Report Focus Change
                        // When the terminal gains focus emit:  ESC [ I 
                        // When the terminal looses focus emit: ESC [ O
                        // vte: Sends current focus state on mode activation.

                        if (!WinMgr_isWindowOpen() && !vMinimized) NET_SendString("\e[I");
                        else NET_SendString("\e[O");
                    break;

                    case 1000:  // Mouse Down+Up Tracking
                        MTrackMode = MT_DownUp;
                        ESC_INFO("Enabling Mouse Down+Up Tracking");
                    break;
                    
                    case 1001:  // Mouse Highlight Mode
                        MTrackMode = MT_HighLight;
                        ESC_INFO("Enabling Mouse Highlight Mode");
                    break;
                    
                    case 1002:  // Mouse Click and Dragging Tracking
                        MTrackMode = MT_ClickDrag;
                        ESC_INFO("Enabling Mouse Click and Dragging Tracking");
                    break;

                    case 1003:  // Mouse Tracking with Movement
                        MTrackMode = MT_Movement;
                        ESC_INFO("Enabling Mouse Tracking with Movement");
                    break;

                    case 1005:  // Mouse Report Format multibyte
                        MReportFormat = MR_Multibyte;
                        ESC_INFO("Enabling Mouse Report Format multibyte");
                    break;

                    case 1006:  // Mouse Reporting Format Digits
                        MReportFormat = MR_Digits;
                        ESC_INFO("Enabling Mouse Reporting Format Digits");
                    break;

                    case 1015:  // Mouse Reporting Format URXVT
                        MReportFormat = MR_URXVT;
                        ESC_INFO("Enabling Mouse Reporting Format URXVT");
                    break;
                
                    case 1045:    // Extended Reverse Wrap Mode
                        bExtReverseWrap = TRUE;
                        ESC_INFO("Enabling Extended Reverse Wrap Mode (%s)", bExtReverseWrap?"TRUE":"FALSE");
                    break;

                    case 47:    // Alternate Screen Buffer (ALTBUF) (ON)
                    case 1047:  // Alternate Screen Buffer, With Clear on Exit
                    case 1049:  // Alternate Screen Buffer, With Cursor Save and Clear on Enter
                        if (BufferSelect == 0)
                        {
                            // Set HScroll to alternate buffer ->
                            if (!TTY_GetFont())
                            {
                                VDP_setHorizontalScroll(BG_A, HScroll-320);
                                VDP_setHorizontalScroll(BG_B, HScroll-320);

                                BufferSelect = 40;
                            }
                            else if (TTY_GetFont() == FONT_SOFTWARE)
                            {
                                VDP_setHorizontalScroll(BG_A, HScroll);
                                VDP_setHorizontalScroll(BG_B, HScroll);

                                BufferSelect = 80;
                            }
                            else
                            {
                                VDP_setHorizontalScroll(BG_A, (HScroll+4-320));
                                VDP_setHorizontalScroll(BG_B, (HScroll-320));

                                BufferSelect = 80;
                            }

                            SW_SetBuffer();

                            // Set VScroll to 0
                            Saved_VScroll = TTY_GetVScroll();
                            TTY_ResetVScroll();

                            switch (QSeqNumber)
                            {
                                case 1049:                                
                                    // Save cursor position from main buffer
                                    Saved_sx[0] = TTY_GetSX();
                                    Saved_sy[0] = TTY_GetSY_A();

                                    // Restore cursor position to alternate buffer
                                    TTY_SetSX(Saved_sx[1]);
                                    TTY_SetSY_A(Saved_sy[1]);

                                    // Clear alternate buffer
                                    if (TTY_GetFont() == FONT_SOFTWARE)
                                    {
                                        SW_ClearScreen();
                                    }
                                    else
                                    {
                                        VDP_clearTileMapRect(BG_A, 40, 0, 40, 30);
                                        VDP_clearTileMapRect(BG_B, 40, 0, 40, 30);
                                    }
                                break;
                            
                                default:
                                break;
                            }

                            ESC_INFO("Alternative screen buffer ON (%uh)", QSeqNumber);
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
                    ESC_ERROR("Not implemented: mode ?%uh", QSeqNumber);
                    //ESC_ERROR("0=%c 1= %c - 2= %c - 3= %c", ESC_QBuffer[0], ESC_QBuffer[1], ESC_QBuffer[2], ESC_QBuffer[3]);
                    //ESC_ERROR("0=%c%c - 2= %c%c", ESC_QBuffer[0], ESC_QBuffer[1], ESC_QBuffer[3], ESC_QBuffer[4]);
                    break;
                }

                goto EndEscape;
            }

            if (byte == 'l')
            {
                if (ESC_QSeqMulti)
                {
                    ESC_ERROR("Not implemented: multi mode setting \"%s\" (disable)", (char*)ESC_QBuffer);
                    goto EndEscape;
                }
                else QSeqNumber = atoi16((char*)ESC_QBuffer);
                //kprintf("QSeqNumber = %u", QSeqNumber);

                switch (QSeqNumber)
                {
                    case 1:     // Normal Cursor Key Format (DECCKM)
                        vDECCKM = FALSE;
                    break;

                    case 40:    // Enable Support for Mode ?3 (132COLS)
                    case 3:     // 132 Column Mode (DECCOLM)
                        ESC_WARN("Not implemented: Disabling 132 column mode");
                    break;
                
                    case 4:    // Insert Mode (IRM)
                        bInsertMode = FALSE;
                        ESC_INFO("Disabling insert mode");
                    break;
                
                    case 5:    // Reverse Display Colors (DECSCNM)
                        bReverseColour = FALSE;
                        ESC_INFO("Disabling reverse colour");
                    break;

                    case 6:     // Normal Cursor Mode (DECOM), VT100.
                        vDECOM = FALSE;
                        ESC_INFO("Disabling DECOM");
                    break;
                
                    case 7:     // No Auto-Wrap Mode (DECAWM), VT100.
                        bWrapMode = FALSE;
                        ESC_INFO("Disabling Auto-Wrap mode");
                    break;
                
                    case 8:    // Repeat Held Keys
                        ESC_ERROR("Not implemented: Repeat Held Keys disable (?8l)");
                    break;

                    case 12:    // Cursor Blinking OFF (ATT610_BLINK)
                        bDoCursorBlink = FALSE;
                        
                        if (TTY_GetFont()) LastCursor = 0x13;
                        else         LastCursor = 0x10;

                        SetSprite_TILE(SPRITE_CURSOR, LastCursor);
                    break;
                
                    case 25:    // Hides the cursor. (DECTCEM)
                        SetSprite_TILE(SPRITE_CURSOR, 0x16);

                        LastCursor = 0x16;

                        ESC_INFO("Hiding cursor (DECTCEM)");
                    break;
                
                    case 41:    // XTerm more(1) workaround
                        bMoreWorkaround = FALSE;
                        ESC_INFO("Disabling XTerm more(1) workaround (%s)", bMoreWorkaround?"TRUE":"FALSE");
                    break;

                    case 45:    // Reverse Wrap Mode (REVERSEWRAP)
                        bReverseWrap = FALSE;
                        ESC_INFO("Disabling Reverse Wrap Mode (%s)", bReverseWrap?"TRUE":"FALSE");
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
                        if (MTrackMode == MT_ClickOnly) MTrackMode = MT_None;

                        ESC_INFO("Disabling Mouse Click-Only Tracking (X10_MOUSE)");
                    break;
                    
                    case 1000:  // Mouse Down+Up Tracking
                        if (MTrackMode == MT_DownUp) MTrackMode = MT_None;

                        ESC_INFO("Disabling Mouse Down+Up Tracking");
                    break;
                    
                    case 1001:  // Mouse Highlight Mode
                        if (MTrackMode == MT_HighLight) MTrackMode = MT_None;

                        ESC_INFO("Disabling Mouse Highlight Mode");
                    break;
                    
                    case 1002:  // Mouse Click and Dragging Tracking
                        if (MTrackMode == MT_ClickDrag) MTrackMode = MT_None;

                        ESC_INFO("Disabling Mouse Click and Dragging Tracking");
                    break;

                    case 1003:  // Mouse Tracking with Movement
                        if (MTrackMode == MT_Movement) MTrackMode = MT_None;

                        ESC_INFO("Disabling Mouse Tracking with Movement");
                    break;

                    case 1005:  // Mouse Report Format multibyte
                        if (MReportFormat == MR_Multibyte) MReportFormat = MR_Default;

                        ESC_INFO("Disabling Mouse Report Format multibyte");
                    break;

                    case 1006:  // Mouse Reporting Format Digits
                        if (MReportFormat == MR_Digits) MReportFormat = MR_Default;

                        ESC_INFO("Disabling Mouse Reporting Format Digits");
                    break;

                    case 1015:  // Mouse Reporting Format URXVT
                        if (MReportFormat == MR_URXVT) MReportFormat = MR_Default;

                        ESC_INFO("Disabling Mouse Reporting Format URXVT");
                    break;

                    case 1045:   // Extended Reverse Wrap Mode
                        bExtReverseWrap = FALSE;

                        ESC_INFO("Disabling Extended Reverse Wrap Mode (%s)", bExtReverseWrap?"TRUE":"FALSE");
                    break;

                    case 47:    // Alternate Screen Buffer (ALTBUF) (OFF)
                    case 1047:  // Alternate Screen Buffer, With Clear on Exit
                    case 1049:  // Alternate Screen Buffer, With Cursor Save and Clear on Enter
                        if (BufferSelect != 0)
                        {
                            BufferSelect = 0;

                            // Set HScroll to main buffer <-
                            if (!TTY_GetFont())
                            {
                                VDP_setHorizontalScroll(BG_A, HScroll);
                                VDP_setHorizontalScroll(BG_B, HScroll);
                            }
                            else if (TTY_GetFont() == FONT_SOFTWARE)
                            {
                                VDP_setHorizontalScroll(BG_A, HScroll);
                                VDP_setHorizontalScroll(BG_B, HScroll);
                            }
                            else
                            {
                                VDP_setHorizontalScroll(BG_A, (HScroll+4));
                                VDP_setHorizontalScroll(BG_B, (HScroll  ));
                            }
                            
                            SW_SetBuffer();

                            // Set VScroll to main buffer vscroll
                            TTY_SetVScrollAbs(Saved_VScroll);

                            switch (QSeqNumber)
                            {
                                case 1047:
                                    // Clear alternate buffer
                                    if (TTY_GetFont() == FONT_SOFTWARE)
                                    {
                                        SW_ClearScreen();
                                    }
                                    else
                                    {
                                        VDP_clearTileMapRect(BG_A, 40, 0, 40, 30);
                                        VDP_clearTileMapRect(BG_B, 40, 0, 40, 30);
                                    }
                                break;

                                case 1049:
                                    // Clear alternate buffer
                                    if (TTY_GetFont() == FONT_SOFTWARE)
                                    {
                                        SW_ClearScreen();
                                    }
                                    else
                                    {
                                        VDP_clearTileMapRect(BG_A, 40, 0, 40, 30);
                                        VDP_clearTileMapRect(BG_B, 40, 0, 40, 30);
                                    }

                                    // Save cursor position from alternate buffer
                                    Saved_sx[1] = TTY_GetSX();
                                    Saved_sy[1] = TTY_GetSY_A();

                                    // Restore cursor position to main buffer
                                    TTY_SetSX(Saved_sx[0]);
                                    TTY_SetSY_A(Saved_sy[0]);
                                break;
                            
                                default:
                                break;
                            }

                            ESC_INFO("Alternative screen buffer OFF (%ul)", QSeqNumber);
                        }
                    break;

                    case 2004:  // Turn off bracketed paste mode. 
                        vBracketedPaste = FALSE;
                    break;

                    default:
                    ESC_ERROR("Not implemented: mode ?%ul", QSeqNumber);
                    break;
                }

                goto EndEscape;
            }

            // Save Mode - ESC[ ? [ Ⓝ ] s
            // Todo: Save current state of mode Ⓝ
            // Only applies to modes prefixed with ?
            if (byte == 's')
            {
                QSeqNumber = atoi16((char*)ESC_QBuffer);

                ESC_ERROR("Not implemented: mode save - ?%u%c", QSeqNumber, byte);
                goto EndEscape;
            }

            // Restore Mode - ESC[ ? [ Ⓝ ] r
            // Todo: Restore saved state of mode Ⓝ
            // Only applies to modes prefixed with ?
            if (byte == 'r')
            {
                QSeqNumber = atoi16((char*)ESC_QBuffer);

                ESC_ERROR("Not implemented: mode restore - ?%u%c", QSeqNumber, byte);
                goto EndEscape;
            }
            
            if (byte == ';') ESC_QSeqMulti = ESC_QSeq;
            else 
            {
                ESC_QBuffer[ESC_QSeq] = byte;
                //ESC_DEBUG("ESC_Q: $%X - '%c'", ESC_QBuffer[ESC_QSeq-1], (char)ESC_QBuffer[ESC_QSeq-1]);

                if (ESC_QSeq > 5) 
                {
                    //for (u8 i = 0; i < 6; i++) ESC_DEBUG("ESC_QBuffer[%u] = $%X (%c)", i, ESC_QBuffer[i], (char)ESC_QBuffer[i]);
                    goto EndEscape;
                }

                ESC_QSeq++;
            }

            return;
        }
        
        default:
            if (ESC_Seq == 1)
            {
                ESC_Type = byte;

                switch (ESC_Type)
                {
                    case 0x0D:   // ??? Unknown ESC sequence
                    case 0x1B:   // ??? Unknown ESC ESC sequence
                        ESC_WARN("Ran into unknown ESC<$%X> sequence - Reinserting byte <$%X> into stream", ESC_Type, byte);

                        // Reinsert byte into stream again, stop escape parser and rerun
                        RestartByte = byte;
                        goto Restart;
                        return;
                    break;

                    case '#':   // ESC # <num>
                    {
                        SpecialCharacter = '#';
                        NextByte = NC_SpecialByte;

                        ESC_INFO("Got \"ESC # <num>\" Awaiting next byte...");
                        return;
                    }

                    case '%':   // ESC % <8/G/@>
                    {
                        SpecialCharacter = '%';
                        NextByte = NC_SpecialByte;

                        ESC_INFO("Got \"ESC %% <8/G/@>\" Awaiting next byte...");
                        return;
                    }

                    case '=':   // ESC =    Application Keypad (DECKPAM).
                        ESC_ERROR("Not implemented: \"ESC =\"");
                        goto EndEscape;
                    break;

                    case '>':   // ESC >    Normal Keypad (DECKPNM), VT100.
                        ESC_ERROR("Not implemented: \"ESC >\"");
                        goto EndEscape;
                    break;

                    case '^':   // ESC ^ ... - "Used by other terminals"
                    {
                        SpecialCharacter = '^';
                        NextByte = NC_SpecialByte;

                        ESC_INFO("Got \"ESC ^\" Awaiting next byte...");
                        return;
                    }

                    case ' ':   // ESC ␣ <char/num>
                    {
                        SpecialCharacter = ' ';
                        NextByte = NC_SpecialByte;

                        ESC_INFO("Got \"ESC ␣ <char/num>\" Awaiting next byte...");
                        return;
                    }

                    case '6':   // ESC 6    Back Index (DECBI)
                        ESC_WARN("\"ESC 6\" Back Index (DECBI) - not fully implemented! (hack)");

                        // This isn't correct but w/e
                        TTY_MoveCursor(TTY_CURSOR_LEFT, 1);

                        goto EndEscape;
                    break;

                    case '7':   // Save Cursor (DECSC) (ESC 7)
                    {
                        TF_DECSC();
                        goto EndEscape;
                    }

                    case '8':   // Restore Cursor (DECRC) (ESC 8)
                    {
                        goto DECRC; // Ends escape after jump
                    }

                    case '9':   // Forward Index (DECFI) (ESC 9)
                    {
                        ESC_ERROR("Not implemented: \"ESC 9\"");
                        goto EndEscape;
                    }

                    case 'D':   // ESC D    Index (IND) - Cursor down - at bottom of region, scroll up
                    {
                        ESC_WARN("\"ESC D\" Index (IND) - not fully implemented!");

                        // TODO: Take margins into account and possible scrolling region up
                        TTY_MoveCursor(TTY_CURSOR_DOWN, 1);

                        bPendingWrap = FALSE;
                        
                        goto EndEscape;
                    }

                    case 'M':   // ESC M    Reverse Index (RI) https://terminalguide.namepad.de/seq/a_esc_cm/  (Old note: Moves cursor one line up, scrolling if needed)
                        ESC_WARN("\"ESC M\" Reverse Index (RI) - not fully implemented!");

                        if (TTY_GetSY_A() > DMarginTop) TTY_MoveCursor(TTY_CURSOR_UP, 1);
                        else
                        {
                            TTY_DrawScrollback_RI(1);
                        }

                        goto EndEscape;
                    break;

                    case 'E':   // ESC E    Next line (same as CR LF)
                    {
                        ESC_DEBUG("ESC E");

                        TTY_SetSX(0);
                        TTY_MoveCursor(TTY_CURSOR_DOWN, 1);
                        goto EndEscape;
                    }

                    case 'H':   // ESC H    Horizontal Tab Set (HTS)
                    {
                        u8 c = (TTY_GetSX() % 80);
                        HTS_Column[c] = 1;

                        ESC_DEBUG("Setting column %u as tabstop", c);
                        goto EndEscape;
                    }
                    
                    case 'N':   // ESC N    Select G2 set for next character only
                    {
                        ESC_ERROR("Not implemented: \"ESC N\" Select G2 set for next character only");
                        goto EndEscape;
                    }

                    case 'O':   // ESC O    Select G2 set for next character only
                    {
                        ESC_ERROR("Not implemented: \"ESC O\" Select G3 set for next character only");
                        goto EndEscape;
                    }

                    case 'P':   // ESC P    Device Control String
                    {
                        SpecialCharacter = 'P';
                        NextByte = NC_SpecialByte;

                        ESC_INFO("Got \"ESC P ...\" Awaiting next byte...");
                        return;
                    }

                    case 'V':   // ESC V    Start Protected Area (SPA)
                    {
                        ESC_ERROR("Not implemented: \"ESC V\"");
                        goto EndEscape;
                    }

                    case 'W':   // ESC W    End Protected Area (EPA)
                    {
                        ESC_ERROR("Not implemented: \"ESC W\"");
                        goto EndEscape;
                    }

                    case 'Z':   // ESC Z    Return Terminal ID (DECID)  -- Same as primary device attributes without parameters (DA1)
                    {
                        NET_SendString("\e[?1;2c");
                        goto EndEscape;
                    }

                    case 'c':   // RIS: Reset to initial state - Resets the device to its state after being powered on. 
                    {
                        TTY_Init(TF_ClearScreen | TF_ResetVariables);
                        goto EndEscape;
                    }
                    
                    case '\\':  // ESC \    String Terminator
                    {
                        ESC_INFO("\"ESC \\\" String Terminator");

                        ESC_OSCBuffer[0] = '\0';
                        ESC_OSCBuffer[1] = '\0';
                        ESC_OSCSeq = 0;
                        memset(OSC_String, 0, 128);
                        bOSC_Parse = FALSE;
                        bOSC_GetType = TRUE;

                        goto EndEscape;
                    }

                    case '_':   // ... Should probably check for C1_7Bit == 0 here
                    {
                        NextByte = NC_APC;

                        ESC_WARN("Skipping: ESC_ (interpreting as APC sequence)");

                        return;
                    }
                    
                    case 'k':  // ESC k - tmux-specific escape sequence to set terminal title
                    {
                        ESC_INFO("\"ESC k\" set title (tmux)");

                        //goto EndEscape;

                        bOSC_GetType = FALSE;
                        bOSC_GetString = TRUE;
                        OSC_Type = 2;
                        ESC_Type = ']';

                        return;
                    }
                
                    default:    // By default skip this round and parse it later
                        if ((ESC_Type != '[') && (ESC_Type != ']') && 
                            (ESC_Type != '(') && (ESC_Type != ')') && 
                            (ESC_Type != '*') && (ESC_Type != '+') && 
                            (ESC_Type != '-') && (ESC_Type != '.') && (ESC_Type != '/')) ESC_ERROR("\e[91;5mSkipping: ESC %c ($%X)\e[0m", ESC_Type, ESC_Type);

                        //ESC_ERROR("Skipping: ESC<$%X> (%c) at $%lX", ESC_Type, ESC_Type);
                    break;
                }

                return;
            }
        break;
    }

    return;

    EndEscape:
    {
        ResetSequence();
        return;
    }

    Restart:
    {
        ResetSequence();
        RXBytes--;
        TELNET_ParseRX(RestartByte);

        return;
    }
}

static void ResetSequence()
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

        SpecialCharacter = 0;
}

static inline __attribute__((always_inline)) void TF_DECSC()
{
        u8 buffer = BufferSelect == 80 ? 1 : 0;

        if (vDECOM)
        {
            // Checkme: what if margin left/top is greater than sx/sy ?
            Saved_sx[buffer] = TTY_GetSX() - DMarginLeft;
            Saved_sy[buffer] = TTY_GetSY_A() - DMarginTop;
        }
        else
        {
            Saved_sx[buffer] = TTY_GetSX();
            Saved_sy[buffer] = TTY_GetSY_A();
        }

        Saved_DECOM = vDECOM;
        Saved_Prot[buffer] = CharProtAttr;

        bCursorSaved[buffer] = TRUE;

        ESC_DEBUG("Saving cursor: s_sx: %d - s_sy: %d", Saved_sx[buffer], Saved_sy[buffer]);
}

static inline __attribute__((always_inline)) void TF_CUB(u8 num)
{
    TTY_MoveCursor(TTY_CURSOR_LEFT, num);
    return;
}

static inline __attribute__((always_inline)) void TF_CUP(u8 nx, u8 ny)
{
    // Bounds check (255 param missing, 0 = invalid/default)
    nx = nx == 255 ? 1 : nx;    // x
    nx = nx == 0   ? 1 : nx;
    ny = ny == 255 ? 1 : ny;    // y
    ny = ny == 0   ? 1 : ny;
    
    ESC_DEBUG("CUP - X: %u - Y: %u", nx, ny);
    
    if (vDECOM)
    {
        // Cap against margins
        nx += DMarginLeft;
        ny += DMarginTop;

        nx = nx > DMarginRight  ? DMarginRight  : nx;
        ny = ny > DMarginBottom ? DMarginBottom : ny;
    }
    else
    {
        // Cap against screen
        nx = nx > C_XMAX ? C_XMAX : nx;
        ny = ny > C_YMAX ? C_YMAX : ny;
    }

    nx--;
    ny--;

    TTY_SetSX(nx);
    TTY_SetSY_A(ny);

    TTY_MoveCursor(TTY_CURSOR_DUMMY);   // Dummy
    bPendingWrap = FALSE;

    return;
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
        IAC_INFO("InSubNeg: Byte recieved: $%X (Seq: %u)", byte, IAC_SNSeq-1);
        return;
    }

    if (byte >= 240)
    {
        IAC_Command = byte;
        IAC_INFO("Got IAC CMD: $%X", IAC_Command);
    }

    if ((IAC_Command == TC_GA) || (IAC_Command == TC_NOP) || (IAC_Command == TC_EOR) || (IAC_Command == TC_DM)) goto skipArg;

    if (byte <= 39)
    {
        IAC_Option = byte;
        IAC_INFO("Got IAC Option: $%X", IAC_Option);
    }

    if ((IAC_Command == 0) || (IAC_Option == 0xFF))
    {
        IAC_INFO("IAC_Seq < 2 - Returning without resetting IAC (Byte = $%X)", byte);
        return;
    }

    skipArg:

    switch (IAC_Command)
    {
        case TC_EOR:
        {
            IAC_INFO("Got End-Of-Record (Treating as NOP)");
            break;
        }

        case TC_SE:
        {
            IAC_InSubNegotiation = FALSE;
            IAC_INFO("End of subneg.");

            switch (IAC_SubNegotiationOption)
            {
                case TO_TERM_TYPE:
                {
                    if (IAC_SubNegotiationBytes[0] == TS_IS)
                    {
                        IAC_INFO("Got <IS TERM_TYPE> subneg. - Ignoring");
                    }
                    else if (IAC_SubNegotiationBytes[0] == TS_SEND)
                    {
                        IAC_INFO("Got <SEND TERM_TYPE> subneg.");

                        // Send "IAC SB TERMINAL-TYPE IS <some_terminal_type> IAC SE"
                        NET_SendChar(TC_IAC);
                        NET_SendChar(TC_SB);
                        NET_SendChar(TO_TERM_TYPE);
                        NET_SendChar(TS_IS);
                        NET_SendString(TermTypeList[sv_TermType]);
                        NET_SendChar(TC_IAC);
                        NET_SendChar(TC_SE);
                        
                        IAC_INFO("Response: IAC SB TERM_TYPE IS %s IAC SE", TermTypeList[sv_TermType]);
                    }
                    break;
                }

                case TO_TERM_SPEED:
                {
                    if (IAC_SubNegotiationBytes[0] == TS_IS)
                    {
                        IAC_INFO("Got <IS TERM_SPEED> subneg. - Ignoring");
                    }
                    else if (IAC_SubNegotiationBytes[0] == TS_SEND)
                    {
                        IAC_INFO("Got <SEND TERM_SPEED> subneg.");

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

                            IAC_INFO("Response: IAC SB TERMINAL-SPEED IS %s,%s IAC SE", RL_REPORT_BAUD, RL_REPORT_BAUD);
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

                            IAC_INFO("Response: IAC SB TERMINAL-SPEED IS %s,%s IAC SE", sv_Baud, sv_Baud);
                        }
                    }
                    break;
                }

                // https://datatracker.ietf.org/doc/html/rfc1116
                case TO_LINEMODE:
                {
                    if (bNoLineModeNeg) 
                    {
                        IAC_INFO("Got LineMode subnegotiation - Ignoring because server demanded to NOT negotiate this.");
                        break;
                    }

                    switch (IAC_SubNegotiationBytes[0])
                    {
                        case LM_MODE:   // IAC SB LINEMODE MODE mask IAC SE
                        {
                            IAC_INFO("Got <LINEMODE MODE = %u> subneg.", IAC_SubNegotiationBytes[1]);

                            u8 NewLM = (IAC_SubNegotiationBytes[1] & (LMSM_EDIT | LMSM_TRAPSIG));

                            if (NewLM == v_LineMode)
                            {
                                IAC_INFO("New linemode == current linemode; ignoring...");
                            }
                            else
                            {
                                v_LineMode = NewLM;

                                NET_SendChar(TC_IAC);
                                NET_SendChar(TC_SB);
                                NET_SendChar(TO_LINEMODE);
                                NET_SendChar(LM_MODE);
                                NET_SendChar((v_LineMode | LMSM_MODEACK));
                                NET_SendChar(TC_IAC);
                                NET_SendChar(TC_SE);

                                IAC_INFO("Response: IAC SB LINEMODE MODE %u IAC SE", (v_LineMode | LMSM_MODEACK));
                            }
                            break;
                        }
                        
                        case LM_FORWARDMASK:
                        IAC_INFO("Got <LINEMODE FORWARDMASK = %u> subneg. NOT IMPLEMENTED", IAC_SubNegotiationBytes[1]);
                        break;
                        
                        case LM_SLC:
                        IAC_INFO("Got <LINEMODE SLC = %u> subneg. NOT IMPLEMENTED", IAC_SubNegotiationBytes[1]);
                        break;
                        
                        default:
                        IAC_ERROR("Unhandled Linemode. case (IAC_SubNegotiationBytes[0] = $%X)", IAC_SubNegotiationBytes[0]);
                        break;
                    }
                    break;
                }

                case TO_ENV:
                {
                    if (IAC_SubNegotiationBytes[0] == TS_IS)
                    {
                        IAC_INFO("Server: IAC SB ENVIRON IS type ... [ VALUE ... ] [ type ... [ VALUE ... ] [");
                        IAC_INFO("Response: IAC SB ... IAC SE");
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

                        IAC_INFO("Server: IAC SB ENVIRON SEND [ type ... [ type ... [ ... ] ] ] IAC SE");
                        IAC_INFO("Response: IAC SB ENVIRON IS ... IAC SE");
                    }
                    else if (IAC_SubNegotiationBytes[0] == TS_INFO)
                    {
                        IAC_INFO("Server: IAC SB ENVIRON INFO type ... [ VALUE ... ] [ type ... [ VALUE ... ] [");
                        IAC_INFO("Response: IAC SB ... IAC SE");
                    }

                    break;
                }

                case TO_LOGOUT:
                {
                    IAC_INFO("Server: IAC SB LOGOUT xyz IAC SE");
                    IAC_INFO("Response: NONE - NOT IMPLEMENTED!");

                    break;
                }
            
                default:
                    IAC_ERROR("Unhandled subneg. case (IAC_SubNegotiationOption = $%X -- IAC_SubNegotiationBytes[0] = $%X)", IAC_SubNegotiationOption, IAC_SubNegotiationBytes[0]);
                break;
            }
            
            break;
        }

        case TC_NOP:
        {
            IAC_INFO("Got NOP");
            break;
        }

        case TC_GA:
        {
            if (vDoGA)
            {

            }
            // else if not vDoGA then treat this command as a NOP.

            IAC_INFO("Got Go-Ahead - vDoGA = %s", vDoGA?"TRUE":"FALSE (Treating as NOP)");
            break;
        }

        case TC_SB:
        {
            IAC_InSubNegotiation = TRUE;
            IAC_SubNegotiationOption = IAC_Option;
            IAC_INFO("Start subneg.");
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
                    
                    IAC_INFO("Server: IAC WILL TRANSMIT_BINARY - Response: IAC DONT TRANSMIT_BINARY - FULL IMPL. TODO");
                    break;
                }

                case TO_ECHO:
                    bNoEcho = TRUE;
                    IAC_INFO("Server: IAC WILL ECHO");
                break;

                case TO_SUPPRESS_GO_AHEAD:
                    vDoGA = 0;

                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_WILL);
                    NET_SendChar(TO_SUPPRESS_GO_AHEAD);

                    IAC_INFO("Server: IAC WILL SUPPRESS_GO_AHEAD - Client response: IAC WILL SUPPRESS_GO_AHEAD");
                break;

                // https://datatracker.ietf.org/doc/html/rfc859
                case TO_STATUS:
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_DONT);
                    NET_SendChar(TO_STATUS);

                    IAC_INFO("Server: IAC WILL STATUS - Client response: IAC DONT STATUS");
                break;

                case TO_END_REC:
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_DONT);
                    NET_SendChar(TO_END_REC);

                    IAC_INFO("Server: IAC WILL END_REC - Client response: IAC DONT END_REC");
                break;

                case TO_AUTH_OPTION:
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_DONT);
                    NET_SendChar(TO_AUTH_OPTION);

                    IAC_INFO("Server: IAC WILL TO_AUTH_OPTION - Client response: IAC DONT AUTH_OPTION");
                break;
                
                case TO_ENCRYPTION_OPTION:
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_DONT);
                    NET_SendChar(TO_ENCRYPTION_OPTION);

                    IAC_INFO("Server: IAC WILL TO_ENCRYPTION_OPTION - Client response: IAC DONT ENCRYPTION_OPTION");
                break;
                
                default:
                    IAC_ERROR("Unhandled TC_WILL option case (IAC_Option = $%X)", IAC_Option);
                break;
            }

            break;
        }
        
        // Do NOT send any responses in IAC WONT case!
        case TC_WONT:
        {
            switch (IAC_Option)
            {
                case TO_ECHO:
                    bNoEcho = FALSE;
                    IAC_INFO("Server: IAC WONT ECHO");
                break;

                case TO_LINEMODE:
                    v_LineMode = 0;
                    bNoLineModeNeg = TRUE;
                    
                    IAC_INFO("Server: IAC WONT LINEMODE");
                break;
                
                default:
                    IAC_ERROR("Unhandled TC_WONT option case (IAC_Option = $%X)", IAC_Option);
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
                    
                    IAC_INFO("Server: IAC DO TRANSMIT_BINARY - Response: IAC WONT TRANSMIT_BINARY - FULL IMPL. TODO");
                    break;
                }

                case TO_ECHO:
                {        
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_WILL);
                    NET_SendChar(TO_ECHO);
                    bNoEcho = TRUE;
                    IAC_INFO("Response: IAC WILL ECHO");
                    break;
                }

                // Not sure if this is the proper response?
                case TO_SUPPRESS_GO_AHEAD:
                {
                    vDoGA = 0;

                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_WILL);
                    NET_SendChar(TO_SUPPRESS_GO_AHEAD);

                    IAC_INFO("Response: IAC WILL SUPPRESS_GO_AHEAD");
                    break;
                }

                case TO_TERM_TYPE:
                {            
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_WILL);
                    NET_SendChar(TO_TERM_TYPE);

                    IAC_INFO("Response: IAC WILL TERM_TYPE");
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
                    NET_SendChar(C_XMAX); // Columns
                    NET_SendChar(0);
                    NET_SendChar(C_YMAX); // Rows
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_SE);

                    IAC_INFO("Response: IAC WILL NAWS - IAC SB 0x%04X 0x%04X IAC SE", C_XMAX, C_YMAX);
                    break;
                }

                case TO_TERM_SPEED:
                {            
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_WILL);
                    NET_SendChar(TO_TERM_SPEED);

                    IAC_INFO("Response: IAC WILL TERM_SPEED");
                    break;
                }

                // https://datatracker.ietf.org/doc/html/rfc1080
                case TO_RFLOW_CTRL:
                {            
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_WONT);
                    NET_SendChar(TO_RFLOW_CTRL);

                    IAC_INFO("Response: IAC WONT RFLOW_CTRL");
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

                    IAC_INFO("Response: IAC SB SEND-LOCATION <location> IAC SE");
                    break;
                }

                case TO_LINEMODE:
                {
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_WILL);
                    NET_SendChar(TO_LINEMODE);

                    bNoLineModeNeg = FALSE;
                    
                    IAC_INFO("Response: IAC WILL LINEMODE");
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
                        
                        IAC_INFO("Response: IAC WILL ENV");
                    }
                    else
                    {
                        NET_SendChar(TC_IAC);
                        NET_SendChar(TC_WONT);
                        NET_SendChar(TO_ENV);
                        
                        IAC_INFO("Response: IAC WONT ENV");
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
                        
                        IAC_INFO("Response: IAC WILL ENV_OP");
                    }
                    else
                    {
                        NET_SendChar(TC_IAC);
                        NET_SendChar(TC_WONT);
                        NET_SendChar(TO_ENV_OP);
                        
                        IAC_INFO("Response: IAC WONT ENV_OP");
                    }

                    break;
                }

                case TO_XDISP:
                {
                    NET_SendChar(TC_IAC);
                    NET_SendChar(TC_WONT);
                    NET_SendChar(TO_XDISP);
                    
                    IAC_INFO("Response: IAC WONT XDISP");

                    break;
                }
                
                default:
                    IAC_ERROR("Unhandled TC_DO option case (IAC_Option = $%X)", IAC_Option);
                break;
            }

            break;
        }

        // Do NOT send any responses in IAC DONT case!
        case TC_DONT:
        {
            switch (IAC_Option)
            {
                case TO_ECHO:
                    /*NET_SendChar(TC_IAC);
                    NET_SendChar(TC_WONT);
                    NET_SendChar(TO_ECHO);*/

                    bNoEcho = FALSE;

                    IAC_INFO("Server: IAC DONT ECHO");
                break;

                case TO_BIN_TRANS:
                    /*NET_SendChar(TC_IAC);
                    NET_SendChar(TC_WONT);
                    NET_SendChar(TO_BIN_TRANS);*/
                    
                    IAC_INFO("Server: IAC DONT BIN_TRANS");
                break;

                case TO_LINEMODE:
                    /*NET_SendChar(TC_IAC);
                    NET_SendChar(TC_WONT);
                    NET_SendChar(TO_LINEMODE);*/

                    v_LineMode = 0;
                    bNoLineModeNeg = TRUE;
                    
                    IAC_INFO("Server: IAC DONT LINEMODE");
                break;
                
                default:
                    IAC_ERROR("Unhandled TC_DONT option case (IAC_Option = $%X)", IAC_Option);
                break;
            }

            break;
        }

        case TC_DM:
        {
            IAC_INFO("Got IAC TC_DM");
            break;
        }
    
        default:
            IAC_ERROR("Unhandled command case (IAC_Command = $%X -- IAC_Option = $%X)", IAC_Command, IAC_Option);
        break;
    }

    IAC_INFO("Ending IAC");

    NextByte = NC_Data;
    IAC_Command = 0;
    IAC_Option = 0xFF;
    IAC_SubNegotiationOption = 0xFF;

    return;
}
