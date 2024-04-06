
#include "QMenu.h"
#include "HexView.h"
#include "Terminal.h"
#include "Input.h"
#include "StateCtrl.h"
#include "Utils.h"
#include "SRAM.h"
#include "Network.h"

extern SM_Device DEV_UART;
extern u8 vKBLayout;        // Selected keyboard layout

void WINFN_Reset();
void WINFN_Newline();
void WINFN_BGColor();
void WINFN_FGColor();
void WINFN_TERMTYPE();
void WINFN_SERIALSPEED();
void WINFN_FONTSIZE();
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
    NULL, NULL, NULL,
    "QUICK MENU",
    {1, 3, 4, 13, 255},
    {"RESET",
     "TERMINAL SETTINGS",
     "MEGA DRIVE SETTINGS",
     "DEBUG",
     "ABOUT"}
},
{//1
    4,
    0, 255, 0,
    NULL, WINFN_Reset, NULL,
    "RESET",
    {255, 255, 255, 255},
    {"HARD RESET",
     "SOFT RESET",
     "SAVE CONFIG TO SRAM",
     "ERASE SRAM"}
},
{//2
    2,
    0, 255, 0,
    NULL, NULL, NULL,
    "SESSION SETTINGS",
    {255, 20},
    {"TELNET",
     "IRC"}
},
{//3
    3,
    0, 255, 0,
    NULL, NULL, NULL,
    "TERMINAL SETTINGS",
    {6, 9, 11},
    {"VARIABLES",
     "TERMINAL TYPE",
     "FONT SIZE"}
},
{//4
    6,
    0, 255, 0,
    NULL, NULL, NULL,
    "MD SETTINGS",
    {7, 8, 10, 16, 19, 12},
    {"CONNECTED DEVICES",
     "COLOUR",
     "SERIAL SPEED",
     "SELECT SERIAL PORT",
     "HSCROLL OFFSET",
     "KEYBOARD LAYOUT"}
},
{//5
    2,
    0, 255, 0,
    NULL, WINFN_Newline, NULL,
    "LINE ENDING",
    {254, 254},
    {"LF",
     "CRLF"}
},
{//6
    3,
    0, 255, 0,
    NULL, NULL, NULL,
    "VARIABLES",
    {5, 21, 22},
    {"LINE ENDING",
     "LOCAL ECHO",
     "LINE MODE"}
},
{//7
    6,
    0, 255, 0,
    WINFN_DEVLISTENTRY, NULL, NULL,
    "CONNECTED DEVICES",
    {255, 255, 255, 255, 255, 255},
    {"P1:0= <?>",
     "P1:1= <?>",
     "P2:0= <?>",
     "P2:1= <?>",
     "P3:0= <?>",
     "P3:1= <?>"}
},
{//8
    3,
    0, 255, 0,
    NULL, NULL, NULL,
    "COLOUR",
    {17, 18, 24},
    {"BG COLOUR",
     "4x8 MONO COLOUR",
     "4x8 8 COLOUR SET"}
},
{//9
    5,
    0, 255, 0,
    NULL, WINFN_TERMTYPE, NULL,
    "TERMINAL TYPE",
    {254, 254, 254, 254, 254},
    {"XTERM",
     "ANSI",
     "VT100",
     "MEGADRIVE",
     "UNKNOWN"}
},
{//10
    4,
    0, 255, 0,
    NULL, WINFN_SERIALSPEED, NULL,
    "SERIAL SPEED",
    {254, 254, 254, 254},
    {"4800 BAUD",
     "2400 BAUD",
     "1200 BAUD",
     "300 BAUD"}
},
{//11
    3,
    0, 255, 0,
    NULL, WINFN_FONTSIZE, NULL,
    "FONT SIZE",
    {254, 254, 254},
    {"8x8 16 COLOUR",
     "4x8 8 COLOUR + AA",
     "4x8 MONO ANTIALIAS"}
},
{//12
    2,
    0, 0, 0,
    NULL, WINFN_KBLayoutSel, NULL,
    "KEYBOARD LAYOUT",
    {254, 254},
    {"US (ENGLISH)",
     "SV (SWEDISH)"}
},
{//13
    3,
    0, 255, 0,
    NULL, WINFN_DEBUGSEL, NULL,
    "DEBUG",
    {15, 23, 255},
    {"TX/RX STATS",
     "RX BUFFER STATS",
     "HEX VIEW - RX"}
},
{//14
    3,
    0, 255, 0,
    NULL, NULL, NULL,
    "ABOUT",
    {255, 255, 255},
    {"","",""}
},
{//15
    2,
    0, 255, 0,
    WINFN_RXTXSTATS, WINFN_RXTXSTATS, NULL,
    "TX/RX STATS",
    {255, 255},
    {"TX BYTES: 0",
     "RX BYTES: 0"}
},
{//16
    4,
    2, 255, 0,
    NULL, WINFN_SERIALPORTSEL, NULL,
    "SELECT SERIAL PORT",
    {254, 254, 254, 254},
    {"DISCONNECTED",
     "PORT 1",
     "PORT 2",
     "PORT 3"}
},
{//17
    3,
    0, 255, 0,
    NULL, WINFN_BGColor, NULL,
    "BG COLOUR",
    {254, 254, 254},
    {"BLACK",
     "WHITE",
     "RANDOM"}
},
{//18
    5,
    0, 255, 0,
    NULL, WINFN_FGColor, NULL,
    "4x8 MONO COLOUR",
    {254, 254, 254, 254, 254},
    {"BLACK",
     "WHITE",
     "AMBER",
     "GREEN",
     "RANDOM"}
},
{//19
    5,
    0, 255, 0,
    NULL, WINFN_HSCOFF, NULL,
    "HSCROLL OFFSET",
    {254, 254, 254, 254, 254},
    {"NONE",
     "-8",
     "-16",
     "+8",
     "+16"}
},
{//20
    2,
    0, 255, 0,
    NULL, NULL, NULL,
    "IRC",
    {254, 254},
    {"SET NICKNAME",
     "SET QUIT MESSAGE"}
},
{//21
    2,
    0, 255, 0,
    NULL, WINFN_Echo, NULL,
    "ECHO",
    {254, 254},
    {"OFF",
     "ON"}
},
{//22
    2,
    0, 255, 0,
    NULL, WINFN_LineMode, NULL,
    "LINEMODE",
    {254, 254},
    {"NONE",
     "+EDIT"}
},
{//23
    3,
    0, 255, 0,
    WINFN_RXBUFSTATS, WINFN_RXBUFSTATS, NULL,
    "RX BUFFER STATS",
    {255, 255, 255},
    {"HEAD: 0",
     "TAIL: 0",
     "FREE: 0"}
},
{//24
    3,
    0, 255, 0,
    NULL, WINFN_CUSTOM_FGCL, NULL,
    "4x8 8 COLOUR SET",
    {254, 254, 254},
    {"NORMAL",
     "HIGHLIGHTED",
     "CUSTOM"}
}};

static const u8 QFrame[3][24] = 
{
    {201, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 187},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {200, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 188},
};

static u8 SelectedIdx = 0;
static u8 MenuIdx = 0;
static const u8 MenuPosX = 1, MenuPosY = 0;
bool bShowQMenu = FALSE;
u8 QSelected_BGCL = 0;
u8 QSelected_FGCL = 1;

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
    MainMenu[ 9].tagged_entry = vTermType;
    MainMenu[11].tagged_entry = FontSize;
    MainMenu[12].tagged_entry = vKBLayout;
    MainMenu[16].tagged_entry = DEV_UART_PORT;
    MainMenu[17].tagged_entry = QSelected_BGCL;
    MainMenu[18].tagged_entry = QSelected_FGCL;
    MainMenu[21].tagged_entry = vDoEcho;
    MainMenu[22].tagged_entry = vLineMode>1?0:vLineMode;
    MainMenu[24].tagged_entry = bHighCL;

    switch (vSpeed[0])
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
    
    switch (D_HSCROLL)
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
    char buf[32];
    
    VDP_setWindowVPos(FALSE, MainMenu[idx].num_entries+4);
    TRM_clearTextArea(0, 0, 36, MainMenu[idx].num_entries+4);

    MainMenu[MenuIdx].selected_entry = SelectedIdx;   // Mark previous menu selection entry

    MenuIdx = idx;
    SelectedIdx = MainMenu[MenuIdx].selected_entry;   // Get menu selection entry from new menu

    TRM_drawText((char*)QFrame[0], 1, 0, PAL1);    // Draw the top of the border frame

    // Insert the menu title into the top border frame
    sprintf(buf, " %s ", MainMenu[MenuIdx].title);
    TRM_drawText(buf, 2, 0, PAL1);

    TRM_drawText((char*)QFrame[2], 1, MainMenu[MenuIdx].num_entries+3, PAL1);

    for (u8 i = 0; i < MainMenu[MenuIdx].num_entries; i++)
    {
        TRM_drawText((char*)QFrame[1], 1, MenuPosY+2+i, PAL1); // Draw left/right border around menu item text
        TRM_drawText(MainMenu[MenuIdx].text[i], MenuPosX+2, MenuPosY+2+i, PAL1);    // Draw menu item text
    }

    // Redraw selected menu item text (highlight)
    TRM_drawText(MainMenu[MenuIdx].text[SelectedIdx], MenuPosX+2, MenuPosY+2+SelectedIdx, PAL3);

    // Draw left/right border above and below menu item text
    TRM_drawText((char*)QFrame[1], 1, MenuPosY+1, PAL1);
    TRM_drawText((char*)QFrame[1], 1, MenuPosY+2+MainMenu[MenuIdx].num_entries, PAL1);

    // Mark activated option
    if ((MainMenu[MenuIdx].tagged_entry < MainMenu[MenuIdx].num_entries) )//|| (MainMenu[MenuIdx].next_menu[SelectedIdx] < 254))
    {
        TRM_drawChar('>', MenuPosX+1, MenuPosY+2+MainMenu[MenuIdx].tagged_entry, PAL1);
        TRM_drawChar('<', MenuPosX+2+strlen(MainMenu[MenuIdx].text[MainMenu[MenuIdx].tagged_entry]), MenuPosY+2+MainMenu[MenuIdx].tagged_entry, PAL1);
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
    TRM_drawText(MainMenu[MenuIdx].text[SelectedIdx], MenuPosX+2, MenuPosY+2+SelectedIdx, PAL1);
    SelectedIdx = (SelectedIdx == 0 ? MainMenu[MenuIdx].num_entries-1 : SelectedIdx-1);
    TRM_drawText(MainMenu[MenuIdx].text[SelectedIdx], MenuPosX+2, MenuPosY+2+SelectedIdx, PAL3);
}

void DownMenu()
{
    TRM_drawText(MainMenu[MenuIdx].text[SelectedIdx], MenuPosX+2, MenuPosY+2+SelectedIdx, PAL1);
    SelectedIdx = (SelectedIdx == MainMenu[MenuIdx].num_entries-1 ? 0 : SelectedIdx+1);
    TRM_drawText(MainMenu[MenuIdx].text[SelectedIdx], MenuPosX+2, MenuPosY+2+SelectedIdx, PAL3);
}

void QMenu_Toggle()
{
    if (bShowQMenu)
    {
        TRM_SetWinHeight(1);
        TRM_ResetStatusText();
        
        #ifndef NO_LOGGING
            KLog("Hiding window");
        #endif
    }
    else
    {
        TRM_SetWinHeight(10);
        TRM_clearTextArea(0, 0, 35, 1);
        TRM_clearTextArea(0, 1, 40, 29);
        DrawMenu(0);
        
        #ifndef NO_LOGGING
            KLog("Showing window");
        #endif
    }

    bShowQMenu = !bShowQMenu;
}

void ChangeText(u8 menu_idx, u8 entry_idx, const char *new_text)
{
    strncpy(MainMenu[menu_idx].text[entry_idx], new_text, 20);
}


void WINFN_Reset()
{
    QMenu_Toggle();

    switch (SelectedIdx)
    {
        case 0:
            ChangeState(PS_Dummy, 0, NULL);
            SYS_hardReset();
        break;
        case 1:
            ChangeState(getState(), 0, NULL);
            //ResetSystem(FALSE);
        break;
        case 2:
            SRAM_SaveData();
        break;
        case 3:
            SRAM_ClearSRAM();
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
            Custom_BGCL = 0;
        break;
        case 1:
            Custom_BGCL = 0xEEE;
        break;
        case 2:
            Custom_BGCL = random();
        break;
    
        default:
            Custom_BGCL = 0;
        break;
    }
    
    QSelected_BGCL = SelectedIdx;
    PAL_setColor(0, Custom_BGCL);
}

extern u16 Cursor_CL;

void WINFN_FGColor()
{
    if (FontSize != 2) return;
    u16 r = random();

    switch (SelectedIdx)
    {
        case 0: // Black
            Cursor_CL = 0x0E0;
            Custom_FG0CL = 0;
            Custom_FG1CL = 0x222;
        break;
        case 1: // White
            Cursor_CL = 0x0E0;
            Custom_FG0CL = 0xEEE;
            Custom_FG1CL = 0x666;
        break;
        case 2: // Amber
            Cursor_CL = 0x0AE;
            Custom_FG0CL = 0x0AE;
            Custom_FG1CL = 0x046;
        break;
        case 3: // Green
            Cursor_CL = 0x0E0;
            Custom_FG0CL = 0x0A0;
            Custom_FG1CL = 0x040;
        break;
        case 4: // Random
            Cursor_CL = r;
            Custom_FG0CL = r;
            Custom_FG1CL = r & 0x666;
        break;
    
        default:
            Cursor_CL = 0x0E0;
            Custom_FG0CL = 0xEEE;
            Custom_FG1CL = 0x666;
        break;
    }
    
    QSelected_FGCL = SelectedIdx;
    PAL_setColor(47, Custom_FG0CL);
    PAL_setColor(46, Custom_FG1CL);
}

void WINFN_TERMTYPE()
{
    vTermType = SelectedIdx;
}

void WINFN_SERIALSPEED()
{
    vu8 *PSCTRL = (vu8 *)DEV_UART.SCtrl;

    switch (SelectedIdx)
    {
        case 0: // 4800
            *PSCTRL = 0x38;
            strcpy(vSpeed, "4800");
        break;
        case 1: // 2400
            *PSCTRL = 0x78;
            strcpy(vSpeed, "2400");
        break;
        case 2: // 1200
            *PSCTRL = 0xB8;
            strcpy(vSpeed, "1200");
        break;
        case 3: // 300
            *PSCTRL = 0xF8;
            strcpy(vSpeed, "300");
        break;
    
        default:
        break;
    }
}

void WINFN_FONTSIZE()
{
    TTY_SetFontSize(SelectedIdx);
    ResetSystem(FALSE);
}

void WINFN_KBLayoutSel()
{
    switch (SelectedIdx)
    {
        default:
        case 0: // US (English)
            vKBLayout = 0;
        break;
        case 1: // SE (Swedish)
            vKBLayout = 1;
        break;
    }
}

void WINFN_RXTXSTATS()
{
    char buf1[32];
    char buf2[32];

    sprintf(buf1, "TX BYTES: %lu", TXBytes);
    sprintf(buf2, "RX BYTES: %lu", RXBytes);

    strncpy(MainMenu[15].text[0], buf1, 20);
    strncpy(MainMenu[15].text[1], buf2, 20);
}

void WINFN_RXBUFSTATS()
{
    char buf1[32];
    char buf2[32];
    char buf3[32];
    
    sprintf(buf1, "HEAD: %u", RxBuffer.head);
    sprintf(buf2, "TAIL: %u", RxBuffer.tail);
    sprintf(buf3, "FREE: %u / %u", BUFFER_LEN - (RxBuffer.tail>RxBuffer.head?(BUFFER_LEN+(s16)(RxBuffer.head-RxBuffer.tail)):(RxBuffer.head-RxBuffer.tail)), BUFFER_LEN);

    strncpy(MainMenu[23].text[0], buf1, 20);
    strncpy(MainMenu[23].text[1], buf2, 20);
    strncpy(MainMenu[23].text[2], buf3, 20);
}

void WINFN_DEBUGSEL()
{
    if (SelectedIdx == 2)
    {
        QMenu_Toggle();
        HexView_Toggle();
    }
}

void WINFN_SERIALPORTSEL()
{
    SetDevicePort(&DEV_UART, SelectedIdx);
    
    vu8 *SCtrl;
    SCtrl = (vu8 *)DEV_UART.SCtrl;
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
            D_HSCROLL = 0;
        break;
        case 1: // -8
            D_HSCROLL = -8;
        break;
        case 2: // -16
            D_HSCROLL = -16;
        break;
        case 3: // +8
            D_HSCROLL = 8;
        break;
        case 4: // +16
            D_HSCROLL = 16;
        break;
    
        default:
        break;
    }

    HScroll = D_HSCROLL;
    if (!FontSize)
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
            bHighCL = FALSE;
        break;
        case 1: // Highlighted
            bHighCL = TRUE;
        break;
        case 2: // Custom
        break;
    
        default:
        break;
    }

    TTY_ReloadPalette();
}
