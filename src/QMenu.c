#include "QMenu.h"
#include "HexView.h"
#include "FavView.h"
#include "Terminal.h"
#include "Input.h"
#include "StateCtrl.h"
#include "Utils.h"
#include "Cursor.h"         // sv_CursorCL
#include "Network.h"        // sv_ListenPort
#include "Keyboard.h"       // sv_KeyLayout
#include "Screensaver.h"    // sv_bScreensaver
#include "UI.h"             // UI_ApplyTheme
#include "IRC.h"            // IRC_SetFontSize

#include "misc/ConfigFile.h"

// Forward decl.
void WINFN_Reset();
void WINFN_Newline();
void WINFN_BGColor();
void WINFN_FGColor();
void WINFN_TERMTYPE();
void WINFN_SERIALSPEED();
void WINFN_FONT_TERM();
void WINFN_FONT_TELNET();
void WINFN_FONT_IRC();
void WINFN_KBLayoutSel();
void WINFN_RXTXSTATS();
void WINFN_RXBUFSTATS();
void WINFN_DEBUGSEL();
void WINFN_SERIALPORTSEL();
void WINFN_DEVLISTENTRY();
void WINFN_HSCOFF();
void WINFN_Echo();
void WINFN_LineMode();
void WINFN_CUSTOM_FGCL();
void WINFN_SCREENSAVER();
void WINFN_CURSOR_CL();
void WINFN_UITHEME();
void WINFN_Backspace();
void WINFN_StartMenu();
void WINFN_WrapAtScreenEdge();
void WINFN_ShowJQMsg();

extern u8 sv_IRCFont;
extern u8 sv_TelnetFont;
extern u8 sv_TerminalFont;
extern u8 sv_ThemeUI;
extern u8 sv_ShowJoinQuitMsg;
extern u8 sv_WrapAtScreenEdge;

static struct s_menu
{
    u8 num_entries;                 // Number of entries in menu
    u8 selected_entry;              // Saved selected entry (automatic, leave at 0)
    u8 tagged_entry;                // Mark the tagged option/setting
    u8 prev_menu;                   // Previous menu to return to when going back (automatic, leave at 0)
    VoidCallback *entry_function;   // Function callback which is called when entering an entry
    VoidCallback *activate_function;// Function callback which is called when trying to enter an entry without submenu (next_menu >= 254)
    VoidCallback *exit_function;    // Function callback which is called when exiting an entry
    const char *title;              // Window title text
    u8 next_menu[8];                // Menu number selected entry leads to (255 = Do nothing/Do not tag - 254 = Do nothing/Do tag)
    char text[8][20];               // Entry text
} MainMenu[] =
{{//0
    5,
    0, 255, 0,
    NULL, WINFN_StartMenu, NULL,
    "Quick Menu",
    {1, 3, 4, 255, 13},
    {"System",
     "Settings",
     "Mega Drive settings",
     "Bookmarks",
     "Debug"}
},
{//1
    4,
    0, 255, 0,
    NULL, WINFN_Reset, NULL,
    "System",
    {255, 255, 255, 255},
    {"Return to terminal",
     "Hard reset",
     "Soft reset",
     "Save config"}
},
{//2
    3,
    0, 255, 0,
    NULL, NULL, NULL,
    "Default fonts",
    {11, 20, 27},
    {"Terminal font",
     "Telnet font",
     "IRC font"}
},
{//3
    6,
    0, 255, 0,
    NULL, NULL, NULL,
    "Settings",
    {8, 14, 2, 25, 12, 28},
    {"Colours",
     "Client settings",
     "Default fonts",
     "Screensaver",
     "Keyboard layout",
     "UI theme"}
},
{//4
    4,
    0, 255, 0,
    NULL, NULL, NULL,
    "MD Settings",
    {7, 10, 16, 19},
    {"Connected devices",
     "Serial speed",
     "Select serial port",
     "H Scroll offset"}
},
{//5
    2,
    0, 255, 0,
    NULL, WINFN_Newline, NULL,
    "Line ending",
    {254, 254},
    {"LF",
     "CRLF"}
},
{//6
    4,
    0, 255, 0,
    NULL, NULL, NULL,
    "Variables",
    {5, 21, 22, 29},
    {"Line ending",
     "Local echo",
     "Line mode",
     "Backspace key"}
},
{//7
    6,
    0, 255, 0,
    WINFN_DEVLISTENTRY, NULL, NULL,
    "Connected devices",
    {255, 255, 255, 255, 255, 255},
    {"P1:0= <?>",
     "P1:1= <?>",
     "P2:0= <?>",
     "P2:1= <?>",
     "P3:0= <?>",
     "P3:1= <?>"}
},
{//8
    4,
    0, 255, 0,
    NULL, NULL, NULL,
    "Colours",
    {17, 18, 24, 26},
    {"BG Colour",
     "4x8 Mono colour",
     "4x8 8 colour set",
     "Cursor colour"}
},
{//9
    5,
    0, 255, 0,
    NULL, WINFN_TERMTYPE, NULL,
    "Terminal type",
    {254, 254, 254, 254, 254},
    {"Xterm",
     "ANSI",
     "VT100",
     "MegaDrive",
     "Unknown"}
},
{//10
    4,
    0, 255, 0,
    NULL, WINFN_SERIALSPEED, NULL,
    "Serial speed",
    {254, 254, 254, 254},
    {"4800 Baud",
     "2400 Baud",
     "1200 Baud",
     "300 Baud"}
},
{//11
    5,
    0, 255, 0,
    NULL, WINFN_FONT_TERM, NULL,
    "Terminal font",
    {254, 254, 254, 254, 254},
    {"8x8 16 Colour",
     "8x8 16 Colour bold",
     "4x8 Mono",
     "4x8  8 Colour",
     "4x8 16 Colour NoInv"}
},
{//12
    2,
    0, 0, 0,
    NULL, WINFN_KBLayoutSel, NULL,
    "Keyboard layout",
    {254, 254},
    {"US (English)",
     "SV (Swedish)"}
},
{//13
    5,
    0, 255, 0,
    NULL, WINFN_DEBUGSEL, NULL,
    "Debug",
    {15, 23, 255, 255, 255},
    {"TX/RX stats",
     "RX Buffer stats",
     "HexView - RX",
     "HexView - TX",
     "HexView - Stdout"}
},
{//14
    2,
    0, 255, 0,
    NULL, NULL, NULL,
    "Client settings",
    {30, 31},
    {"Terminal",
     "IRC"}
},
{//15
    2,
    0, 255, 0,
    WINFN_RXTXSTATS, WINFN_RXTXSTATS, NULL,
    "TX/RX stats",
    {255, 255},
    {"TX bytes: 0",
     "RX bytes: 0"}
},
{//16
    4,
    2, 255, 0,
    NULL, WINFN_SERIALPORTSEL, NULL,
    "Select serial port",
    {254, 254, 254, 254},
    {"Disconnected",
     "Port 1",
     "Port 2",
     "Port 3"}
},
{//17
    4,
    0, 255, 0,
    NULL, WINFN_BGColor, NULL,
    "BG Colour",
    {254, 254, 254, 254},
    {"Black",
     "White",
     "Light BG+Dark FG",
     "Random"}
},
{//18
    5,
    0, 255, 0,
    NULL, WINFN_FGColor, NULL,
    "4x8 Mono colour",
    {254, 254, 254, 254, 254},
    {"Black",
     "White",
     "Amber",
     "Green",
     "Random"}
},
{//19
    5,
    0, 255, 0,
    NULL, WINFN_HSCOFF, NULL,
    "H Scroll offset",
    {254, 254, 254, 254, 254},
    {"None",
     "-8",
     "-16",
     "+8",
     "+16"}
},
{//20
    5,
    0, 255, 0,
    NULL, WINFN_FONT_TELNET, NULL,
    "Telnet font",
    {254, 254, 254, 254, 254},
    {"8x8 16 Colour",
     "8x8 16 Colour bold",
     "4x8 Mono",
     "4x8  8 Colour",
     "4x8 16 Colour NoInv"}
},
{//21
    2,
    0, 255, 0,
    NULL, WINFN_Echo, NULL,
    "Local echo",
    {254, 254},
    {"Off",
     "On"}
},
{//22
    2,
    0, 255, 0,
    NULL, WINFN_LineMode, NULL,
    "Line mode",
    {254, 254},
    {"None",
     "+Edit"}
},
{//23
    3,
    0, 255, 0,
    WINFN_RXBUFSTATS, WINFN_RXBUFSTATS, NULL,
    "RX Buffer stats",
    {255, 255, 255},
    {"Head: 0",
     "Tail: 0",
     "Free: 0"}
},
{//24
    3,
    0, 255, 0,
    NULL, WINFN_CUSTOM_FGCL, NULL,
    "4x8 8 Colour set",
    {254, 254, 254},
    {"Normal",
     "Highlighted",
     "Custom"}
},
{//25
    2,
    0, 255, 0,
    NULL, WINFN_SCREENSAVER, NULL,
    "Screensaver",
    {254, 254},
    {"Off",
     "On"}
},
{//26
    4,
    0, 255, 0,
    NULL, WINFN_CURSOR_CL, NULL,
    "Cursor colour",
    {254, 254, 254, 254},
    {"Green",
     "Black",
     "White",
     "Random"}
},
{//27
    3,
    0, 255, 0,
    NULL, WINFN_FONT_IRC, NULL,
    "IRC font",
    {254, 254, 254},
    {"8x8 16 Colour",
     "8x8 16 Colour bold",
     "4x8 Mono"}
},
{//28
    6,
    0, 255, 0,
    NULL, WINFN_UITHEME, NULL,
    "UI theme",
    {254, 254, 254, 254, 254, 254},
    {"Dark blue",
     "Dark lime",
     "Dark amber",
     "High contrast",
     "Aqua",
     "Hot pink"}
},
{//29
    2,
    0, 255, 0,
    NULL, WINFN_Backspace, NULL,
    "Backspace key",
    {254, 254},
    {"DEL",
     "^H"}
},
{//30
    2,
    0, 255, 0,
    NULL, NULL, NULL,
    "Terminal",
    {9, 6},
    {"Terminal type",
     "Variables"}
},
{//31
    2,
    0, 255, 0,
    NULL, NULL, NULL,
    "IRC",
    {32, 33},
    {"Word wrap",
     "Show join/quit msg"}
},
{//32
    2,
    0, 255, 0,
    NULL, WINFN_WrapAtScreenEdge, NULL,
    "Word wrap",
    {254, 254},
    {"Off",
     "On"}
},
{//33
    2,
    0, 255, 0,
    NULL, WINFN_ShowJQMsg, NULL,
    "Show join/quit msg",
    {254, 254},
    {"No",
     "Yes"}
}};

static const u8 QFrame[5][24] = 
{
    {0xC0, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC2},
    {0xC3, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC5},
    {0xC6, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC8},
    {0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCB},
    {0xCC, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCE},
};

static u8 SelectedIdx = 0;
static u8 MenuIdx = 0;
static const u8 MenuPosX = 1, MenuPosY = 0;
bool bShowQMenu = FALSE;
u8 sv_QBGCL  = 0;    // Selected BG colour entry in quick menu
u8 sv_QFGCL  = 1;    // Selected FG colour entry in quick menu
u8 sv_QCURCL = 0;    // Selected cursor colour entry in quick menu


void QMenu_Input()
{
    if (!bShowQMenu) return;

    if (is_KeyDown(KEY_RETURN))
    {
        EnterMenu();
    }
    if (is_KeyDown(KEY_ESCAPE))
    {
        ExitMenu();
    }
    if (is_KeyDown(KEY_UP))
    {
        UpMenu();
    }
    if (is_KeyDown(KEY_DOWN))
    {
        DownMenu();
    }
}

void SetupQItemTags()
{
    MainMenu[ 5].tagged_entry = vNewlineConv;
    MainMenu[ 9].tagged_entry = sv_TermType;
    MainMenu[12].tagged_entry = sv_KeyLayout;
    MainMenu[16].tagged_entry = sv_ListenPort;
    MainMenu[17].tagged_entry = sv_QBGCL;
    MainMenu[18].tagged_entry = sv_QFGCL;
    MainMenu[21].tagged_entry = vDoEcho;
    MainMenu[22].tagged_entry = vLineMode>1?0:vLineMode;
    MainMenu[24].tagged_entry = sv_bHighCL;
    MainMenu[25].tagged_entry = sv_bScreensaver;
    MainMenu[28].tagged_entry = sv_ThemeUI;
    MainMenu[29].tagged_entry = vBackspace;
    MainMenu[32].tagged_entry = sv_WrapAtScreenEdge;
    MainMenu[33].tagged_entry = sv_ShowJoinQuitMsg;

    switch (sv_TerminalFont)
    {
        case 0:
        if (sv_BoldFont) MainMenu[11].tagged_entry = 1;
        else MainMenu[11].tagged_entry = 0;
        break;
        case 1:
        MainMenu[11].tagged_entry = 3;
        break;
        case 2:
        MainMenu[11].tagged_entry = 2;
        break;
        case 3:
        MainMenu[11].tagged_entry = 4;
        break;

        default:
        MainMenu[11].tagged_entry = 255;
        break;
    }

    switch (sv_TelnetFont)
    {
        case 0:
        if (sv_BoldFont) MainMenu[20].tagged_entry = 1;
        else MainMenu[20].tagged_entry = 0;
        break;
        case 1:
        MainMenu[20].tagged_entry = 3;
        break;
        case 2:
        MainMenu[20].tagged_entry = 2;
        break;
        case 3:
        MainMenu[20].tagged_entry = 4;
        break;

        default:
        MainMenu[20].tagged_entry = 255;
        break;
    }

    switch (sv_IRCFont)
    {
        case 0:
        if (sv_BoldFont) MainMenu[27].tagged_entry = 1;
        else MainMenu[27].tagged_entry = 0;
        break;
        case 2:
        MainMenu[27].tagged_entry = 2;
        break;

        default:
        MainMenu[27].tagged_entry = 255;
        break;
    }

    switch (sv_Baud[0])
    {
        case '4':
        MainMenu[10].tagged_entry = 0;
        break;
        case '2':
        MainMenu[10].tagged_entry = 1;
        break;
        case '1':
        MainMenu[10].tagged_entry = 2;
        break;
        case '3':
        MainMenu[10].tagged_entry = 3;
        break;

        default:
        MainMenu[10].tagged_entry = 255;
        break;
    }
    
    switch (sv_HSOffset)
    {
        case 0:
        MainMenu[19].tagged_entry = 0;
        break;
        case -8:
        MainMenu[19].tagged_entry = 1;
        break;
        case -16:
        MainMenu[19].tagged_entry = 2;
        break;
        case 8:
        MainMenu[19].tagged_entry = 3;
        break;
        case 16:
        MainMenu[19].tagged_entry = 4;
        break;

        default:
        MainMenu[19].tagged_entry = 255;
        break;
    }
}

void DrawMenu(u8 idx)
{    
    TRM_SetWinHeight(MainMenu[idx].num_entries+5);
    TRM_ClearArea(1, 1, 23, MainMenu[idx].num_entries+4, PAL1, TRM_CLEAR_BG);

    MainMenu[MenuIdx].selected_entry = SelectedIdx;   // Mark previous menu selection entry

    MenuIdx = idx;
    SelectedIdx = MainMenu[MenuIdx].selected_entry;   // Get menu selection entry from new menu

    TRM_DrawText((char*)QFrame[0], 1, 1, PAL1);    // Draw the top of the title bar frame
    TRM_DrawText((char*)QFrame[1], 1, 2, PAL1);    // Draw the middle of the title bar frame
    TRM_DrawText((char*)QFrame[2], 1, 3, PAL1);    // Draw the bottom of the title bar frame
    TRM_DrawText((char*)QFrame[4], 1, MainMenu[MenuIdx].num_entries+4, PAL1);   // Draw the bottom of the window frame

    // Insert the menu title into the title bar
    TRM_DrawText(MainMenu[MenuIdx].title, 2, 2, PAL0);

    for (u8 i = 0; i < MainMenu[MenuIdx].num_entries; i++)
    {
        TRM_DrawText((char*)QFrame[3], 1, MenuPosY+4+i, PAL1); // Draw left/right window border around menu item text
        TRM_DrawText(MainMenu[MenuIdx].text[i], MenuPosX+2, MenuPosY+4+i, PAL1);    // Draw menu item text
    }

    // Redraw selected menu item text (highlight)
    TRM_DrawText(MainMenu[MenuIdx].text[SelectedIdx], MenuPosX+2, MenuPosY+4+SelectedIdx, PAL0);

    // Mark activated option
    if (MainMenu[MenuIdx].tagged_entry < MainMenu[MenuIdx].num_entries)
    {
        TRM_DrawChar('>', MenuPosX+1, MenuPosY+4+MainMenu[MenuIdx].tagged_entry, PAL1);
        TRM_DrawChar('<', MenuPosX+2+strlen(MainMenu[MenuIdx].text[MainMenu[MenuIdx].tagged_entry]), MenuPosY+4+MainMenu[MenuIdx].tagged_entry, PAL1);
    }
}

void EnterMenu()
{
    u8 next = MainMenu[MenuIdx].next_menu[SelectedIdx];
    if (next < 254) 
    {
        VoidCallback *func = MainMenu[next].entry_function;
        if (func != NULL) func();

        MainMenu[next].prev_menu = MenuIdx;    // Return menu, when going back
        DrawMenu(next);
    }
    else
    {
        VoidCallback *func = MainMenu[MenuIdx].activate_function;
        if (func != NULL) 
        {
            func();
            
            if (next == 254) 
            {
                MainMenu[MenuIdx].tagged_entry = SelectedIdx;
                DrawMenu(MenuIdx);
            }
        }
    }
}

void ExitMenu()
{
    VoidCallback *func = MainMenu[MenuIdx].exit_function;
    if (func != NULL) func();

    if (MainMenu[MenuIdx].prev_menu == MenuIdx) QMenu_Toggle();   // prev_menu == MenuIdx is only true when at the root menu, therefor close the window if trying to back out
    else DrawMenu(MainMenu[MenuIdx].prev_menu);
}

void UpMenu()
{
    TRM_DrawText(MainMenu[MenuIdx].text[SelectedIdx], MenuPosX+2, MenuPosY+4+SelectedIdx, PAL1);
    SelectedIdx = (SelectedIdx == 0 ? MainMenu[MenuIdx].num_entries-1 : SelectedIdx-1);
    TRM_DrawText(MainMenu[MenuIdx].text[SelectedIdx], MenuPosX+2, MenuPosY+4+SelectedIdx, PAL0);
}

void DownMenu()
{
    TRM_DrawText(MainMenu[MenuIdx].text[SelectedIdx], MenuPosX+2, MenuPosY+4+SelectedIdx, PAL1);
    SelectedIdx = (SelectedIdx == MainMenu[MenuIdx].num_entries-1 ? 0 : SelectedIdx+1);
    TRM_DrawText(MainMenu[MenuIdx].text[SelectedIdx], MenuPosX+2, MenuPosY+4+SelectedIdx, PAL0);
}

void QMenu_Toggle()
{
    if (bShowQMenu)
    {
        TRM_ResetWinParam();
    }
    else
    {
        TRM_SetWinHeight(10);
        TRM_ClearArea(0, 1, 24, 29, PAL1, TRM_CLEAR_BG);
                
        DrawMenu(0);
    }

    bShowQMenu = !bShowQMenu;
}

void ChangeText(u8 menu_idx, u8 entry_idx, const char *new_text)
{
    strncpy(MainMenu[menu_idx].text[entry_idx], new_text, 20);
}


// Callback functions

void WINFN_Reset()
{
    QMenu_Toggle();

    switch (SelectedIdx)
    {
        case 0: // Exit
            if (getState() != PS_Terminal) RevertState();
        break;
        case 1: // Hard reset
            ChangeState(PS_Dummy, 0, NULL);
            SYS_hardReset();
        break;
        case 2: // Soft reset
            SYS_reset();
        break;
        case 3: // Save config to file
            CFG_SaveData();
        break;
        case 4: // Erase config
            //CFG_ClearConfig();
        break;
    
        default:
        break;
    } 
}

void WINFN_Newline()
{    
    vNewlineConv = SelectedIdx;
}

void WINFN_BGColor()
{
    switch (SelectedIdx)
    {
        case 0:
            sv_CBGCL = 0;
        break;
        case 1:
            sv_CBGCL = 0xEEE;
        break;
        case 2:
            sv_CBGCL = 0xAAA;

            PAL_setColor(0x0D, 0x000);
            PAL_setColor(0x0C, 0x000);

            PAL_setColor(0x1D, 0x006);
            PAL_setColor(0x1C, 0x004);

            PAL_setColor(0x2D, 0x060);
            PAL_setColor(0x2C, 0x040);

            PAL_setColor(0x3D, 0x066);
            PAL_setColor(0x3C, 0x044);

            PAL_setColor(0x0F, 0x600);
            PAL_setColor(0x0E, 0x400);

            PAL_setColor(0x1F, 0x606);
            PAL_setColor(0x1E, 0x404);

            PAL_setColor(0x2F, 0x660);
            PAL_setColor(0x2E, 0x440);

            PAL_setColor(0x3F, 0x666);
            PAL_setColor(0x3E, 0x444);
        break;
        case 3:
            sv_CBGCL = random();
        break;
    
        default:
            sv_CBGCL = 0;
        break;
    }
    
    sv_QBGCL = SelectedIdx;
    PAL_setColor( 0, sv_CBGCL);
    PAL_setColor(17, sv_CBGCL);
    PAL_setColor(50, sv_CBGCL);
}

void WINFN_FGColor()
{
    if (sv_Font != FONT_4x8_1) return;
    u16 r = random();

    switch (SelectedIdx)
    {
        case 0: // Black
            sv_CursorCL = 0x0E0;
            sv_CFG0CL = 0;
            sv_CFG1CL = 0x222;
        break;
        case 1: // White
            sv_CursorCL = 0x0E0;
            sv_CFG0CL = 0xEEE;
            sv_CFG1CL = 0x666;
        break;
        case 2: // Amber
            sv_CursorCL = 0x0AE;
            sv_CFG0CL = 0x0AE;
            sv_CFG1CL = 0x046;
        break;
        case 3: // Green
            sv_CursorCL = 0x0E0;
            sv_CFG0CL = 0x0A0;
            sv_CFG1CL = 0x040;
        break;
        case 4: // Random
            sv_CursorCL = r;
            sv_CFG0CL = r;
            sv_CFG1CL = r & 0x666;
        break;
    
        default:
            sv_CursorCL = 0x0E0;
            sv_CFG0CL = 0xEEE;
            sv_CFG1CL = 0x666;
        break;
    }
    
    sv_QFGCL = SelectedIdx;
    PAL_setColor(47, sv_CFG0CL);
    PAL_setColor(46, sv_CFG0CL);
}

void WINFN_TERMTYPE()
{
    sv_TermType = SelectedIdx;
}

void WINFN_SERIALSPEED()
{
    vu8 *PSCTRL = (vu8 *)DRV_UART.SCtrl;

    switch (SelectedIdx)
    {
        case 0: // 4800
            *PSCTRL = 0x38;
            strcpy(sv_Baud, "4800");
        break;
        case 1: // 2400
            *PSCTRL = 0x78;
            strcpy(sv_Baud, "2400");
        break;
        case 2: // 1200
            *PSCTRL = 0xB8;
            strcpy(sv_Baud, "1200");
        break;
        case 3: // 300
            *PSCTRL = 0xF8;
            strcpy(sv_Baud, "300");
        break;
    
        default:
        break;
    }
}

void WINFN_FONT_TERM()
{
    switch (SelectedIdx)
    {
        case 0:
            sv_TerminalFont = 0;
            sv_BoldFont = FALSE;
        break;
        case 1:
            sv_TerminalFont = 0;
            sv_BoldFont = TRUE;
        break;
        case 2:
            sv_TerminalFont = 2;
        break;
        case 3:
            sv_TerminalFont = 1;
        break;
        case 4:
            sv_TerminalFont = 3;
        break;
    
        default:
        break;
    }

    if (getState() == PS_Terminal) 
    {
        TTY_SetFontSize(sv_TerminalFont);
        TTY_Reset(TRUE);
        printf("%s> ", FS_GetCWD());
    }
}

void WINFN_FONT_TELNET()
{
    switch (SelectedIdx)
    {
        case 0:
            sv_TelnetFont = 0;
            sv_BoldFont = FALSE;
        break;
        case 1:
            sv_TelnetFont = 0;
            sv_BoldFont = TRUE;
        break;
        case 2:
            sv_TelnetFont = 2;
        break;
        case 3:
            sv_TelnetFont = 1;
        break;
        case 4:
            sv_TelnetFont = 3;
        break;
    
        default:
        break;
    }
    
    if (getState() == PS_Telnet) TTY_SetFontSize(sv_TelnetFont);
}

void WINFN_FONT_IRC()
{
    switch (SelectedIdx)
    {
        case 0:
            sv_IRCFont = 0;
            sv_BoldFont = FALSE;
        break;
        case 1:
            sv_IRCFont = 0;
            sv_BoldFont = TRUE;
        break;
        case 2:
            sv_IRCFont = 2;
        break;
    
        default:
        break;
    }

    if (getState() == PS_IRC)
    {
        // TODO: Clear TMB buffers here!
    }
}

void WINFN_KBLayoutSel()
{
    switch (SelectedIdx)
    {
        default:
        case 0: // US (English)
            sv_KeyLayout = 0;
        break;
        case 1: // SE (Swedish)
            sv_KeyLayout = 1;
        break;
    }
}

void WINFN_RXTXSTATS()
{
    char buf1[32];
    char buf2[32];

    sprintf(buf1, "TX Bytes: %lu", TXBytes);
    sprintf(buf2, "RX Bytes: %lu", RXBytes);

    strncpy(MainMenu[15].text[0], buf1, 20);
    strncpy(MainMenu[15].text[1], buf2, 20);
}

void WINFN_RXBUFSTATS()
{
    char buf1[32];
    char buf2[32];
    char buf3[32];
    
    sprintf(buf1, "Head: %4u", RxBuffer.head);
    sprintf(buf2, "Tail: %4u", RxBuffer.tail);
    sprintf(buf3, "Free: %4u / %4u", BUFFER_LEN - (RxBuffer.tail>RxBuffer.head?(BUFFER_LEN+(s16)(RxBuffer.head-RxBuffer.tail)):(RxBuffer.head-RxBuffer.tail)), BUFFER_LEN);

    strncpy(MainMenu[23].text[0], buf1, 20);
    strncpy(MainMenu[23].text[1], buf2, 20);
    strncpy(MainMenu[23].text[2], buf3, 20);
}

void WINFN_DEBUGSEL()
{
    switch (SelectedIdx)
    {
        case 2:
            QMenu_Toggle();
            HexView_Open("/system/rxbuffer.io");
        break;
        case 3:
            QMenu_Toggle();
            HexView_Open("/system/txbuffer.io");
        break;
        case 4:
            QMenu_Toggle();
            HexView_Open("/system/stdout.io");
        break;
    
        default:
        break;
    }
}

void WINFN_SERIALPORTSEL()
{
    SetDevicePort(&DRV_UART, (DevPort)SelectedIdx);
    sv_ListenPort = (DevPort)SelectedIdx;
    
    vu8 *SCtrl;
    SCtrl = (vu8 *)DRV_UART.SCtrl;
    *SCtrl = 0x38;
}

void WINFN_DEVLISTENTRY()
{
    char buf[32];
    u8 p = 0, s = 0;
    u8 d = 0;

    for (u8 i = 0; i < DEV_MAX; i++)
    {
        if (DevList[i] == 0) continue;
        if (DevList[i]->PAssign == DP_None) continue;

        p = DevList[i]->PAssign;
        s = DevList[i]->Id.Bitshift >> 1;

        if (DevList[i]->Id.Mode == DEVMODE_PARALLEL)
        {
            sprintf(buf, "P%u:%u= %s", p, s, DevList[i]->Id.sName);
        }
        else if (DevList[i]->Id.Mode == DEVMODE_SERIAL)
        {
            sprintf(buf, "P%u:S= %s", p, DevList[i]->Id.sName);
        }
        else if (DevList[i]->Id.Mode == (DEVMODE_SERIAL | DEVMODE_PARALLEL))
        {
            sprintf(buf, "P%u:D= %s", p, DevList[i]->Id.sName);
        }

        ChangeText(7, d, buf);
        d++;
    }

    if (bMegaCD)
    {
        sprintf(buf, "EXTD= %s", bPALSystem ? "Sega CC" : "Mega CD");

        ChangeText(7, d, buf);
        d++;
    }

    MainMenu[7].num_entries = d;
}

void WINFN_HSCOFF()
{
    switch (SelectedIdx)
    {
        case 0: // None
            sv_HSOffset = 0;
        break;
        case 1: // -8
            sv_HSOffset = -8;
        break;
        case 2: // -16
            sv_HSOffset = -16;
        break;
        case 3: // +8
            sv_HSOffset = 8;
        break;
        case 4: // +16
            sv_HSOffset = 16;
        break;
    
        default:
        break;
    }

    HScroll = sv_HSOffset;
    if (!sv_Font)
    {
        VDP_setHorizontalScroll(BG_A, HScroll);
        VDP_setHorizontalScroll(BG_B, HScroll);
    }
    else
    {
        VDP_setHorizontalScroll(BG_A, (HScroll+4));  // -4
        VDP_setHorizontalScroll(BG_B, (HScroll  ));  // -8
    }

    if (getState() != PS_IRC) TTY_MoveCursor(TTY_CURSOR_DUMMY);
}

void WINFN_Echo()
{    
    vDoEcho = SelectedIdx;
}

void WINFN_LineMode()
{    
    vLineMode = SelectedIdx;
}

void WINFN_CUSTOM_FGCL()
{
    switch (SelectedIdx)
    {
        case 0: // Normal
            sv_bHighCL = FALSE;
        break;
        case 1: // Highlighted
            sv_bHighCL = TRUE;
        break;
        case 2: // Custom
        break;
    
        default:
        break;
    }

    TTY_ReloadPalette();
}

void WINFN_SCREENSAVER()
{
    sv_bScreensaver = SelectedIdx;
}

void WINFN_CURSOR_CL()
{
    switch (SelectedIdx)
    {
        case 0:
            sv_CursorCL = 0x0E0;
        break;
        case 1:
            sv_CursorCL = 0;
        break;
        case 2:
            sv_CursorCL = 0xEEE;
        break;
        case 3:
            sv_CursorCL = random();
        break;
    
        default:
            sv_CursorCL = 0;
        break;
    }
    
    sv_QCURCL = SelectedIdx;
    PAL_setColor(4, sv_CursorCL);
}

void WINFN_UITHEME()
{
    sv_ThemeUI = SelectedIdx;
    UI_ApplyTheme();
}

void WINFN_Backspace()
{
    vBackspace = SelectedIdx;
}

void WINFN_StartMenu()
{
    switch (SelectedIdx)
    {
        case 3:
            QMenu_Toggle();
            FavView_Toggle();
        break;
    
        default:
        break;
    }
}

void WINFN_WrapAtScreenEdge()
{
    switch (SelectedIdx)
    {
        case 0:
            sv_WrapAtScreenEdge = 0;
        break;
    
        default:
            sv_WrapAtScreenEdge = 1;
        break;
    }
}

void WINFN_ShowJQMsg()
{
    switch (SelectedIdx)
    {
        case 0:
            sv_ShowJoinQuitMsg = 0;
        break;
    
        default:
            sv_ShowJoinQuitMsg = 1;
        break;
    }
}
