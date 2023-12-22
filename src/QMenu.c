
#include "QMenu.h"
#include "HexView.h"
#include "Terminal.h"
#include "Input.h"
#include "StateCtrl.h"
#include "Utils.h"

void WINFN_Reset();
void WINFN_Newline();
void WINFN_Columns();
void WINFN_BGColor();
void WINFN_TERMTYPE();
void WINFN_SERIALSPEED();
void WINFN_FONTSIZE();
void WINFN_PRINTDELAY();
void WINFN_RXTXSTATS();
void WINFN_DEBUGSEL();

static struct s_menu
{
    u8 num_entries;             // Number of entries in menu
    u8 selected_entry;          // Saved selected entry (automatic, leave at 0)
    u8 prev_menu;               // Previous menu to return to when going back (automatic, leave at 0)
    VoidCallback *entry_function;// Function callback which is called when entering entry
    VoidCallback *activate_function; // Function callback which is called when trying to enter an entry without submenu (next_menu=255)
    VoidCallback *exit_function; // Function callback which is called when exiting entry
    const char *title;          // Window title text
    u8 next_menu[8];            // Menu number selected entry leads to (255 = Do nothing)
    const char *text[8];        // Entry text
} MainMenu[] =
{{//0
    6,
    0, 0,
    NULL, NULL, NULL,
    "QUICK MENU",
    {1, 2, 3, 4, 13, 255},
    {"RESET",
     "DISCONNECT",
     "TERMINAL SETTINGS",
     "MEGA DRIVE SETTINGS",
     "DEBUG",
     "ABOUT"}
},
{//1
    2,
    0, 0,
    NULL, WINFN_Reset, NULL,
    "RESET",
    {255, 255},
    {"HARD RESET",
     "SOFT RESET"}
},
{//2
    1,
    0, 0,
    NULL, NULL, NULL,
    "DISCONNECT",
    {255},
    {"DISCONNECTING"}
},
{//3
    4,
    0, 0,
    NULL, NULL, NULL,
    "TERMINAL SETTINGS",
    {5, 6, 9, 11},
    {"NEWLINE CONVERSION",
     "MAX COLUMNS",
     "TERMINAL TYPE",
     "FONT SIZE"}
},
{//4
    4,
    0, 0,
    NULL, NULL, NULL,
    "MD SETTINGS",
    {7, 8, 10, 12},
    {"CONNECTED DEVICES",
     "BG COLOR",
     "SERIAL SPEED",
     "PRINT DELAY"}
},
{//5
    2,
    0, 0,
    NULL, WINFN_Newline, NULL,
    "NEWLINE CONVERSION",
    {255, 255},
    {"\\N = \\N",
     "\\N = \\N\\R"}
},
{//6
    2,
    0, 0,
    NULL, WINFN_Columns, NULL,
    "COLUMNS",
    {255, 255},
    {"80",
     "40"}
},
{//7
    3,
    0, 0,
    NULL, NULL, NULL,
    "CONNECTED DEVICES",
    {255, 255, 255},
    {"PORT 1: <UNKNOWN>",
     "PORT 2: UART",
     "PORT 3: <UNKNOWN>"}
},
{//8
    3,
    0, 0,
    NULL, WINFN_BGColor, NULL,
    "BG COLOR",
    {255, 255, 255},
    {"BLACK",
     "WHITE",
     "RANDOM"}
},
{//9
    5,
    0, 0,
    NULL, WINFN_TERMTYPE, NULL,
    "TERMINAL TYPE",
    {255, 255, 255, 255, 255},
    {"XTERM",
     "ANSI",
     "VT100",
     "MEGADRIVE",
     "UNKNOWN"}
},
{//10
    4,
    0, 0,
    NULL, WINFN_SERIALSPEED, NULL,
    "SERIAL SPEED",
    {255, 255, 255, 255},
    {"4800 BAUD",
     "2400 BAUD",
     "1200 BAUD",
     "300 BAUD"}
},
{//11
    3,
    2, 0,
    NULL, WINFN_FONTSIZE, NULL,
    "FONT SIZE",
    {255, 255, 255},
    {"8x8 COLOUR",
     "4x8 MONO",
     "4x8 MONO ANTIALIAS"}
},
{//12
    4,
    0, 0,
    NULL, WINFN_PRINTDELAY, NULL,
    "PRINT DELAY",
    {255, 255, 255, 255},
    {"0 MS",
     "1 MS",
     "2 MS",
     "4 MS"}
},
{//13
    2,
    0, 0,
    NULL, WINFN_DEBUGSEL, NULL,
    "DEBUG",
    {15, 255},
    {"TX/RX STATS",
     "HEX VIEW - RX"}
},
{//14
    3,
    0, 0,
    NULL, NULL, NULL,
    "ABOUT",
    {255, 255, 255},
    {"","",""/*"Code: smds",
     "Testing: b1tsh1ft3r",
     "4x8 Fonts: RKT"*/}
},
{//15
    2,
    0, 0,
    WINFN_RXTXSTATS, WINFN_RXTXSTATS, NULL,
    "TX/RX STATS",
    {255, 255},
    {"TX BYTES: 0",
     "RX BYTES: 0"}
}};

static u8 SelectedIdx = 0;
static u8 MenuIdx = 0;
static const u8 MenuPosX = 1, MenuPosY = 0;
bool bShowQMenu = FALSE;

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

void DrawMenu(u8 idx)
{
    char buf[32];
    
    VDP_setWindowVPos(FALSE, MainMenu[idx].num_entries+4);
    TRM_clearTextArea(0, 0, 36, MainMenu[idx].num_entries+4);

    MainMenu[MenuIdx].selected_entry = SelectedIdx;   // Mark previous menu selection entry

    MenuIdx = idx;
    SelectedIdx = MainMenu[MenuIdx].selected_entry;   // Get menu selection entry from new menu

    sprintf(buf, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", 201, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 187);
    TRM_drawText(buf, 1, 0, PAL1);
    sprintf(buf, " %s ", MainMenu[MenuIdx].title);
    TRM_drawText(buf, 2, 0, PAL1);

    sprintf(buf, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", 200, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 188);
    TRM_drawText(buf, 1, MainMenu[MenuIdx].num_entries+3, PAL1);

    for (u8 i = 0; i < MainMenu[MenuIdx].num_entries; i++)
    {        
        sprintf(buf, "%c %-19s %c", 186, MainMenu[MenuIdx].text[i], 186);
        TRM_drawText(buf, MenuPosX, MenuPosY+2+i, PAL1);
    }

    TRM_drawText(MainMenu[MenuIdx].text[SelectedIdx], MenuPosX+2, MenuPosY+2+SelectedIdx, PAL3);

    sprintf(buf, "%c %-19s %c", 186, "", 186);
    TRM_drawText(buf, MenuPosX, 1, PAL1);
    sprintf(buf, "%c %-19s %c", 186, "", 186);
    TRM_drawText(buf, MenuPosX, MainMenu[MenuIdx].num_entries+2, PAL1);

    //VoidCallback *func = MainMenu[MenuIdx].entry_function;
    //if (func != NULL) func();
}

void EnterMenu()
{
    u8 next = MainMenu[MenuIdx].next_menu[SelectedIdx];
    if (next != 255) 
    {
        VoidCallback *func = MainMenu[next].entry_function;
        if (func != NULL) func();

        MainMenu[next].prev_menu = MenuIdx;    // Return menu, when going back
        DrawMenu(next);
    }
    else
    {
        VoidCallback *func = MainMenu[MenuIdx].activate_function;
        if (func != NULL) func();
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
        VDP_setWindowVPos(FALSE, 1);
        TRM_clearTextArea(0, 0, 36, 1);
        TRM_drawText(STATUS_TEXT, 1, 0, PAL1);
        
        #ifndef NO_LOGGING
            KLog("Hiding window");
        #endif
    }
    else
    {
        VDP_setWindowVPos(FALSE, 10);
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
    MainMenu[menu_idx].text[entry_idx] = new_text;
}


void WINFN_Reset()
{
    QMenu_Toggle();

    switch (SelectedIdx)
    {
        case 0:
            SYS_hardReset();
        break;
        case 1:
            ResetSystem(FALSE);
        break;
    
        default:
        break;
    } 
}

void WINFN_Newline()
{    
    vNewlineConv = SelectedIdx;
}

void WINFN_Columns()
{
    TTY_SetColumns(SelectedIdx?D_COLUMNS_40:D_COLUMNS_80);
}

void WINFN_BGColor()
{
    switch (SelectedIdx)
    {
        case 0:
            PAL_setColor(0, 0);
        break;
        case 1:
            PAL_setColor(0, 0xeee);
        break;
        case 2:
            PAL_setColor(0, random());
        break;
    
        default:
            PAL_setColor(0, 0);
        break;
    }    
}

void WINFN_TERMTYPE()
{
    vTermType = SelectedIdx;
}

void WINFN_SERIALSPEED()
{
    vu8 *PSCTRL = (vu8 *)TTY_PORT_SCTRL;

    switch (SelectedIdx)
    {
        case 0: // 4800
            *PSCTRL = 0x38;
            vSpeed = "4800";
        break;
        case 1: // 2400
            *PSCTRL = 0x78;
            vSpeed = "2400";
        break;
        case 2: // 1200
            *PSCTRL = 0xB8;
            vSpeed = "1200";
        break;
        case 3: // 300
            *PSCTRL = 0xF8;
            vSpeed = "300";
        break;
    
        default:
        break;
    }
}

void WINFN_FONTSIZE()
{
    TTY_SetFontSize(SelectedIdx);
}

void WINFN_PRINTDELAY()
{
    switch (SelectedIdx)
    {
        case 0: // 0
            PrintDelay = 0;
        break;
        case 1: // 1
            PrintDelay = 1;
        break;
        case 2: // 2
            PrintDelay = 2;
        break;
        case 3: // 4
            PrintDelay = 4;
        break;
    
        default:
        break;
    }
}

char buf1[32];
char buf2[32];
void WINFN_RXTXSTATS()
{
    sprintf(buf1, "TX BYTES: %lu", TXBytes);
    sprintf(buf2, "RX BYTES: %lu", RXBytes);

    MainMenu[15].text[0] = buf1;
    MainMenu[15].text[1] = buf2;
}

void WINFN_DEBUGSEL()
{
    if (SelectedIdx == 1)
    {
        QMenu_Toggle();
        HexView_Toggle();
    }
}
