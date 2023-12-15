
#include "StateCtrl.h"
#include "Input.h"
#include "Terminal.h"
#include "main.h"

void MN_FUNC_TELNET();
void MN_FUNC_IRC();
void MN_FUNC_TERMINAL();
void MN_FUNC_DEVICES();

static const char *TelnetPage[27] =
{
    "Telnet session",
    "4800 Baud - 8n1",
    "",
    "Press <RETURN> to launch",
    "telnet session",
    "","","","","","","","","","","","","","","","","","","","","",""
};

static const char *IRCPage[27] =
{
    "IRC session",
    "",
    "Press <RETURN> to launch",
    "IRC session",
    "", "","","","","","","","","","","","","","","","","","","","","",""
};

static const char *SessionPage[27] =
{
    "Select session type",
    "","","","","","","","","","","","","","","","","","","","","","","","","",""
};

static const char *TerminalPage[27] =
{
    "Options controlling the",
    "terminal emulation",
    "",
    "",
    "","","","","","","","","","","","","","","","","","","","","","",""
};

static const char *ConnectionPage[27] =
{
    "Options controlling the",
    "connection",
    "",
    "Not implemented.",
    "","","","","","","","","","","","","","","","","","","","","","",""
};

static const char *DevicePage[27] =
{
    "Options controlling the",
    "devices connected to your",
    "mega drive",
    "",
    "Not implemented.",
    "","","","","","","","","","","","","","","","","","","","","",""
};

static const char *HelpPage[27] =
{
    "How to use SMDT",
    "",
    "Not implemented.",
    "","","","","","","","","","","","","","","","","","","","","","","",""
};

static const char *AboutPage[27] =
{
    "About SMDT",
    "",
    "Not implemented.",
    "","","","","","","","","","","","","","","","","","","","","","","",""
};

/*static const char *NopPage[27] =
{
    "Not implemented.",
    "","","","","","","","","","","","","","","","","","","","","","","","","",""
};*/


static const char *TerminalSubPage[27] =
{
    "Options controlling the",
    "terminal emulation",
    "",
    "",
    "","","","","","","","","","","","","","","","","","","","","","",""
    /*"[ ] Wrap mode initially on",
    "[ ] Convert CR to CRLF",
    "[x] Local echo",
    "",
    "","","","","","","","","","","","","","","","","","","","","","",""*/
};

static const char *KeyboardSubPage[27] =
{
    "Options controlling the",
    "effects of keys",
    "",
    "",
    "","","","","","","","","","","","","","","","","","","","","","",""
};

static const char *BellSubPage[27] =
{
    "Options controlling the",
    "terminal bell",
    "", "", "","","","","","","","","","","","","","","","","","","","","","",""
};

static const char *FeaturesSubPage[27] =
{
    "Enabling and disabling",
    "advanced terminal",
    "features", "", "","","","","","","","","","","","","","","","","","","","","","",""
};

static struct s_menu
{
    u8 num_entries;                 // Number of entries in menu
    u8 selected_entry;              // Saved selected entry (automatic, leave at 0)
    u8 prev_menu;                   // Previous menu to return to when going back (automatic, leave at 0)
    VoidCallback *entry_function;   // Function callback which is called when entering entry
    VoidCallback *activate_function;// Function callback which is called when trying to enter an entry without submenu (next_menu=255)
    VoidCallback *exit_function;    // Function callback which is called when exiting entry
    u8 next_menu[8];                // Menu number selected entry leads to (255 = Do nothing)
    const char **page[8];           // Sidepanel text
    u8 type[8];                     // 0=Normal entry - 1=Submenu
    const char *text[8];            // Entry text
} MainMenu[] =
{{//0
    6,
    0, 0,
    NULL, NULL, NULL,
    {1, 4, 255, 255, 255, 255},
    {SessionPage, TerminalPage, ConnectionPage, DevicePage, HelpPage, AboutPage},
    {0},
    {"Session",
     "Terminal",
     "Connection",
     "Devices",
     "Help",
     "About"}
},
{//1
    2,
    0, 0,
    NULL, NULL, NULL,
    {2, 3},
    {TelnetPage, IRCPage},
    {0},
    {"Telnet",
     "IRC"}
},
{//2    - Launch telnet client
    0,
    0, 0,
    MN_FUNC_TELNET, NULL, NULL,
    {255},
    {NULL},
    {0},
    {""}
},
{//3    - Launch IRC
    0,
    0, 0,
    MN_FUNC_IRC, NULL, NULL,
    {255},
    {NULL},
    {0},
    {""}
},
{//4    - Terminal
    4,
    0, 0,
    NULL, NULL, NULL,
    {128, 255, 255, 255},
    {TerminalSubPage, KeyboardSubPage, BellSubPage, FeaturesSubPage},
    {1, 0, 0, 0},
    {"Terminal",
     "Keyboard",
     "Bell",
     "Features"}
},
{//5(128)    - Actual terminal this time
    0,
    0, 0,
    MN_FUNC_TERMINAL, NULL, NULL,
    {255},
    {NULL},
    {0},
    {""}
}};

#define SM_CHECKBOX 1
#define SM_TEXTBOX 2

static struct s_submenu
{
    u8 num_entries;             // Number of entries in menu
    u8 selected_entry;          // Saved selected entry (automatic, leave at 0)
    u8 type[16];
    void *ptr[16];
    const char *text[16];        // Entry text
} SubMenu[] =
{{//0
    3, 0,
    {SM_CHECKBOX, SM_CHECKBOX, SM_CHECKBOX},
    {&bWrapAround, &vNewlineConv, NULL},
    {"Wrap mode initially on",
     "Convert CR to CRLF",
     "Local echo"}
}};

static const u8 Frame[][40] = 
{
    {201, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 187},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {204, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 209, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 202, 205, 205, 205, 187},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 179, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {200, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 207, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 188},
};

static const u8 MenuPosX = 1, MenuPosY = 1;
static const u8 SubMenuPosX = 12, SubMenuPosY = 1;
static u8 SelectedIdx = 0;
static u8 MenuIdx = 0;

static u8 SubSelectedIdx = 0;
static u8 SubMenuIdx = 0;
static u8 bInSubMenu = FALSE;

static void DrawFrame()
{
    TRM_drawText((char*)Frame[0], 0, 0, PAL1);
    TRM_drawText((char*)Frame[1], 0, 1, PAL1);
    TRM_drawText((char*)Frame[2], 0, 2, PAL1);
    for (u8 y = 3; y < 27; y++)
    {
        TRM_drawText((char*)Frame[3], 0, y, PAL1);
    }
    TRM_drawText((char*)Frame[4], 0, 27, PAL1);
}

static void DrawMenu(u8 idx)
{
    TRM_clearTextArea(MenuPosX, MenuPosY+2, 10, 22);

    MainMenu[MenuIdx].selected_entry = SelectedIdx;   // Mark previous menu selection entry

    MenuIdx = idx;
    SelectedIdx = MainMenu[MenuIdx].selected_entry;   // Get menu selection entry from new menu

    for (u8 i = 0; i < MainMenu[MenuIdx].num_entries; i++)
    {
        TRM_drawText(MainMenu[MenuIdx].text[i], MenuPosX, MenuPosY+2+i, PAL1);
    }

    TRM_drawText(MainMenu[MenuIdx].text[SelectedIdx], MenuPosX, MenuPosY+2+SelectedIdx, PAL3);
}

static void DrawSubMenu(u8 idx)
{
    char buf[32];
    u8 bv = 255;
    TRM_clearTextArea(SubMenuPosX, SubMenuPosY+2, 10, 22);

    SubMenu[SubMenuIdx].selected_entry = SubSelectedIdx;   // Mark previous menu selection entry

    SubMenuIdx = idx;
    SubSelectedIdx = SubMenu[SubMenuIdx].selected_entry;   // Get menu selection entry from new menu

    for (u8 i = 0; i < SubMenu[SubMenuIdx].num_entries; i++)
    {
        switch (SubMenu[SubMenuIdx].type[i])
        {
            case SM_CHECKBOX:
                bv = 0;//(u8)&SubMenu[SubMenuIdx].ptr[i];
                kprintf("bv = %u", bv);
                sprintf(buf, "[%c]", (char)(bv==FALSE?' ':'X'));
            break;

            case SM_TEXTBOX:
                sprintf(buf, " ");
            break;
        
            default:
                sprintf(buf, "NUL");
            break;
        }

        TRM_drawText(buf, SubMenuPosX, SubMenuPosY+2+i, PAL1);
        TRM_drawText(SubMenu[SubMenuIdx].text[i], SubMenuPosX+4, SubMenuPosY+2+i, PAL1);
    }

    TRM_drawText(SubMenu[SubMenuIdx].text[SubSelectedIdx], SubMenuPosX+4, SubMenuPosY+2+SubSelectedIdx, PAL3);
}

static void DrawPage(const char *page[])
{
    if (page == NULL) return;

    TRM_clearTextArea(12, 3, 27, 24);

    for (u8 y = 0; y < 24; y++)
    {
        TRM_drawText(page[y], 12, y+3, PAL1);
    }
}

static void EnterMenu()
{
    if (bInSubMenu) return;

    if (MainMenu[MenuIdx].type[SelectedIdx] > 0)
    {
        TRM_clearTextArea(12, 3, 27, 24);
        DrawSubMenu(MainMenu[MenuIdx].type[SelectedIdx]-1);
        bInSubMenu = TRUE;
        return;
    }

    u8 next = MainMenu[MenuIdx].next_menu[SelectedIdx];
    if (next != 255) 
    {
        VoidCallback *func = MainMenu[next].entry_function;
        if (func != NULL) func();

        MainMenu[next].prev_menu = MenuIdx;    // Return menu, when going back
        DrawMenu(next);

        DrawPage(MainMenu[MenuIdx].page[SelectedIdx]);
    }
    else
    {
        VoidCallback *func = MainMenu[MenuIdx].activate_function;
        if (func != NULL) func();
    }
}

static void ExitMenu()
{
    if (bInSubMenu)
    {
        bInSubMenu = FALSE;
        DrawPage(MainMenu[MenuIdx].page[SelectedIdx]);
        return;
    }

    VoidCallback *func = MainMenu[MenuIdx].exit_function;
    if (func != NULL) func();

    //if (MainMenu[MenuIdx].prev_menu == MenuIdx) QMenu_Toggle();   // prev_menu == MenuIdx is only true when at the root menu, therefor close the window if trying to back out
    //else 
    DrawMenu(MainMenu[MenuIdx].prev_menu);
}

static void UpMenu()
{
    if (bInSubMenu)
    {
        TRM_drawText(SubMenu[SubMenuIdx].text[SubSelectedIdx], SubMenuPosX+4, SubMenuPosY+2+SubSelectedIdx, PAL1);
        SubSelectedIdx = (SubSelectedIdx == 0 ? SubMenu[SubMenuIdx].num_entries-1 : SubSelectedIdx-1);
        TRM_drawText(SubMenu[SubMenuIdx].text[SubSelectedIdx], SubMenuPosX+4, SubMenuPosY+2+SubSelectedIdx, PAL3);
    }
    else
    {
        TRM_drawText(MainMenu[MenuIdx].text[SelectedIdx], MenuPosX, MenuPosY+2+SelectedIdx, PAL1);
        SelectedIdx = (SelectedIdx == 0 ? MainMenu[MenuIdx].num_entries-1 : SelectedIdx-1);
        TRM_drawText(MainMenu[MenuIdx].text[SelectedIdx], MenuPosX, MenuPosY+2+SelectedIdx, PAL3);

        DrawPage(MainMenu[MenuIdx].page[SelectedIdx]);
    }
}

static void DownMenu()
{
    if (bInSubMenu)
    {
        TRM_drawText(SubMenu[SubMenuIdx].text[SubSelectedIdx], SubMenuPosX+4, SubMenuPosY+2+SubSelectedIdx, PAL1);
        SubSelectedIdx = (SubSelectedIdx == SubMenu[SubMenuIdx].num_entries-1 ? 0 : SubSelectedIdx+1);
        TRM_drawText(SubMenu[SubMenuIdx].text[SubSelectedIdx], SubMenuPosX+4, SubMenuPosY+2+SubSelectedIdx, PAL3);
    }
    else
    {
        TRM_drawText(MainMenu[MenuIdx].text[SelectedIdx], MenuPosX, MenuPosY+2+SelectedIdx, PAL1);
        SelectedIdx = (SelectedIdx == MainMenu[MenuIdx].num_entries-1 ? 0 : SelectedIdx+1);
        TRM_drawText(MainMenu[MenuIdx].text[SelectedIdx], MenuPosX, MenuPosY+2+SelectedIdx, PAL3);

        DrawPage(MainMenu[MenuIdx].page[SelectedIdx]);
    }
}

static void UpdateView()
{
    DrawMenu(0);
}

void MN_FUNC_TELNET()
{
    ChangeState(PS_Telnet, 0, NULL);
}

void MN_FUNC_IRC()
{
    ChangeState(PS_IRC, 0, NULL);
}

void MN_FUNC_TERMINAL()
{
    DrawSubMenu(0);
}

void MN_FUNC_DEVICES()
{

}

// -- State ------------------------------------------------------------------

#ifndef EMU_BUILD
static u8 kbdata;
#endif

void Enter_Entry(u8 argc, const char *argv[])
{
    VDP_setWindowVPos(FALSE, 30);
    TRM_clearTextArea(0, 0, 35, 1);
    TRM_clearTextArea(0, 1, 40, 29);

    DrawFrame();
    TRM_drawText("SMDT - Startup Menu", 1, 1, PAL1);
    TRM_drawText("Welcome to SMDT!", 12, 8, PAL1);
    
    //TRM_drawText("Keyboard required!", 12, 10, PAL1);

    TRM_drawText("Use Up/Down key to select", 12, 13, PAL1);
    TRM_drawText("an entry", 12, 14, PAL1);

    TRM_drawText("Use Return/Escape key to", 12, 16, PAL1);
    TRM_drawText("enter or leave an entry", 12, 17, PAL1);

    TRM_drawText("-smds", 12, 24, PAL1);

    /*
    TRM_drawText("Session", 1, 3, PAL1);
    TRM_drawText("Terminal", 1, 4, PAL1);
    TRM_drawText("Devices", 1, 5, PAL1);
    TRM_drawText("Help", 1, 6, PAL1);

    //JOY_setSupport(PORT_2, JOY_SUPPORT_KEYBOARD);

    kprintf("PORT1: %u - PORT2: %u", JOY_getPortType(PORT_1), JOY_getPortType(PORT_2));
    */

    UpdateView();
}

void ReEnter_Entry()
{
}

void Exit_Entry()
{
    VDP_setWindowVPos(FALSE, 1);
    TRM_clearTextArea(0, 0, 36, 1);
    TRM_drawText(STATUS_TEXT, 1, 0, PAL1);
}

void Reset_Entry()
{
}

void Run_Entry()
{
    #ifndef EMU_BUILD
    while (KB_Poll2(&kbdata))
    {
        KB_Handle_Scancode(kbdata);
    }
    #endif
}

void Input_Entry()
{
    if (is_KeyDown(KEY_UP))
    {
        UpMenu();
    }

    if (is_KeyDown(KEY_DOWN))
    {
        DownMenu();
    }

    if (is_KeyDown(KEY_LEFT))
    {
    }

    if (is_KeyDown(KEY_RIGHT))
    {
    }

    if (is_KeyDown(KEY_ESCAPE))
    {
        ExitMenu();
    }

    if (is_KeyDown(KEY_RETURN))
    {
        EnterMenu();
    }
}

const PRG_State EntryState = 
{
    Enter_Entry, ReEnter_Entry, Exit_Entry, Reset_Entry, Run_Entry, Input_Entry
};

