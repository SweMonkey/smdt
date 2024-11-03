#include "CMDFunc.h"
#include "StateCtrl.h"
#include "Utils.h"
#include "Network.h"
#include "IRC.h"
#include "Telnet.h"
#include "Terminal.h"
#include "ConfigFile.h"
#include "HexView.h"

#include "devices/XP_Network.h"
#include "devices/Keyboard_PS2.h"

#include "misc/VarList.h"

#include "system/Stdout.h"
#include "system/Time.h"
#include "system/File.h"
#include "system/Filesystem.h"
#include "system/ELF_ldr.h"


SM_CMDList CMDList[] =
{
    {"telnet",  CMD_LaunchTelnet,   "<address:port>"},
    {"irc",     CMD_LaunchIRC,      "<address:port>"},
    {"gopher",  CMD_LaunchGopher,   "<address>"},
    {"tty",     CMD_LaunchLinuxTTY, "- Init sane linux TTY"},
    {"echo",    CMD_Echo,           "- Echo string to screen"},
    {"kbc",     CMD_KeyboardSend,   "- Send command to keyboard"},
    {"tmattr",  CMD_SetAttr,        "- Set terminal attributes"},
    {"xport",   CMD_xport,          "- Send command to xport"},
    {"uname",   CMD_UName,          "- Print system information"},
    {"setcon",  CMD_SetConn,        "- Set connection timeout"},
    {"clear",   CMD_ClearScreen,    "- Clear screen"},
    {"setvar",  CMD_SetVar,         "- Set variable"},
    {"getip",   CMD_GetIP,          "- Get network IP"},
    {"free",    CMD_Free,           "- List free memory"},
    {"reboot",  CMD_Reboot,         "- Reboot system"},
    {"savecfg", CMD_SaveCFG,        "- Save confg to file"},
    {"test",    CMD_Test,           "- Test"},
    {"bflush",  CMD_FlushBuffer,    "- Flush specified buffer"},
    {"bprint",  CMD_PrintBuffer,    "- Print byte from buffer"},
    {"ping",    CMD_Ping,           "- Ping IP address"},
    {"run",     CMD_Run,            "- Run binary file"},
    {"cd",      CMD_ChangeDir,      "- Change directory"},
    {"ls",      CMD_ListDir,        "- List directory"},
    {"mv",      CMD_MoveFile,       "- Move file"},
    {"cp",      CMD_CopyFile,       "- Copy file"},
    {"cat",     CMD_Concatenate,    "- Concatenate file"},
    {"touch",   CMD_Touch,          "- Update or create file"},
    {"mkdir",   CMD_MakeDirectory,  "- Create directory"},
    {"rm",      CMD_RemoveLink,     "- Remove directory/file"},
    {"attr",    CMD_Attr,           "- Modify file attributes"},
    {"lsblk",   CMD_ListBlock,      "- List block devices"},
    {"hexview", CMD_HexView,        "<filename>"},
    {"uptime",  CMD_Uptime,         "- Show system uptime"},
    {"date",    CMD_Date,           "- Show/Set date and time"},
    {"about",   CMD_About,          "- About SMDTC/Licenses"},
    {"help",    CMD_Help,           "- This command"},
    {0, 0, 0}  // List terminator
};


void CMD_LaunchTelnet(u8 argc, char *argv[]) 
{
    ChangeState(PS_Telnet, argc, argv); 
}

void CMD_LaunchIRC(u8 argc, char *argv[])
{
    ChangeState(PS_IRC, argc, argv);
}

void CMD_LaunchLinuxTTY(u8 argc, char *argv[]) 
{
    char *arg[] = {argv[0], "-tty"};
    ChangeState(PS_Telnet, 2, arg);
}
void CMD_LaunchGopher(u8 argc, char *argv[]) 
{
    #ifndef EMU_BUILD
    if (argc < 2)
    {
        Stdout_Push("[91mNo address specified![0m\n");
        return;
    }
    #endif

    ChangeState(PS_Gopher, argc, argv);
}

void CMD_SetAttr(u8 argc, char *argv[])
{
    char tmp[32];

    switch (argc)
    {
        case 2:
            sprintf(tmp, "[%um\n", atoi(argv[1]));
            Stdout_Push(tmp);
        break;
        case 3:
            sprintf(tmp, "[%u;%um\n", atoi(argv[1]), atoi(argv[2]));
            Stdout_Push(tmp);
        break;
    
        default:
            Stdout_Push("Set terminal attribute\n\nUsage:\n");
            Stdout_Push(argv[0]);
            Stdout_Push(" <number> <number>\n");
            Stdout_Push(argv[0]);
            Stdout_Push(" <number>\n");
        return;
    }

    Stdout_Push("Attributes set.\n");
}

void CMD_Echo(u8 argc, char *argv[]) 
{
    if (argc < 2)
    {
        Stdout_Push("Usage:\necho [string or $variable]... [> filename]\n");
        return;
    }

    char *buffer = malloc(256);

    if (buffer == NULL) return;

    memset(buffer, 0, 256);

    // Pointer for the output file, initially set to NULL.
    SM_File *file = NULL;
    int end_index = argc;

    // Check if the second-to-last argument is ">" indicating file redirection.
    if (argc >= 3 && strcmp(argv[argc - 2], ">") == 0)
    {
        // Open the file specified as the last argument for writing.
        char *fn_buf = malloc(FILE_MAX_FNBUF);
        FS_ResolvePath(argv[argc - 1], fn_buf);
        file = F_Open(fn_buf, FM_CREATE | FM_WRONLY);
        free(fn_buf);

        if (file == NULL)
        {
            Stdout_Push("Error opening file");
            return;
        }
        // Update the end index to exclude the ">" and filename arguments.
        end_index = argc - 2;
    }

    // Iterate through the arguments and print them to the chosen output (screen or file).
    for (int i = 1; i < end_index; i++)
    {
        if (argv[i][0] == '$') 
        {
            char *value = malloc(256);
            memset(value, 0, 256);
            getenv(argv[i] + 1, value);

            if (value)
            {
                strncat(buffer, value, 256);
                free(value);
            }
            else
            {
                strncat(buffer, argv[i], 256);
            }
        }
        else
        {
            strncat(buffer, argv[i], 256);
        }

        // Add a space between arguments, except for the last one.
        if (i < end_index - 1)
        {
            strncat(buffer, " ", 256);
        }
    }

    if (file)
    {
        F_Write(buffer, strlen(buffer), 1, file);
        F_Close(file);
    }
    else
    {
        Stdout_Push(buffer);
        Stdout_Push("\n");
        Stdout_Flush();
    }

    free(buffer);
    return;
}

void CMD_KeyboardSend(u8 argc, char *argv[])
{
    char tmp[64];

    if (argc < 2) 
    {
        Stdout_Push("Send command to keyboard\n\nUsage:\n");
        Stdout_Push(argv[0]);
        Stdout_Push(" <decimal number between 0 and 255>\n");
        return;
    }

    u8 kbcmd = atoi(argv[1]);
    u8 ret = 0;

    sprintf(tmp, "Sending command $%X to keyboard...\n", kbcmd);
    Stdout_Push(tmp);
    
    ret = KB_PS2_SendCommand(kbcmd);
    
    sprintf(tmp, "Recieved byte $%X from keyboard   \n", ret);
    Stdout_Push(tmp);
}

void CMD_Help(u8 argc, char *argv[])
{
    u16 i = 0;
    
    Stdout_Push("Commands available:\n\n");

    while (CMDList[i].id != 0)
    {
        stdout_printf("%10s %-28s\n", CMDList[i].id, CMDList[i].desc);

        i++;
    }
}

void CMD_xport(u8 argc, char *argv[])
{
    if (argc == 1)
    {
        Stdout_Push("xPort debug\n\nUsage:\n\
xport enter       - Enter monitor mode\n\
xport exit        - Exit monitor mode\n\
xport <string>    - Send string to xPort\n\
xport connect <address>\n\
xport disconnect  - Close connection\n");
        return;
    }

    if ((argc > 1) && (strcmp(argv[1], "enter") == 0))
    {
        stdout_printf("Entering monitor mode...\n");
        Stdout_Flush();

        bool r = XPN_EnterMonitorMode();

        if (r) stdout_printf("OK!\n");
        else stdout_printf("Timeout!\n");
        
    }
    else if ((argc > 1) && (strcmp(argv[1], "exit") == 0))
    {
        stdout_printf("Exiting monitor mode...\n");
        Stdout_Flush();
        
        bool r = XPN_ExitMonitorMode();

        if (r) stdout_printf("OK!\n");
        else stdout_printf("Timeout!\n");
    }
    else if ((argc > 2) && (strcmp(argv[1], "connect") == 0))
    {
        stdout_printf("Connecting to %s ...\n", argv[2]);
        Stdout_Flush();

        bool r = XPN_Connect(argv[2]);

        if (r) stdout_printf("Connected!\n");
        else stdout_printf("Error!\n");
    }
    else if ((argc > 1) && (strcmp(argv[1], "disconnect") == 0))
    {
        stdout_printf("Disconnecting... \n");
        Stdout_Flush();

        XPN_Disconnect();
    }
    else if (argc > 1)
    {
        char buf[128];
        
        strclr(buf);

        for (u8 i = 1; i < argc; i++)
        {
            strcat(buf, argv[i]);
            if (i != argc-1) strcat(buf, " ");
        }

        stdout_printf("Sending string:\n\"%s\"\n", buf);
        Stdout_Flush();

        XPN_SendMessage(buf);
    }
}

void CMD_UName(u8 argc, char *argv[])
{
    char tmp[256] = {'\0'};
    const char *Krnl_Str = "NO-KERNEL ";
    const char *OS_Str = "SMDTC ";
    const char *Mach_Str = "m68k ";

    if (argc == 1)
    {
        Stdout_Push(OS_Str);
        Stdout_Push("\n");
        return;
    }

    for (u8 i = 1; i < argc; i++)
    {
        switch (argv[i][1])
        {
            case 's':
                strcat(tmp, Krnl_Str);
            break;
            case 'n':
                strcat(tmp, sv_Username);
                strcat(tmp, " ");
            break;
            case 'r':
            case 'v':
                strcat(tmp, STATUS_TEXT);
                strcat(tmp, " ");
            break;
            case 'm':
            case 'p':
                strcat(tmp, Mach_Str);
            break;
            case 'o':
                strcat(tmp, OS_Str);
            break;

            case 'a':
                sprintf(tmp, "%s%s%s %s", OS_Str, Mach_Str, STATUS_TEXT, sv_Username);
            break;
        
            default:
            break;
        }
    }

    strcat(tmp, "\n");
    Stdout_Push(tmp);
}

void CMD_SetConn(u8 argc, char *argv[])
{
    char tmp[64];

    if (argc < 2) 
    {
        Stdout_Push("Set connection time out\n\nUsage:\nsetcon <number of ticks>\n\n");
        sprintf(tmp, "Current time out: %lu ticks\n", sv_ConnTimeout);
        Stdout_Push(tmp);
        return;
    }

    sv_ConnTimeout = atoi32(argv[1]);

    sprintf(tmp, "Connection time out set to %lu\n", sv_ConnTimeout);
    Stdout_Push(tmp);
}

void CMD_ClearScreen(u8 argc, char *argv[])
{
    TTY_Reset(TRUE);
}

void CMD_SetVar(u8 argc, char *argv[])
{
    if ((argc < 3) && (strcmp(argv[1], "-list")))
    {
        Stdout_Push("Set variable\n\nUsage:\nsetvar <variable_name> <value>\nsetvar -list\n");
        return;
    }

    if (argc == 2)
    {
        u16 i = 0;
        stdout_printf("%-12s %s   %s\n\n", "Name", "Type", "Value");

        while (VarList[i].size)
        {
            switch (VarList[i].size)
            {
                case ST_BYTE:
                    stdout_printf("%-12s u8     %u\n", VarList[i].name, *((u8*)VarList[i].ptr));
                break;
                case ST_WORD:
                    stdout_printf("%-12s u16    %u\n", VarList[i].name, *((u16*)VarList[i].ptr));
                break;
                case ST_LONG:
                    stdout_printf("%-12s u32    %lu\n", VarList[i].name, *((u32*)VarList[i].ptr));
                break;        
                case ST_SPTR:
                    stdout_printf("%-12s StrPtr \"%s\"\n", VarList[i].name, (char*)VarList[i].ptr);
                break;
                case ST_SARR:
                    stdout_printf("%-12s StrArr \"%s\"\n", VarList[i].name, (char*)VarList[i].ptr);
                break;
                default:
                break;
            }

            i++;
        }

        Stdout_Push("\n[96mNote: Changes to variables may not take effect until you reboot.[0m\n");

        return;
    }

    if (argc >= 3)
    {
        u16 i = 0;
        char catbuf[64] = {'\0'};
        
        while ((i < 65500) && (VarList[i].size))
        {
            if (strcmp(VarList[i].name, argv[1]) == 0)
            {
                switch (VarList[i].size)
                {
                    case ST_BYTE:
                        *(u8*)VarList[i].ptr = atoi(argv[2]);
                        i = 65534;
                    break;
                    case ST_WORD:
                        *(u16*)VarList[i].ptr = atoi16(argv[2]);
                        i = 65534;
                    break;
                    case ST_LONG:
                        *(u32*)VarList[i].ptr = atoi32(argv[2]);
                        i = 65534;
                    break;
                    case ST_SPTR:
                    break;
                    case ST_SARR:
                        for (u8 a = 0; a < argc-2; a++)
                        {
                            strncat(catbuf, argv[a+2], 64);

                            if (a < argc-3)
                            {
                                u16 l = strlen(catbuf);
                                catbuf[l > 62 ? 62 : l] = ' ';
                                catbuf[l+1 > 63 ? 63 : l+1] = '\0';
                            }
                        }
                        strcpy((char*)VarList[i].ptr, catbuf); // ohboy... lets hope the receiving character array can hold everything :)))
                        i = 65534;
                    break;
                    default:
                    break;
                }
            }

            i++;
        }

        if (i == 65534) CFG_SaveData();
        return;
    }
}

void CMD_GetIP(u8 argc, char *argv[])
{
    char *ipstr = malloc(32);

    if (ipstr)
    {
        Stdout_Push("Please wait...\n");

        u8 r = NET_GetIP(ipstr);

        if (r == 0)
        {
            char tmp[64];

            sprintf(tmp, "IP: %s\n", ipstr);
            Stdout_Push(tmp);
        }
        else if (r == 1)
        {
            Stdout_Push("Error: IPSTR is NULL!\n");
        }
        else if (r == 2)
        {
            Stdout_Push("Error: Timed out\n");
        }

        free(ipstr);

        return;
    }

    Stdout_Push("Error: Out of RAM!\n");
}

void CMD_Free(u8 argc, char *argv[])
{
    if ((argc > 1) && (strcmp(argv[1], "-defrag") == 0))
    {
        MEM_pack();
        Stdout_Push("Defrag complete.\n");
        return;
    }

    if (bMegaCD)
    {
        Stdout_Push("\n─ Mega Drive ──────────────────────────\n");
    }

    // May need an "else print \n" here?

    stdout_printf("%20s %5u bytes\n", "Free:", MEM_getFree());
    stdout_printf("%20s %5u bytes\n", "Largest free block:", MEM_getLargestFreeBlock());
    stdout_printf("%20s %5u bytes\n", "Used:", MEM_getAllocated());
    stdout_printf("%20s %5u bytes\n", "System reserved:", 65536-(MEM_getFree()+MEM_getAllocated()));

    u16 a = MEM_getFree()/1650;
    u16 b = MEM_getAllocated()/1650;
    u16 c = (65536/1650)-a-b;

    Stdout_Push("\n[32m");
    for (u8 i = 0; i < a; i++) Stdout_Push("█");    // Total Free
    Stdout_Push("[31m");
    for (u8 i = 0; i < b; i++) Stdout_Push("█");    // Used
    Stdout_Push("[94m");
    for (u8 i = 0; i < c; i++) Stdout_Push("▒");    // System reserved
    Stdout_Push("[0m\n");


    if (bMegaCD)
    {
        stdout_printf("\n─ %s ─────────────────────────────\n", bPALSystem ? "Sega CD" : "Mega CD");

        stdout_printf("%20s %6u bytes\n", "Free:", 524288);
        stdout_printf("%20s %6u bytes\n", "Largest free block:", 524288);
        stdout_printf("%20s %6u bytes\n", "Used:", 0);

        a = 524288/13200;
        b = 0;
        c = (524288/13200)-a-b;

        Stdout_Push("\n[32m");
        for (u8 i = 0; i < a; i++) Stdout_Push("█");
        Stdout_Push("[31m");
        for (u8 i = 0; i < b; i++) Stdout_Push("█");
        Stdout_Push("[0m\n");
    }
    
    Stdout_Push("\n─ Info ────────────────────────────────\n");

    stdout_printf("Run \"%s -defrag\" for memory defrag.\n", argv[0]);
}

void CMD_Reboot(u8 argc, char *argv[])
{
    if ((argc > 1) && (strcmp(argv[1], "-soft") == 0)) SYS_reset();

    SYS_hardReset();
}

void CMD_SaveCFG(u8 argc, char *argv[])
{
    CFG_SaveData();
}

void CMD_Test(u8 argc, char *argv[])
{
    Stdout_Push("[91mWarning: The test command may cause side effects on SMDTC operation.\nIt may outright crash your system depending on the parameters given![0m\n\n");
    /*if (argc > 1)
    {
        for (u8 i = 0; i < argc; i++) 
        {
            Stdout_Push(argv[i]);
            Stdout_Push("\n");
        }
        return;
    }*/

    if ((argc > 2) && (strcmp("-f", argv[1]) == 0))
    {
        char buf[162] = "Hello World!\nThis is a long text file that just keeps dragging on and on and on...\nMaybe it will repeat forever? who knows, it shouldn't repeat... but it might";
        SM_File *f = F_Open(argv[2], FM_WRONLY);

        stdout_printf("Writing this:\n\n%s\n\nto file %s\n", buf, argv[2]);
        F_Write(buf, 162, 1, f);

        F_Close(f);
        
        f = F_Open(argv[2], FM_RDONLY);

        memset(buf, 0, 162);
        F_Read(buf, 162, 1, f);

        stdout_printf("Reading back file %s:\n\n%s\n", argv[2], buf);

        F_Close(f);

        return;
    }

    /*if ((argc > 1) && (strcmp("-test1arg", argv[1]) == 0))
    {
        return;
    }

    if ((argc > 2) && (strcmp("-test2arg", argv[1]) == 0))
    {
        return;
    }*/

    if ((argc > 1) && (strcmp("-c64", argv[1]) == 0))
    {
        /*PAL_setColor(32, 0x000);
        PAL_setColor(33, RGB24_TO_VDPCOLOR(0x894036));
        PAL_setColor(34, RGB24_TO_VDPCOLOR(0x68A941));
        PAL_setColor(35, RGB24_TO_VDPCOLOR(0xD0DC71));
        PAL_setColor(36, RGB24_TO_VDPCOLOR(0x3E31A2));
        PAL_setColor(37, RGB24_TO_VDPCOLOR(0x8A46AE));
        PAL_setColor(38, RGB24_TO_VDPCOLOR(0x7ABFC7));
        PAL_setColor(39, RGB24_TO_VDPCOLOR(0xABABAB));
        
        PAL_setColor(40, RGB24_TO_VDPCOLOR(0x555555));
        PAL_setColor(41, RGB24_TO_VDPCOLOR(0xBB776D));
        PAL_setColor(42, RGB24_TO_VDPCOLOR(0xACEA88));
        //PAL_setColor(43, RGB24_TO_VDPCOLOR(0x0));//LIGHT YELLOW
        PAL_setColor(44, RGB24_TO_VDPCOLOR(0x7C70DA));
        //PAL_setColor(45, RGB24_TO_VDPCOLOR(0x0));//LIGHT MAGENTA
        //PAL_setColor(46, RGB24_TO_VDPCOLOR(0x0));//LIGHT TEAL
        PAL_setColor(47, RGB24_TO_VDPCOLOR(0xFFFFFF));*/

        PAL_setColor(32, 0x000);
        PAL_setColor(33, RGB24_TO_VDPCOLOR(0x772D26));  // Red
        PAL_setColor(34, RGB24_TO_VDPCOLOR(0x559E4A));  // Green
        PAL_setColor(35, RGB24_TO_VDPCOLOR(0xBDCC71));  // Yellow
        PAL_setColor(36, RGB24_TO_VDPCOLOR(0x42348B));  // Blue
        PAL_setColor(37, RGB24_TO_VDPCOLOR(0xA85FB4));  // Magenta
        PAL_setColor(38, RGB24_TO_VDPCOLOR(0x85D4DC));  // Teal
        PAL_setColor(39, RGB24_TO_VDPCOLOR(0xABABAB));  // Light gray -
        
        PAL_setColor(40, RGB24_TO_VDPCOLOR(0x555555));  // Dark gray -
        PAL_setColor(41, RGB24_TO_VDPCOLOR(0xB66862));  // Light red
        PAL_setColor(42, RGB24_TO_VDPCOLOR(0x92DF87));  // Light green
        PAL_setColor(43, RGB24_TO_VDPCOLOR(0xFFFFB0));  // Light yellow
        PAL_setColor(44, RGB24_TO_VDPCOLOR(0x7E70CA));  // Light blue
        PAL_setColor(45, RGB24_TO_VDPCOLOR(0xE99DF5));  // Light magenta
        PAL_setColor(46, RGB24_TO_VDPCOLOR(0xC5FFFF));  // Light teal
        PAL_setColor(47, RGB24_TO_VDPCOLOR(0xFFFFFF));  // White
        
        return;
    }

    if ((argc > 2) && (strcmp("-buf", argv[1]) == 0))
    {
        u8 n = atoi(argv[2]);

        kprintf("n= %u", n);

        if (n == 1) Stdout_Push("[?1049h");
        else Stdout_Push("[?1049l");

        Stdout_Flush();

        return;
    }

    if ((argc > 1) && (strcmp("-erase", argv[1]) == 0))
    {    
        if (!sv_Font)
        {
            Stdout_Push("0123456789ABCDEF0123456789ABCDEF01234567\n");
            Stdout_Flush();
            Stdout_Push("████████████████████████████████████████\n");
            Stdout_Flush();
            Stdout_Push("████████████████████████████████████████\n");
            Stdout_Flush();
            Stdout_Push("████████████████████████████████████████\n");
            Stdout_Flush();
            Stdout_Push("████████████████████████████████████████\n");
            Stdout_Flush();
        }
        else
        {
            Stdout_Push("0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF\n");
            Stdout_Flush();
            Stdout_Push("████████████████████████████████████████████████████████████████████████████████\n");
            Stdout_Flush();
            Stdout_Push("████████████████████████████████████████████████████████████████████████████████\n");
            Stdout_Flush();
            Stdout_Push("████████████████████████████████████████████████████████████████████████████████\n");
            Stdout_Flush();
            Stdout_Push("████████████████████████████████████████████████████████████████████████████████\n");
            Stdout_Flush();
        }

        // Erase from start to cursor
        TTY_MoveCursor(TTY_CURSOR_UP, 4);
        TTY_MoveCursor(TTY_CURSOR_RIGHT, 4);
        Stdout_Push("[1K");
        Stdout_Flush();

        // Erase from cursor to end
        TTY_MoveCursor(TTY_CURSOR_DOWN, 1);
        Stdout_Push("[0K");
        Stdout_Flush();

        // Erase entire line
        TTY_MoveCursor(TTY_CURSOR_DOWN, 1);
        Stdout_Push("[2K");
        Stdout_Flush();

        TTY_MoveCursor(TTY_CURSOR_DOWN, 1);
        
        return;
    }
    
    if ((argc > 2) && (strcmp("-erasev", argv[1]) == 0))
    {    
        u8 n = atoi(argv[2]);

        kprintf("n= %u", n);

        if (!sv_Font)
        {
            Stdout_Push("0123456789ABCDEF0123456789ABCDEF01234567\n");
            Stdout_Flush();
            Stdout_Push("████████████████████████████████████████\n");
            Stdout_Flush();
            Stdout_Push("████████████████████████████████████████\n");
            Stdout_Flush();
            Stdout_Push("████████████████████████████████████████\n");
            Stdout_Flush();
            Stdout_Push("████████████████████████████████████████\n");
            Stdout_Flush();
            Stdout_Push("████████████████████████████████████████\n");
            Stdout_Flush();
            Stdout_Push("████████████████████████████████████████\n");
            Stdout_Flush();
            Stdout_Push("████████████████████████████████████████\n");
            Stdout_Flush();
            Stdout_Push("████████████████████████████████████████\n");
            Stdout_Flush();
        }
        else
        {
            Stdout_Push("0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF\n");
            Stdout_Flush();
            Stdout_Push("0███████████████████████████████████████████████████████████████████████████████\n");
            Stdout_Flush();
            Stdout_Push("1███████████████████████████████████████████████████████████████████████████████\n");
            Stdout_Flush();
            Stdout_Push("2███████████████████████████████████████████████████████████████████████████████\n");
            Stdout_Flush();
            Stdout_Push("3███████████████████████████████████████████████████████████████████████████████\n");
            Stdout_Flush();
            Stdout_Push("4███████████████████████████████████████████████████████████████████████████████\n");
            Stdout_Flush();
            Stdout_Push("5███████████████████████████████████████████████████████████████████████████████\n");
            Stdout_Flush();
            Stdout_Push("6███████████████████████████████████████████████████████████████████████████████\n");
            Stdout_Flush();
            Stdout_Push("7███████████████████████████████████████████████████████████████████████████████\n");
            Stdout_Flush();
        }

        switch (n)
        {
            case 0:
                // Clear screen from cursor down
                TTY_MoveCursor(TTY_CURSOR_UP, 5);
                Stdout_Push("[0J");
                Stdout_Flush();
            break;
            case 1:
                // Clear screen from cursor up
                TTY_MoveCursor(TTY_CURSOR_UP, 5);
                Stdout_Push("[1J");
                Stdout_Flush();
            break;
            case 2:
                // Clear screen
                Stdout_Push("[2J");
                Stdout_Flush();
            break;
        
            default:
            break;
        }

        TTY_MoveCursor(TTY_CURSOR_DOWN, 1);
        
        return;
    }

    if ((argc > 1) && (strcmp("-devinit", argv[1]) == 0))
    {
        DeviceManager_Init();
        return;
    }

    if ((argc > 1) && (strcmp("-colour", argv[1]) == 0))
    {
    Stdout_Push("\
[30m█[90m█\
[91m█[31m█\
[32m█[92m█\
[93m█[33m█\
[34m█[94m█\
[95m█[35m█\
[36m█[96m█\
[97m█[37m█\
[30m\n\r");

Stdout_Push("\n\
[90m█[30m█\
[31m█[91m█\
[92m█[32m█\
[33m█[93m█\
[94m█[34m█\
[35m█[95m█\
[96m█[36m█\
[37m█[97m█\
[30m\n\r");

Stdout_Push("\n\
[30m █ [90m█\
[31m █ [91m█\
[32m █ [92m█\
[33m █ [93m█\
[34m █ [94m█\
[35m █ [95m█\
[36m █ [96m█\
[37m █ [97m█\
[30m\n");

Stdout_Push(" \
[30m █ [90m█\
[31m █ [91m█\
[32m █ [92m█\
[33m █ [93m█\
[34m █ [94m█\
[35m █ [95m█\
[36m █ [96m█\
[37m █ [97m█\
[30m\n");

Stdout_Push("\n\
[30m█[90m█\
[31m█[91m█\
[32m█[92m█\
[33m█[93m█\
[34m█[94m█\
[35m█[95m█\
[36m█[96m█\
[37m█[97m█\
[30m");

Stdout_Push("\n\
[90m█[30m█\
[91m█[31m█\
[92m█[32m█\
[93m█[33m█\
[94m█[34m█\
[95m█[35m█\
[96m█[36m█\
[97m█[37m█\
[0m\n\r");

        return;
    }
}

void CMD_FlushBuffer(u8 argc, char *argv[])
{
    if (argc < 2) 
    {
        Stdout_Push("Flush buffer and set to 0\n\nUsage:\nbflush <buffer>\n\nBuffers available: rx, tx, stdout\n");
        return;
    }

    u16 i = BUFFER_LEN;

    if (strcmp(argv[1], "rx") == 0)
    {
        while (i--)
        {
            Buffer_Push(&RxBuffer, 0);
        }
        
        Buffer_Flush(&RxBuffer);
    }
    else if (strcmp(argv[1], "tx") == 0)
    {
        while (i--)
        {
            Buffer_Push(&TxBuffer, 0);
        }
        
        Buffer_Flush(&TxBuffer);
    }
    else if (strcmp(argv[1], "stdout") == 0)
    {
        while (i--)
        {
            Buffer_Push(&stdout, 0);
        }
        
        Buffer_Flush(&stdout);
    }
}

void CMD_PrintBuffer(u8 argc, char *argv[])
{
    if (argc < 3) 
    {
        Stdout_Push("Print byte at <position> in <buffer>\n\nUsage:\nbprint <buffer> <position>\n\nBuffers available: rx, tx, stdout\n");
        return;
    }

    char buf[8];

    if (strcmp(argv[1], "rx") == 0)
    {
        sprintf(buf, "$%X\n", RxBuffer.data[atoi16(argv[2]) % BUFFER_LEN]);
        Stdout_Push(buf);
    }
    else if (strcmp(argv[1], "tx") == 0)
    {
        sprintf(buf, "$%X\n", TxBuffer.data[atoi16(argv[2]) % BUFFER_LEN]);
        Stdout_Push(buf);
    }
    else if (strcmp(argv[1], "stdout") == 0)
    {
        sprintf(buf, "$%X\n", stdout.data[atoi16(argv[2]) % BUFFER_LEN]);
        Stdout_Push(buf);
    }
}

void CMD_Ping(u8 argc, char *argv[])
{
    if (argc < 2) 
    {
        Stdout_Push("Ping IP address\n\nUsage:\nping <address>\n");
        return;
    }

    NET_PingIP(argv[1]);
}


void CMD_Run(u8 argc, char *argv[])
{
    if (argc < 2) return;
    char *fn_buf = malloc(FILE_MAX_FNBUF);

    FS_ResolvePath(argv[1], fn_buf);

    void *proc = ELF_LoadProc(fn_buf);

    free(fn_buf);
    Stdout_Flush();

    if (proc == NULL) return;

    //SRAM_enableRO();
    kprintf("cmd proc: $%p", proc);

    asm("move.w #0x000, %sr");
    VAR2REG_L(proc, "a5");
    asm("jsr (%a5)");

    Stdout_Push("Returned from oblivion...\n");
    //SRAM_disable();
}

void CMD_ChangeDir(u8 argc, char *argv[])
{
    if (argc < 2) return;
    
    FS_ChangeDir(argv[1]);
}

void CMD_ListDir(u8 argc, char *argv[]) 
{
    if (argc > 1)
    {
        char tmp[64];
        sprintf(tmp, "%s", argv[1]);
        FS_ListDir(tmp);
    }
    else 
    {
        FS_ListDir(FS_GetCWD());
    }
}

void CMD_MoveFile(u8 argc, char *argv[])
{
    if (argc < 3) return;
    
    /* This is buggy
    char *fn_buf1, *fn_buf2;

    fn_buf1 = malloc(FILE_MAX_FNBUF);
    fn_buf2 = malloc(FILE_MAX_FNBUF);

    FS_ResolvePath(argv[1], fn_buf1);
    FS_ResolvePath(argv[2], fn_buf2);

    FS_Rename(fn_buf1, fn_buf2);
    
    free(fn_buf1);
    free(fn_buf2);*/

    CMD_CopyFile(argc, argv);

    char *fn_buf1, *fn_buf2;

    fn_buf1 = malloc(FILE_MAX_FNBUF);
    fn_buf2 = malloc(FILE_MAX_FNBUF);

    FS_ResolvePath(argv[1], fn_buf1);
    FS_ResolvePath(argv[2], fn_buf2);

    SM_File *f = F_Open(fn_buf2, FM_RDONLY);
    if (f == NULL)
    {
        stdout_printf("Failed to move file \"%s\" to \"%s\"\n", argv[1], argv[2]);
        goto OnExit;
    }

    F_Close(f);
    FS_Remove(fn_buf1);

    OnExit:
    free(fn_buf1);
    free(fn_buf2);

    return;
}

void CMD_CopyFile(u8 argc, char *argv[])
{
    if (argc < 3) return;

    char *fn_buf1, *fn_buf2;

    fn_buf1 = malloc(FILE_MAX_FNBUF);
    fn_buf2 = malloc(FILE_MAX_FNBUF);

    FS_ResolvePath(argv[1], fn_buf1);
    FS_ResolvePath(argv[2], fn_buf2);
    
    SM_File *f1 = F_Open(fn_buf1, FM_RDONLY);

    if (f1 == NULL)
    {
        stdout_printf("File \"%s\" does not exist\n", fn_buf1);
        goto OnExit;
    }

    SM_File *f2 = F_Open(fn_buf2, FM_CREATE | FM_WRONLY);

    if (f2 == NULL)
    {
        stdout_printf("Failed to create file \"%s\"\n", fn_buf2);
        goto OnExit;
    }

    F_Seek(f1, 0, SEEK_END);
    u16 size = F_Tell(f1);

    u8 *buf = (u8*)malloc(size);

    if (buf)
    {
        memset(buf, 0, size);
        F_Seek(f1, 0, SEEK_SET);
        F_Read(buf, size, 1, f1);
        F_Write(buf, size, 1, f2);
    }

    OnExit:
    free(buf);
    free(fn_buf1);
    free(fn_buf2);
    F_Close(f1);
    F_Close(f2);
    return;
}

void CMD_Concatenate(u8 argc, char *argv[]) 
{
    if (argc < 2)
    {
        return;
    }

    char *fn_buf = malloc(FILE_MAX_FNBUF);
    FS_ResolvePath(argv[1], fn_buf);

    SM_File *f = F_Open(fn_buf, FM_RDWR);
    free(fn_buf);

    if (f)
    {
        F_Seek(f, 0, SEEK_END);
        u16 size = F_Tell(f);

        char *buf = (char*)malloc(size+1);

        if (buf)
        {
            memset(buf, 0, size+1);
            F_Seek(f, 0, SEEK_SET);
            F_Read(buf, size, 1, f);

            Stdout_Push(buf);
            Stdout_Push("\n");
            Stdout_Flush();
        }

        free(buf);
        F_Close(f);
    }
}

void CMD_Touch(u8 argc, char *argv[]) 
{
    if (argc < 2)
    {
        Stdout_Push("Update file or create a new file if it does not exist\nNot enough arguments\n");
        return;
    }

    char *fn_buf = malloc(FILE_MAX_FNBUF);
    FS_ResolvePath(argv[1], fn_buf);

    SM_File *f = F_Open(fn_buf, FM_CREATE);
    F_Close(f);
    free(fn_buf);
}

void CMD_MakeDirectory(u8 argc, char *argv[]) 
{
    if (argc < 2)
    {
        Stdout_Push("Create a directory if it does not already exist\nNot enough arguments\n");
        return;
    }

    char *fn_buf = malloc(FILE_MAX_FNBUF);
    FS_ResolvePath(argv[1], fn_buf);

    FS_MkDir(fn_buf);
    free(fn_buf);
}

void CMD_RemoveLink(u8 argc, char *argv[]) 
{
    if (argc < 2)
    {
        Stdout_Push("Remove a file/directory\nNot enough arguments\n");
        return;
    }

    char *fn_buf = malloc(FILE_MAX_FNBUF);
    FS_ResolvePath(argv[1], fn_buf);

    FS_Remove(fn_buf);
    free(fn_buf);
}

void CMD_HexView(u8 argc, char *argv[])
{
    if (argc < 2)
    {
        stdout_printf("File HexViewer\n\nUsage: %s <filename>\n", argv[0]);
        return;
    }

    HexView_Open(argv[1]);
}

void CMD_Attr(u8 argc, char *argv[])
{
    //if (argc < 4)
    {
        stdout_printf("Modifiy file attributes\n\nUsage: %s -s <attribute> <filename>\n       %s -g <filename>\n\nNOT IMPLEMENTED\n", argv[0], argv[0]);
        return;
    }
}

void CMD_ListBlock(u8 argc, char *argv[])
{
    FS_PrintBlockDevices();
}

void CMD_Uptime(u8 argc, char *argv[])
{
    SM_Time t = SecondsToDateTime(SystemUptime);

    stdout_printf("%02lu:%02u:%02u up %lu days, %lu:%02u\n", SystemTime.hour, SystemTime.minute, SystemTime.second, t.day-1, t.hour, t.minute);
}

void CMD_Date(u8 argc, char *argv[])
{
    if ((argc > 1) && (strcmp("-sync", argv[1]) == 0))
    {
        char *sync_server;

        if (argc > 2) sync_server = argv[2];
        else sync_server = sv_TimeServer;

        u8 r = DoTimeSync(sync_server);

        switch (r)
        {
            case 1:
                stdout_printf("Connection to %s failed\n", sync_server);
            break;
            case 2:
                stdout_printf("Time was synchronized too recently.\n");
            break;
        
            default:
            break;
        }
    }
    else if ((argc > 1) && (strcmp("-help", argv[1]) == 0))
    {

        Stdout_Push("Show/Set date and time\n\nUsage:\n\
date -sync  - Synchronize date & time\n\
date -help  - This screen\n\
date        - Show date and time\n");
    }
    else stdout_printf("%lu-%lu-%lu %02lu:%02u:%02u\n", SystemTime.day, SystemTime.month, SystemTime.year, SystemTime.hour, SystemTime.minute, SystemTime.second);
}

void CMD_About(u8 argc, char *argv[])
{
    if (sv_Font)
    {
        Stdout_Push("\n SMDTC - a dumb project created by smds\n");
        Stdout_Push(" Copyright (c) 2024 smds\n");
        Stdout_Push(" See SMDTC github for more info:\n");
        Stdout_Push(" [36mgithub.com/SweMonkey/smdt[0m\n\n");

        Stdout_Push(" This project incorporates some code by b1tsh1ft3r\n");
        Stdout_Push(" Copyright (c) 2023 B1tsh1ft3r\n");
        Stdout_Push(" See retro.link github for more info:\n");
        Stdout_Push(" [36mgithub.com/b1tsh1ft3r/retro.link[0m\n\n");

        Stdout_Push(" This project makes use of littleFS\n");
        Stdout_Push(" Copyright (c) 2022, The littlefs authors.\n");
        Stdout_Push(" Copyright (c) 2017, Arm Limited. All rights reserved.\n");
        Stdout_Push(" See littleFS github for more info:\n");
        Stdout_Push(" [36mgithub.com/littlefs-project/littlefs[0m\n");
    }
    else
    {
        Stdout_Push("SMDTC - a dumb project created by smds\n");
        Stdout_Push("Copyright (c) 2024 smds\n");
        Stdout_Push("See SMDTC github for more info:\n");
        Stdout_Push("[36mgithub.com/SweMonkey/smdt[0m\n\n");

        Stdout_Push("This project incorporates some code -\n - by b1tsh1ft3r\n");
        Stdout_Push("Copyright (c) 2023 B1tsh1ft3r\n");
        Stdout_Push("See retro.link github for more info:\n");
        Stdout_Push("[36mgithub.com/b1tsh1ft3r/retro.link[0m\n\n");

        Stdout_Push("This project makes use of littleFS\n");
        Stdout_Push("Copyright (c) 2022, The littlefs authors");
        Stdout_Push("Copyright (c) 2017, Arm Limited. -\n - All rights reserved\n");
        Stdout_Push("See littleFS github for more info:\n");
        Stdout_Push("[36mgithub.com/littlefs-project/littlefs[0m\n");
    }
}
