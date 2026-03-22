#include "CMDFunc.h"
#include "StateCtrl.h"
#include "Utils.h"
#include "Network.h"
#include "IRC.h"
#include "Telnet.h"
#include "Terminal.h"
#include "ConfigFile.h"
#include "WinMgr.h"
#include "Palette.h"
#include "HTTP_Webserver.h"
#include "SwRenderer.h"

#include "devices/XP_Network.h"
#include "devices/Keyboard_PS2.h"

#include "misc/VarList.h"

#include "system/PseudoFile.h"
#include "system/Time.h"
#include "system/File.h"
#include "system/Filesystem.h"
#include "system/ELF_ldr.h"
#include "system/StatusBar.h"


SM_CMDList CMDList[] =
{
    {"telnet",  CMD_LaunchTelnet,   "<address:port>"},
    {"irc",     CMD_LaunchIRC,      "<address:port>"},
    {"gopher",  CMD_LaunchGopher,   "<address>"},
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
    {"psgbeep", CMD_PSGBeep,        "- Play PSG beep"},
    {"webserv", CMD_HTTPWeb,        "- Start HTTP webserver"},
    {"ans",     CMD_ViewANS,        "- Print ANSI file"},
    {"type",    CMD_ViewANS,        "- Print file"},
    {"about",   CMD_About,          "- About SMDT/Licenses"},
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

void CMD_LaunchGopher(u8 argc, char *argv[]) 
{
    #ifndef EMU_BUILD
    if (argc < 2)
    {
        printf("\e[91mNo address specified!\e[0m\n");
        return;
    }
    #endif

    ChangeState(PS_Gopher, argc, argv);
}

void CMD_SetAttr(u8 argc, char *argv[])
{
    switch (argc)
    {
        case 2:
            printf("\e[%um\n", atoi(argv[1]));
        break;
        case 3:
            printf("\e[%u;%um\n", atoi(argv[1]), atoi(argv[2]));
        break;
    
        default:
            printf("Set terminal attribute\n\nUsage:\n");
            printf("%s <number> <number>\n", argv[0]);
            printf("%s <number>\n\n", argv[0]);
        return;
    }

    printf("Attributes set.\n\n");
}

void CMD_Echo(u8 argc, char *argv[]) 
{
    if (argc < 2)
    {
        printf("Usage:\necho [string or $variable]... [> filename]\n\n");
        return;
    }

    char *buffer = malloc(256);

    if (buffer == NULL) return;

    memset(buffer, 0, 256);

    // Iterate through the arguments and print them
    for (int i = 1; i < argc; i++)
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
        if (i < argc - 1)
        {
            strncat(buffer, " ", 256);
        }
    }

    printf("%s\n\n", buffer);
    free(buffer);

    return;
}

void CMD_KeyboardSend(u8 argc, char *argv[])
{
    if (argc < 2) 
    {
        printf("Send command to keyboard\n\nUsage:\n");
        printf("%s <byte 1> <byte 2> <...>\n\n", argv[0]);
        printf("Byte= decimal number between 0 and 255\n\n");
        return;
    }

    u8 kbcmd;
    u8 ret;

    for (u8 i = 1; i < argc; i++)
    {
        kbcmd = atoi(argv[i]);
        ret = 0;

        printf("Sending command $%X to keyboard...\n", kbcmd);
        
        ret = KB_PS2_SendCommand(kbcmd);
        
        printf("Recieved byte $%X from keyboard   \n", ret);
        waitMs(2);
    }
    printf("\n");
}

void CMD_Help(u8 argc, char *argv[])
{
    u16 i = 0;
    
    printf("Commands available:\n\n");

    while (CMDList[i].id != 0)
    {
        printf("%10s %-28s\n", CMDList[i].id, CMDList[i].desc);

        i++;
    }
    printf("\n");
}

void CMD_xport(u8 argc, char *argv[])
{
    if (argc == 1)
    {
        printf("xPort debug\n\nUsage:\n\
xport enter       - Enter monitor mode\n\
xport exit        - Exit monitor mode\n\
xport <string>    - Send string to xPort\n\
xport connect <address>\n\
xport disconnect  - Close connection\n\n");
        return;
    }

    if ((argc > 1) && (strcmp(argv[1], "enter") == 0))
    {
        printf("Entering monitor mode...\n");
        Stdout_Flush();

        bool r = XPN_EnterMonitorMode();

        if (r) printf("OK!\n");
        else printf("Timeout!\n");
    }
    else if ((argc > 1) && (strcmp(argv[1], "exit") == 0))
    {
        printf("Exiting monitor mode...\n");
        Stdout_Flush();
        
        bool r = XPN_ExitMonitorMode();

        if (r) printf("OK!\n");
        else printf("Timeout!\n");
    }
    else if ((argc > 2) && (strcmp(argv[1], "connect") == 0))
    {
        printf("Connecting to %s ...\n", argv[2]);
        Stdout_Flush();

        bool r = XPN_Connect(argv[2]);

        if (r) printf("Connected!\n");
        else printf("Error!\n");
    }
    else if ((argc > 1) && (strcmp(argv[1], "disconnect") == 0))
    {
        printf("Disconnecting... \n");
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

        printf("Sending string:\n\"%s\"\n", buf);
        Stdout_Flush();

        XPN_SendMessage(buf);
    }
    printf("\n");
}

void CMD_UName(u8 argc, char *argv[])
{
    const char *Krnl_Str = STATUS_TEXT_SHORT;
    const char *OS_Str = STATUS_TEXT_SHORT;
    const char *Mach_Str = "m68k";
    //const char *Node_Str = "Local";
    const char *Mach_Name = "None";

    if (argc == 1)
    {
        printf("%s\n\n", OS_Str);
        return;
    }

    bool show_kernel_name = 0;
    bool show_node_name = 0;
    bool show_kernel_release = 0;
    bool show_kernel_version = 0;
    bool show_machine = 0;
    bool show_processor = 0;
    bool show_os = 0;
    bool show_all = 0;

    // Parse command-line arguments
    for (u8 i = 1; i < argc; i++) 
    {
        if (strcmp(argv[i], "-s") == 0) 
        {
            show_kernel_name = 1;
        } 
        else if (strcmp(argv[i], "-n") == 0) 
        {
            show_node_name = 1;
        } 
        else if (strcmp(argv[i], "-r") == 0) 
        {
            show_kernel_release = 1;
        } 
        else if (strcmp(argv[i], "-v") == 0) 
        {
            show_kernel_version = 1;
        } 
        else if (strcmp(argv[i], "-m") == 0) 
        {
            show_machine = 1;
        } 
        else if (strcmp(argv[i], "-p") == 0) 
        {
            show_processor = 1;
        } 
        else if (strcmp(argv[i], "-o") == 0) 
        {
            show_os = 1;
        } 
        else if (strcmp(argv[i], "-a") == 0) 
        {
            show_all = 1;
        } 
        else 
        {
            printf("%s\n\n", OS_Str);
            return;
        }
    }

    // Handle the case where "-a" is specified (implies all options)
    if (show_all) 
    {
        show_kernel_name = 1;
        show_node_name = 1;
        show_kernel_release = 1;
        show_kernel_version = 1;
        show_machine = 1;
        show_processor = 1;
        show_os = 1;
    }

    if (show_kernel_name) 
    {
        printf("%s ", Krnl_Str);
    }
    if (show_node_name) 
    {
        printf("%s ", sv_Username); // Node_Str
    }
    if (show_kernel_release) 
    {
        printf("%s ", STATUS_VER_STR);
    }
    if (show_kernel_version) 
    {
        printf("%s %s ", __DATE__, __TIME__);
    }
    if (show_machine)
    {
        printf("%s ", Mach_Name);
    }
    if (show_processor) 
    {
        printf("%s ", Mach_Str);
    }
    if (show_os) 
    {
        printf("%s ", OS_Str);
    }

    printf("\n\n");
}

void CMD_SetConn(u8 argc, char *argv[])
{
    if (argc < 2) 
    {
        printf("Set connection time out\n\nUsage:\nsetcon <number of ticks>\n\n");
        printf("Current time out: %lu ticks\n\n", sv_ConnTimeout);
        return;
    }

    sv_ConnTimeout = atoi32(argv[1]);

    printf("Connection time out set to %lu\n\n", sv_ConnTimeout);
}

void CMD_ClearScreen(u8 argc, char *argv[])
{
    TTY_Init(TF_ClearScreen | TF_ResetVariables);

    // Hack: Move the cursor up once because the shell always autoinserts a newline upon running a command, which we don't want when running this command
    //TTY_MoveCursor(TTY_CURSOR_UP, 1);
}

void CMD_SetVar(u8 argc, char *argv[])
{
    if ((argc < 3) && (strcmp(argv[1], "-list")))
    {
        printf("Set variable\n\nUsage:\n");
        printf("setvar <variable_name> <value>\n");
        printf("setvar -list\n");
        printf("setvar -list <variable_name>\n\n");
        return;
    }
    else if ((argc >= 2) && (strcmp(argv[1], "-list") == 0))
    {
        u16 i = 0;
        printf("%-12s %s   %s\n\n", "Name", "Type", "Value");

        while (VarList[i].size)
        {
            if ( ((argc >= 3) && (strcmp(VarList[i].name, argv[2]) == 0)) || (argc == 2))
            {
            switch (VarList[i].size)
            {
                case ST_BYTE:
                    printf("%-12s u8     %u\n", VarList[i].name, *((u8*)VarList[i].ptr));
                break;
                case ST_WORD:
                    printf("%-12s u16    %u\n", VarList[i].name, *((u16*)VarList[i].ptr));
                break;
                case ST_LONG:
                    printf("%-12s u32    %lu\n", VarList[i].name, *((u32*)VarList[i].ptr));
                break;        
                case ST_SPTR:
                    printf("%-12s StrPtr \"%s\"\n", VarList[i].name, (char*)VarList[i].ptr);
                break;
                case ST_SARR:
                    printf("%-12s StrArr \"%s\"\n", VarList[i].name, (char*)VarList[i].ptr);
                break;
                default:
                break;
            }
            }

            i++;
        }

        printf("\n\e[96mNote: Changes to variables may not take effect until you save and reboot.\e[0m\n\n");

        return;
    }
    else if (argc >= 3)
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
        memset(ipstr, 0, 32);
        //printf("Please wait...\n");

        u8 r = NET_GetIP(ipstr);

        switch (r)
        {
            case 0:
                printf("IP: %s\n\n", ipstr);
            break;

            case 1:
                printf("Error: Generic error\n\n");
            break;

            case 2:
                printf("Error: Timed out\n\n");
            break;
        
            default:
                printf("Error: Unknown\n\n");
            break;
        }

        free(ipstr);

        return;
    }

    printf("Error: Out of RAM!\n\n");
}

void CMD_Free(u8 argc, char *argv[])
{
    if ((argc > 1) && (strcmp(argv[1], "-defrag") == 0))
    {
        MEM_pack();
        printf("Defrag complete.\n\n");
        return;
    }

    if (bMegaCD)
    {
        printf("\n─ Mega Drive ──────────────────────────\n");
    }

    // May need an "else print \n" here?

    printf("%20s %5u bytes\n", "Free:", MEM_getFree());
    printf("%20s %5u bytes\n", "Largest free block:", MEM_getLargestFreeBlock());
    printf("%20s %5u bytes\n", "Used:", MEM_getAllocated());
    printf("%20s %5u bytes\n", "System reserved:", 65536-(MEM_getFree()+MEM_getAllocated()));

    u16 a = MEM_getFree()/1650;
    u16 b = MEM_getAllocated()/1650;
    u16 c = (65536/1650)-a-b;

    printf("\n\e[32m");
    for (u8 i = 0; i < a; i++) printf("█");    // Total Free
    printf("\e[31m");
    for (u8 i = 0; i < b; i++) printf("█");    // Used
    printf("\e[94m");
    for (u8 i = 0; i < c; i++) printf("▒");    // System reserved
    printf("\e[0m\n");


    if (bMegaCD)
    {
        printf("\n─ %s ─────────────────────────────\n", bPALSystem ? "Sega CD" : "Mega CD");

        printf("%20s %6u bytes\n", "Free:", 524288);
        printf("%20s %6u bytes\n", "Largest free block:", 524288);
        printf("%20s %6u bytes\n", "Used:", 0);

        a = 524288/13200;
        b = 0;
        c = (524288/13200)-a-b;

        printf("\n\e[32m");
        for (u8 i = 0; i < a; i++) printf("█");
        printf("\e[31m");
        for (u8 i = 0; i < b; i++) printf("█");
        printf("\e[0m\n");
    }
    
    printf("\n─ Info ────────────────────────────────\n");

    printf("Run \"%s -defrag\" for memory defrag.\n\n", argv[0]);
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
    if ((argc > 1) && (strcmp("-timing_im", argv[1]) == 0))
    {
        u32 frame = 0;  // Frames elapsed
        u16 num = 0;    // Number of characters printed

        u16 hv = 0;                         // Last hv counter value
        u16 hvc = 0;                        // Current hv counter value
        u16 fc = bPALSystem ? 250 : 300;    // How many frames we should run the test for
        u16 nf = bPALSystem ? 50 : 60;      // How many frames there are in a second

        SYS_disableInts();
        
        while (frame < fc)
        {
            TTY_PrintChar('A');
            num++;

            hvc = *((vu16*)(VDP_HVCOUNTER_PORT));

            if (hv < hvc)
            {
                hv = hvc;
            }
            else if (hv - hvc > 51200)// if (hv > hvc)
            {
                hv = 0;
                frame++;
                TTY_SetSX(0);
            }
        }

        SYS_enableInts();
        printf("\nManaged to print %u characters in %lu seconds (Insert mode ON)\n = %lu characters/s\n = %lu characters/frame\n\n", num, frame/nf, num/(frame/nf), num/frame);
        kprintf("\nManaged to print %u characters in %lu seconds (Insert mode ON)\n = %lu characters/s\n = %lu characters/frame\n\n", num, frame/nf, num/(frame/nf), num/frame);
        return;
    }

    if ((argc > 1) && (strcmp("-timing", argv[1]) == 0))
    {
        u32 frame = 0;  // Frames elapsed
        u16 num = 0;    // Number of characters printed

        u16 hv = 0;                         // Last hv counter value
        u16 hvc = 0;                        // Current hv counter value
        u16 fc = bPALSystem ? 250 : 300;    // How many frames we should run the test for
        u16 nf = bPALSystem ? 50 : 60;      // How many frames there are in a second

        while (frame < fc)
        {
            num++;
            TTY_PrintChar('A');

            hvc = *((vu16*)(VDP_HVCOUNTER_PORT));

            if (hv < hvc)
            {
                hv = hvc;
            }
            else if (hv - hvc > 51200)// if (hv > hvc)
            {
                hv = 0;
                frame++;
            }
        }

        printf("\nManaged to print %u characters in %lu seconds (Insert mode OFF)\n = %lu characters/s\n = %lu characters/frame\n\n", num, frame/nf, num/(frame/nf), num/frame);
        kprintf("\nManaged to print %u characters in %lu seconds (Insert mode OFF)\n = %lu characters/s\n = %lu characters/frame\n\n", num, frame/nf, num/(frame/nf), num/frame);
        return;
    }

    if ((argc > 1) && (strcmp("-blank", argv[1]) == 0))
    {
        for (u16 i = 0; i < 2560; i++)
        {
            TELNET_ParseRX(' ');
        }
        return;
    }

    if ((argc > 1) && (strcmp("-blanke", argv[1]) == 0))
    {
        for (u16 i = 0; i < 2560; i++)
        {
            TELNET_ParseRX('E');
        }
        return;
    }
    
    if ((argc > 1) && (strcmp("-colour2", argv[1]) == 0))
    {
        printf("Standard colours  - Red FG,   Black BG:      \e[38;5;1mTEST!\e[0m\n");
        printf("Standard colours  - White FG, Bright red BG: \e[48;5;9mTEST!\e[0m\n\n");

        printf("6x6x6 colour cube - Blue FG,  Black BG:      \e[38;5;21mTEST!\e[0m\n");
        printf("6x6x6 colour cube - White FG, Blue BG:       \e[48;5;21mTEST!\e[0m\n");

        printf("6x6x6 colour cube - Blue FG, Pink BG:        \e[38;5;21;48;5;201mTEST!\e[0m\n\n");


        /*printf("RGB24             - Red FG,   Black BG:      \e[38;2;128;0;0mTEST!\e[0m\n");
        printf("RGB24             - White FG, Red BG:        \e[48;2;128;0;0mTEST!\e[0m\n");

        printf("RGB24             - Green FG, Black BG:      \e[38;2;0;128;0mTEST!\e[0m\n");
        printf("RGB24             - White FG, Green BG:      \e[48;2;0;128;0mTEST!\e[0m\n");

        printf("RGB24             - Blue FG, Black BG:       \e[38;2;0;0;128mTEST!\e[0m\n");
        printf("RGB24             - White FG, Blue BG:       \e[48;2;0;0;128mTEST!\e[0m\n");

        printf("RGB24             - BRed FG,  Black BG:      \e[38;2;192;0;0mTEST!\e[0m\n");
        printf("RGB24             - White FG, BRed BG:       \e[48;2;192;0;0mTEST!\e[0m\n");

        printf("RGB24             - BGreen FG, Black BG:     \e[38;2;0;192;0mTEST!\e[0m\n");
        printf("RGB24             - White FG,  BGreen BG:    \e[48;2;0;192;0mTEST!\e[0m\n");

        printf("RGB24             - BBlue FG, Black BG:      \e[38;2;0;0;192mTEST!\e[0m\n");
        printf("RGB24             - White FG, BBlue BG:      \e[48;2;0;0;192mTEST!\e[0m\n");*/

        printf("RGB24             - Bright red FG, Black BG: \e[38;2;212;20;70mTEST!\e[0m\n");
        printf("RGB24             - White FG, Bright red BG: \e[48;2;212;20;70mTEST!\e[0m\n\n");

        printf("\e[0;5;31;42mTEST!\e[0m\n");
        printf("\e[0;37;40;1mTEST!\e[0m\n");
        printf("\e[0;37;1mTEST!\e[0m\n");
        printf("\e[0;37mTEST!\e[0m\n");
        printf("\e[37mTEST!\e[0m\n");
        printf("\e[31m\e[0;32;40mGreen text on black BG\e[0m\n\n");

        printf("\e[0mA\e[1;30;47mB\e[40mC    \e[0mD\n\n");

        printf("RGB24 explicit components: \e[38;2;120;100;240;48;2;200;200;200mTEST!\e[0m\n\n");        
        
        return;
    }
    
    ///printf("\e[91mWarning: The test command may cause side effects on SMDT operation.\nIt may outright crash your system depending on the parameters given!\e[0m\n");

    /*if ((argc > 1) && (strcmp("-test1arg", argv[1]) == 0))
    {
        return;
    }

    if ((argc > 2) && (strcmp("-test2arg", argv[1]) == 0))
    {
        return;
    }*/

    if ((argc > 1) && (strcmp("-www_create", argv[1]) == 0))
    {
        char index200[] = "<!doctype html><html lang=\"en\"><head><meta http-equiv=\"Content-Type\" content=\"text/html;charset=ISO-8859-1\"><title>SMDT WebServer</title><style>body{min-height: 100vh;display: flex;flex-direction: column;}footer{margin-top: auto;text-align: center;}</style></head><body><h1>Hello world from a http webserver running on a Mega Drive!</h1><br><br><p>This is the default index page located at /sram/www/index.html</p><footer><p>SMDT HTTP WebServer v1.0</p></footer></body></html>";
        char index404[] = "<!doctype html><html lang=\"en\"><head><meta http-equiv=\"Content-Type\" content=\"text/html;charset=ISO-8859-1\"><title>404 Page Not Found</title><style>body{min-height: 100vh;display: flex;flex-direction: column;}footer{margin-top: auto;text-align: center;}</style></head><body><center><h1><br><br>404 Page not found</h1></center><footer><p>SMDT HTTP WebServer v1.0</p></footer></body></html>";
        SM_File *f;

        FS_MkDir("/sram/www");

        f = F_Open("/sram/www/index.html", FM_CREATE | FM_RDWR);
        F_Write(index200, strlen(index200), 1, f);
        F_Close(f);

        f = F_Open("/sram/www/404.html", FM_CREATE | FM_RDWR);
        F_Write(index404, strlen(index404), 1, f);
        F_Close(f);

        return;
    }

    if ((argc > 1) && (strcmp("-www_test", argv[1]) == 0))
    {        
        char index200[] = "<html><head><title>SMDT WebServer</title><style>body{min-height: 100vh;display: flex;flex-direction: column;}footer{margin-top: auto;text-align: center;}</style></head><body><h1>SMDT Webserver</h1><br><br><img src=\"favicon.ico\" alt=\"favicon goes here\"><footer><p>SMDT HTTP WebServer v1.0</p></footer></body></html>";
        SM_File *f;

        FS_MkDir("/sram/www");

        f = F_Open("/sram/www/image_test.html", FM_CREATE | FM_RDWR);
        F_Write(index200, strlen(index200), 1, f);
        F_Close(f);

        return;
    }

    if ((argc > 1) && (strcmp("-force_xport", argv[1]) == 0))
    {
        printf("Forcing xport...\n");

        DRV_UART.Id.sName = "xPort UART";

        *((vu8*) DRV_UART.SCtrl) = 0x38;

        DEV_SetCtrl(DRV_UART, 0x40);
        DEV_ClrData(DRV_UART);

        bXPNetwork = TRUE;

        NET_SetConnectFunc(XPN_Connect);
        NET_SetDisconnectFunc(XPN_Disconnect);
        NET_SetGetIPFunc(XPN_GetIP);
        NET_SetPingFunc(XPN_PingIP);
    }
    
    if ((argc > 1) && (strcmp("-bktxt", argv[1]) == 0))
    {
        Stdout_Flush();
        sv_Font = 0;
        sv_BoldFont = FALSE;
        TTY_Init(TF_ClearScreen | TF_ReloadFont | TF_ResetVariables);
        TRM_SetWinHeight(0);
        SetColor( 0, 0x8E6);
        SetColor( 4, 0);
        SetColor(17, 0x8E6);
        SetColor(50, 0x8E6);
        SetColor(39, 0);
        printf("────────────────────────────────────────\n\n");
        printf(" SEGA SC-3000 BASIC Level 3 ver 1.0   \n\n");
        printf("    Export Version With Diereses      \n\n");
        printf("     Copyright 1983 (C) by NITEC      \n\n");
        printf("────────────────────────────────────────\n\n");
        printf(" %u Bytes free\n", MEM_getFree());
        printf("Ready\n\r");

        Stdout_Flush();

        while (1)
        {
            SYS_doVBlankProcess();
        }        

        return;
    }

    if ((argc > 3) && (strcmp("-setcl", argv[1]) == 0))
    {
        u16 idx = atoi16(argv[2]);
        u16 cl  = atoi16(argv[3]);
        SetColor(idx, cl);
        return;
    }

    if ((argc > 1) && (strcmp("-redraw", argv[1]) == 0))
    {
        SW_RedrawScreen();
    }

    if ((argc > 1) && (strcmp("-bse", argv[1]) == 0))
    {
        u32 delay = 1000;
        printf("This should type out a string, backspace once and then clear a single character (under cursor) and repeat until the string is erased\n");

        printf("asdasd");
        Stdout_Flush();
        waitMs(delay);
        printf("\b");
        Stdout_Flush();
        waitMs(delay);
        printf("\033[K");
        Stdout_Flush();
        waitMs(delay);
        printf("\b");
        Stdout_Flush();
        waitMs(delay);
        printf("\033[K");
        Stdout_Flush();
        waitMs(delay);
        printf("\b");
        Stdout_Flush();
        waitMs(delay);
        printf("\033[K");
        Stdout_Flush();
        waitMs(delay);
        printf("\b");
        Stdout_Flush();
        waitMs(delay);
        printf("\033[K");
        Stdout_Flush();
        waitMs(delay);
        printf("\b");
        Stdout_Flush();
        waitMs(delay);
        printf("\033[K");
        Stdout_Flush();
        waitMs(delay);
        printf("\b");
        Stdout_Flush();
        waitMs(delay);
        printf("\033[K");
        Stdout_Flush();
        waitMs(delay);
        printf("\n\n");
        return;
    }

    if ((argc > 1) && (strcmp("-et2", argv[1]) == 0))
    {
        TELNET_ParseRX('\e');
        TELNET_ParseRX('#');
        TELNET_ParseRX('8');
        return;
    }

    if ((argc > 1) && (strcmp("-et3", argv[1]) == 0))
    {
        printf("\e[636;0;1;1;1;1*y");
       
        return;
    }

    if ((argc > 1) && (strcmp("-et4", argv[1]) == 0))
    {
        printf("\e[50M");
        printf("\e[M");
       
        return;
    }

    if ((argc > 1) && (strcmp("-etT", argv[1]) == 0))
    {
        kprintf("-- >0;1T --");
        printf("\e[>0;1T");
        kprintf("-- >T --");
        printf("\e[>T");
        return;
    }

    if ((argc > 1) && (strcmp("-et1", argv[1]) == 0))
    {
        /*printf("\e[1\"q");
        printf("\e[s");
        printf("\e[0\"q");
        printf("\e[u");*/
        
        printf("\e[65;1\"p");
        printf("\e[!p");
        printf("\e[8;25;80t");
        printf("\e[?1047l");
        printf("\e[?1049l");
        printf("\e[?47l");
        printf("\e[?69l");
        printf("\e[4l");
        printf("\e[20l");
        printf("\e[?7h");
        printf("\e[?41l");
        printf("\e[>0;1T");
        printf("\e[>2;3t");
        printf("\e[2J");
        printf("\e[23;0t");
        printf("\e[23;0t");
        printf("\e[23;0t");
        printf("\e[23;0t");
        printf("\e[23;0t");
        printf("\e[3g");
        printf("\e[18t");
        //ReadCSI parameters: 8;25;80
        printf("\e[1;1H");
        printf("\e[1;9H");
        printf("\e[1;17H");
        printf("\e[1;25H");
        printf("\e[1;33H");
        printf("\e[1;41H");
        printf("\e[1;49H");
        printf("\e[1;57H");
        printf("\e[1;65H");
        printf("\e[1;73H");
        printf("\e[1;1H");
        printf("\e[1t");
        printf("\e]104;\e\\");
        printf("\e]10;#000\e\\");
        printf("\e]11;#ffffff\e\\");
        printf("\e[1\"q");
        printf("\e[?1048h");
        printf("\e[0\"q");
        printf("\e[?1048l");
        printf("\e[1;1;1;1${");
        printf("\e[1050;0;1;1;1;1*y");
        //Read response: <ESC>P1050!~FFE0<ESC>

        return;
    }

    if ((argc > 2) && (strcmp("-ftitle", argv[1]) == 0))
    {
        SB_SetStatusText(argv[2]);
        SB_ResetStatusText();
        return;
    }

    if ((argc > 1) && (strcmp("-dmouse", argv[1]) == 0))
    {
        bMouse = FALSE;
        return;
    }

    if ((argc > 1) && (strcmp("-dmclock", argv[1]) == 0))
    {
        printf("\e[91mWarning: Z80 & PSG clock is now set to 7.6 MHz\e[0m\n\r");
        *((vu32*) 0xC00018) = 0x100;   // Set 
        *((vu16*) 0xC0001C) = 1;       // Set 
        return;
    }

    if ((argc > 1) && (strcmp("-illegal", argv[1]) == 0))
    {
        asm("illegal");
        return;
    }

    if ((argc > 1) && (strcmp("-lbrk", argv[1]) == 0))
    {
        printf("Testing linebreak/wraparound...\n\n");

        printf("This is a long string of text that should wrap around exactly at column 80, somewhere around here... and no letters should be missing in the word \"somewhere\"\n\n");

        printf("This is another long string that should have another line of text under it witho\nut any space inbetween them\n\n");

        printf("\e[15;1H--------------------------------------------------------------------------------\r\n\e[16;1HThere should be no empty line above this text!\n");

        return;
    }

    if ((argc > 1) && (strcmp("-crp", argv[1]) == 0))
    {
        printf("Testing cursor reporting, check kdebug output with ESC_LOGGING enabled\nOutput will be in the following format: [y;xR\n\n");
        printf("\e[s\e[255B\e[255C\e[6n\e[u");

        return;
    }

    if ((argc > 1) && (strcmp("-cuf1x", argv[1]) == 0))
    {
        printf("Testing cursor reporting, check kdebug output with ESC_LOGGING enabled\nResult should be: [y;80R\n\n");
        printf("\e[?69h\e[5;10s\e[3;12H\e[6n\e[80C\e[6n");

        return;
    }

    if ((argc > 1) && (strcmp("-cuf2x", argv[1]) == 0))
    {
        printf("Testing cursor reporting, check kdebug output with ESC_LOGGING enabled\nResult should be: [y;10R\n\n");
        printf("\e[?69h\e[5;10s\e[3;7H\e[80C\e[6n");

        return;
    }

    if ((argc > 1) && (strcmp("-src", argv[1]) == 0))
    {
        printf("\e[1\"q");
        printf("\e[0\"q");

        return;
    }

    if ((argc > 1) && (strcmp("-sbres", argv[1]) == 0))
    {
        SB_ResetStatusBar();
        return;
    }

    if ((argc > 2) && (strcmp("-clearv", argv[1]) == 0))
    {
        printf("\e[2J\e[1;1H1   \n2   \n3   \n4   \n5   \n6   \n7   \n8   \n9   \n10  \n11  \n12  \n13  \n14  \n15  \n16  \n17  \n18  \n19  \n20  \n21  \n22  \n");
        Stdout_Flush();
        u8 n = atoi(argv[2]);
        n = n ? n : 1;
        SW_ClearLine(14, n);

        kprintf("Clearing lines 14 -> %u", 14 + n);
    }

    if ((argc > 1) && (strcmp("-clearp", argv[1]) == 0))
    {
        printf("\e[2J\e[1;1H0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF\n");
        printf(              "1111111111111111111111111111111111111111|111111111111111111111111111111111111111\n");
        printf(              "222222222222222222222222222222222222222|2222222222222222222222222222222222222222\n");
        printf(              "|3333333333333333333333333333333333333333|33333333333333333333333333333333333333\n");
        printf(              "44444444444444444444444444444444444444|4444444444444444444444444444444444444444|\n");
        printf(              "55555555555555555555555555555555555555|55555555555555555555555555555555555555555\n");
        printf(              "666666666666666666666666666666666666666|666666666666666666666666666666666666666|\nEnd\n");
        Stdout_Flush();

        waitMs(250);
        SW_ClearPartialLine(2, 0, 40);
        waitMs(250);
        SW_ClearPartialLine(3, 40, 80);
        waitMs(250);
        SW_ClearPartialLine(4, 1, 41);
        waitMs(250);
        SW_ClearPartialLine(5, 39, 79);
        waitMs(250);
        SW_ClearPartialLine(6, 39, 80);
        waitMs(250);
        SW_ClearPartialLine(7, 40, 79);
        waitMs(250);
    }
    
    if ((argc > 1) && (strcmp("-fpstdout", argv[1]) == 0))
    {
        F_Printf(stdout, "Hello, is this thing on? %s\n", "maybe...");
        return;
    }

    if ((argc > 1) && (strcmp("-fptx", argv[1]) == 0))
    {
        F_Printf(tty_out, "Hello, is this thing on? %s\n", "maybe...");
        return;
    }

    if ((argc > 2) && (strcmp("-buf", argv[1]) == 0))
    {
        u8 n = atoi(argv[2]);

        kprintf("n= %u", n);

        if (n == 1) printf("\e[?1049h");
        else printf("\e[?1049l");

        Stdout_Flush();

        return;
    }

    if ((argc > 1) && (strcmp("-E", argv[1]) == 0))
    {
        printf("\e#8");
        return;
    }

    if ((argc > 1) && (strcmp("-cr", argv[1]) == 0))
    {
        printf("\e[255;0H\e[79Ca");
        return;
    }

    if ((argc > 1) && (strcmp("-lz", argv[1]) == 0))
    {
        printf("\e[2J\e[1;1HTest of leading zeros in ESC sequences\n\rTwo lines below you should see the sentence \"This is a correct sentence\".\e[00000000004;000000001HT\e[00000000004;000000002Hh\e[00000000004;000000003Hi\e[00000000004;000000004Hs\e[00000000004;000000005H \e[00000000004;000000006Hi\e[00000000004;000000007Hs\e[00000000004;000000008H \e[00000000004;000000009Ha\e[00000000004;0000000010H \e[00000000004;0000000011Hc\e[00000000004;0000000012Ho\e[00000000004;0000000013Hr\e[00000000004;0000000014Hr\e[00000000004;0000000015He\e[00000000004;0000000016Hc\e[00000000004;0000000017Ht\e[00000000004;0000000018H \e[00000000004;0000000019Hs\e[00000000004;0000000020He\e[00000000004;0000000021Hn\e[00000000004;0000000022Ht\e[00000000004;0000000023He\e[00000000004;0000000024Hn\e[00000000004;0000000025Hc\e[00000000004;0000000026He\e[5;1H\n");
        Stdout_Flush();
        return;
    }

    if ((argc > 1) && (strcmp("-srp", argv[1]) == 0))
    {
        printf("\e[1\"q\e[s\e[0\"q\e[ua");

        // 1. Enable char prot
        // 2. Save attr
        // 3. Disable char prot
        // 4. Restore attr
        return;
    }

    if ((argc > 1) && (strcmp("-bs0", argv[1]) == 0))
    {
        printf("\e[?45h\e[s\e[?45l\e[u\e[2;1H\b\e[6n");
        return;
    }

    if ((argc > 1) && (strcmp("-tstop", argv[1]) == 0))
    {
        printf("\e[1;25H\e[2Z\e[6n");
        printf("\e[1;25H\e[Z\e[6n");
        return;
    }

    if ((argc > 1) && (strcmp("-srcwrap", argv[1]) == 0))
    {
        printf("\e[2J\e[?7h\e[s\e[?7l\e[u\e[18t\e[1;79Habcd\e[6n");
        Stdout_Flush();
        printf("\nCursor position should be: 1;80\n");
        printf("Cursor position is at: %d;%d (Check logs for accuracy)\n", TTY_GetSY_A()+1, TTY_GetSX()+1);
        return;
    }

    

    if ((argc > 1) && (strcmp("-hpa", argv[1]) == 0))
    {
        printf("\e[3;5H\e[G\e[6n");
        printf("\e[3;5H\e[2G\e[6n");
        printf("\e[9999G\e[6n");  // Stupidly large, but tests 8/16 bit param
        
        return;
    }

    if ((argc > 1) && (strcmp("-cpl", argv[1]) == 0))
    {
        printf("\e[4;5r\e[?69h\e[5;10s\e[3;7H\e[18t\e[25F\e[6n");
        
        return;
    }

    if ((argc > 1) && (strcmp("-k12", argv[1]) == 0))
    {
        printf("\033[1;1H");    // move to top-left
        printf("\033[0J");      // clear screen
        printf("\033#8");       // Fill screen with E
        printf("\033[2;40H");   // move
        printf("\033[1K");      // Partial clear

        Stdout_Flush();
        waitMs(2000);

        printf("\033[1;1H");    // move to top-left
        printf("\033[0J");      // clear screen
        printf("\033#8");       // Fill screen with E
        printf("\033[4;40H");   // move
        printf("\033[0K");      // Partial clear

        Stdout_Flush();
        waitMs(2000);
        return;
    }

    if ((argc > 1) && (strcmp("-cup1", argv[1]) == 0))  // CUP V-1: Normal Usage
    {
        printf("\033[1;1H"); // move to top-left
        printf("\033[0J"); // clear screen
        printf("\033[2;3H");
        printf("A");

        /*
        |__________|
        |__Ac______|
        */
       
        Stdout_Flush();
        waitMs(2000);
        return;
    }

    if ((argc > 1) && (strcmp("-cup2", argv[1]) == 0))  // CUP V-2: Off the Screen
    {
        printf("\033[1;1H"); // move to top-left
        printf("\033[0J"); // clear screen
        printf("\033[500;500H");
        printf("A");

        /*
        |__________|
        |__________|
        |_________Ac
        */
       
        Stdout_Flush();
        waitMs(2000);
        return;
    }

    if ((argc > 1) && (strcmp("-cup3", argv[1]) == 0))  // CUP V-3: Relative to Origin
    {
        printf("\033[1;1H"); // move to top-left
        printf("\033[0J"); // clear screen
        printf("\033[2;3r"); // scroll region top/bottom
        printf("\033[?6h"); // origin mode
        printf("\033[1;1H"); // move to top-left
        printf("X");

        /*
        |__________|
        |X_________|
        */
       
        Stdout_Flush();
        waitMs(2000);
        return;
    }

    if ((argc > 1) && (strcmp("-cup4", argv[1]) == 0))  // CUP V-4: Relative to Origin with Left/Right Margins
    {
        printf("\033[1;1H"); // move to top-left
        printf("\033[0J"); // clear screen
        printf("\033[?69h"); // enable left/right margins
        printf("\033[3;5s"); // scroll region left/right
        printf("\033[2;3r"); // scroll region top/bottom
        printf("\033[?6h"); // origin mode
        printf("\033[1;1H"); // move to top-left
        printf("X");
        printf("\033[?69l");

        /*
        |__________|
        |__X_______|
        */
       
        Stdout_Flush();
        waitMs(2000);
        return;
    }

    if ((argc > 1) && (strcmp("-cup5", argv[1]) == 0))  // CUP V-5: Limits with Scroll Region and Origin Mode
    {
        printf("\033[1;1H"); // move to top-left
        printf("\033[0J"); // clear screen
        printf("\033[?69h"); // enable left/right margins
        printf("\033[3;5s"); // scroll region left/right
        printf("\033[2;3r"); // scroll region top/bottom
        printf("\033[?6h"); // origin mode
        printf("\033[500;500H"); // move to top-left
        printf("X");
        printf("\033[?69l");

        /*
        |__________|
        |__________|
        |____X_____|
        */
       
        Stdout_Flush();
        waitMs(2000);
        return;
    }

    if ((argc > 1) && (strcmp("-cup6", argv[1]) == 0))  // CUP V-6: Pending Wrap is Unset
    {
        printf("\033[1;1H"); // move to top-left
        printf("\033[0J"); // clear screen
        printf("\033[%dG", C_XMAX); // move to last column
        printf("A"); // set pending wrap state
        printf("\033[1;1H");
        printf("X");

        /*
        |Xc_______X|
        */

        Stdout_Flush();
        waitMs(2000);
        return;
    }

    if ((argc > 1) && (strcmp("-cuf1", argv[1]) == 0))  // CUF V-1: Pending Wrap is Unset
    {
        printf("\033[1;1H"); // move to top-left
        printf("\033[0J"); // clear screen
        printf("\033[%uG", C_XMAX); // move to last column
        printf("A"); // set pending wrap state
        printf("\033[C"); // move forward one
        printf("XYZ");

        /*
        |_________X|
        |YZ________|
        */

        Stdout_Flush();
        waitMs(2000);
        return;
    }

    if ((argc > 1) && (strcmp("-cuf2", argv[1]) == 0))  // CUF V-2: Rightmost Boundary with Reverse Wrap Disabled
    {
        printf("\033[1;1H"); // move to top-left
        printf("\033[0J"); // clear screen
        printf("A");
        printf("\033[500C"); // forward larger than screen width
        printf("B");

        /*
        |A________Bc
        */

        Stdout_Flush();
        waitMs(2000);
        return;
    }

    if ((argc > 1) && (strcmp("-cuf3", argv[1]) == 0))  // CUF V-3: Left of the Right Margin
    {
        printf("\033[1;1H"); // move to top-left
        printf("\033[0J"); // clear screen
        printf("\033[?69h"); // enable left/right margins
        printf("\033[3;5s"); // scroll region left/right
        printf("\033[1G"); // move to left
        printf("\033[500C"); // forward larger than screen width
        printf("X");
        printf("\033[?69l");

        /*
        |____X_____|
        */

        Stdout_Flush();
        waitMs(2000);
        return;
    }

    if ((argc > 1) && (strcmp("-cuf4", argv[1]) == 0))  // CUF V-4: Right of the Right Margin
    {
        printf("\033[1;1H"); // move to top-left
        printf("\033[0J"); // clear screen
        printf("\033[?69h"); // enable left/right margins
        printf("\033[3;5s"); // scroll region left/right
        printf("\033[6G"); // move to right of margin
        printf("\033[500C"); // forward larger than screen width
        printf("X");
        printf("\033[?69l");

        /*
        |_________X|
        */

        Stdout_Flush();
        waitMs(2000);
        return;
    }

    if ((argc > 1) && (strcmp("-cuf_param", argv[1]) == 0))  // Param test (16 bit)
    {
        printf("\033[255C");
        printf("\033[500C");
        printf("\033[C");
        return;
    }

    if ((argc > 1) && (strcmp("-cuf0", argv[1]) == 0))  // No param actually move 1 forward
    {
        printf("A");
        printf("\033[C");
        printf("X");
        // A_Xc
        return;
    }

    if ((argc > 1) && (strcmp("-mlr1", argv[1]) == 0))  // DECSLRM V-1: Full Screen
    {
        printf("\033[1;1H"); // move to top-left
        printf("\033[0J"); // clear screen
        printf("ABC\n");
        printf("DEF\n");
        printf("GHI\n");
        printf("\033[?69h"); // enable left/right margins
        printf("\033[s"); // scroll region left/right
        printf("\033[X");
        printf("\033[?69l");

        /*
        |cBC_____|
        |DEF_____|
        |GHI_____|
        */

        Stdout_Flush();
        waitMs(2000);
        return;
    }

    if ((argc > 1) && (strcmp("-mlr2", argv[1]) == 0))  // DECSLRM V-2: Left Only
    {
        printf("\033[1;1H"); // move to top-left
        printf("\033[0J"); // clear screen
        printf("ABC\n");
        printf("DEF\n");
        printf("GHI\n");
        printf("\033[?69h"); // enable left/right margins
        printf("\033[2s"); // scroll region left/right
        printf("\033[2G"); // move cursor to column 2
        printf("\033[L");

        /*
        |Ac______|
        |DBC_____|
        |GEF_____|
        | HI_____|
        */

        Stdout_Flush();
        waitMs(2000);
        return;
    }

    if ((argc > 1) && (strcmp("-cub1", argv[1]) == 0))  // CUB V-1: Pending Wrap is Unset
    {
        printf("\033[1;1H"); // move to top-left
        printf("\033[0J"); // clear screen
        
        printf("\033[%uG", C_XMAX); // move to last column
        printf("A"); // set pending wrap state
        printf("\033[D"); // move back one
        printf("XYZ");

        /*
        |________XY|
        |Zc________|
        */
        
        Stdout_Flush();
        waitMs(2000);
        return;
    }
    
    if ((argc > 1) && (strcmp("-cub2", argv[1]) == 0))  // CUB V-2: Leftmost Boundary with Reverse Wrap Disabled
    {
        printf("\033[1;1H"); // move to top-left
        printf("\033[0J"); // clear screen
        
        printf("\033[?45l"); // disable reverse wrap
        printf("A\n");
        printf("\033[10D"); // back
        printf("B");

        /*
        |A_________|
        |Bc________|
        */
        
        Stdout_Flush();
        waitMs(2000);
        return;
    }
    
    if ((argc > 1) && (strcmp("-cub3", argv[1]) == 0))  // CUB V-3: Reverse Wrap
    {
        printf("\033[?7h"); // enable wraparound
        printf("\033[?45h"); // enable reverse wrap
        printf("\033[1;1H"); // move to top-left
        printf("\033[0J"); // clear screen
        printf("\033[%uG", C_XMAX); // move to end of line
        printf("AB"); // write and wrap
        printf("\033[D"); // move back two
        printf("X");

        /*
        |_________Xc
        |B_________|
        */
        
        Stdout_Flush();
        waitMs(2000);
        return;
    }
    
    if ((argc > 1) && (strcmp("-cub4", argv[1]) == 0))  // CUB V-4: Extended Reverse Wrap Single Line
    {
        printf("\033[?7h"); // enable wraparound
        printf("\033[?1045h"); // enable extended reverse wrap
        printf("\033[1;1H"); // move to top-left
        printf("\033[0J"); // clear screen
        printf("A\n");
        printf("B");
        printf("\033[2D"); // move back two
        printf("X");

        /*
        |A________Xc
        |B_________|
        */
        
        Stdout_Flush();
        waitMs(2000);
        return;
    }
    
    if ((argc > 1) && (strcmp("-cub5", argv[1]) == 0))  // CUB V-5: Extended Reverse Wrap Wraps to Bottom
    {
        printf("\033[?7h"); // enable wraparound
        printf("\033[?1045h"); // enable extended reverse wrap
        printf("\033[1;1H"); // move to top-left
        printf("\033[0J"); // clear screen
        printf("\033[1;3r"); // set scrolling region
        printf("A\n");
        printf("B");
        printf("\033[D"); // move back one
        printf("\033[%uD", C_XMAX); // move back entire width
        printf("\033[D"); // move back one
        printf("X");

        /*
        |A_________|
        |B_________|
        |_________Xc
        */
        
        Stdout_Flush();
        waitMs(2000);
        return;
    }
    
    if ((argc > 1) && (strcmp("-cub6", argv[1]) == 0))  // CUB V-6: Reverse Wrap Outside of Margins
    {
        printf("\033[1;1H");
        printf("\033[0J");
        printf("\033[?45h");
        printf("\033[3r");
        printf("\b");
        printf("X");

        /*
        |__________|
        |__________|
        |Xc________|
        */
        
        Stdout_Flush();
        waitMs(2000);
        return;
    }
    
    if ((argc > 1) && (strcmp("-cub7", argv[1]) == 0))  // CUB V-7: Reverse Wrap with Pending Wrap State
    {
        printf("\033[1;1H"); // move to top-left
        printf("\033[0J"); // clear screen
        
        printf("\033[?45h");
        printf("\033[%uG", C_XMAX);
        printf("\033[4D");
        printf("ABCDE");
        printf("\033[D");
        printf("X");

        /*
        |_____ABCDX|
        */
        
        Stdout_Flush();
        waitMs(2000);
        return;
    }
    
    if ((argc > 1) && (strcmp("-tmux", argv[1]) == 0))
    {
        printf("\033ktmux\033\\");
        
        return;
    }
    if ((argc > 1) && (strcmp("-title", argv[1]) == 0))
    {
        printf("\033]2;Title\7");
        
        return;
    }


    if ((argc > 1) && (strcmp("-erase", argv[1]) == 0))
    {    
        if (!sv_Font)
        {
            printf("0123456789ABCDEF0123456789ABCDEF01234567\n");
            Stdout_Flush();
            printf("████████████████████████████████████████\n");
            Stdout_Flush();
            printf("████████████████████████████████████████\n");
            Stdout_Flush();
            printf("████████████████████████████████████████\n");
            Stdout_Flush();
            printf("████████████████████████████████████████\n");
            Stdout_Flush();
        }
        else
        {
            printf("0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF\n");
            Stdout_Flush();
            printf("████████████████████████████████████████████████████████████████████████████████\n");
            Stdout_Flush();
            printf("████████████████████████████████████████████████████████████████████████████████\n");
            Stdout_Flush();
            printf("████████████████████████████████████████████████████████████████████████████████\n");
            Stdout_Flush();
            printf("████████████████████████████████████████████████████████████████████████████████\n");
            Stdout_Flush();
        }

        // Erase from start to cursor
        TTY_MoveCursor(TTY_CURSOR_UP, 4);
        TTY_MoveCursor(TTY_CURSOR_RIGHT, 4);
        printf("\e[1K");
        Stdout_Flush();

        // Erase from cursor to end
        TTY_MoveCursor(TTY_CURSOR_DOWN, 1);
        printf("\e[0K");
        Stdout_Flush();

        // Erase entire line
        TTY_MoveCursor(TTY_CURSOR_DOWN, 1);
        printf("\e[2K");
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
                Stdout_Push("\e[0J");
                Stdout_Flush();
                // Rows 0, 1, 2 (including top column display) should be left on screen
            break;
            case 1:
                // Clear screen from cursor up
                TTY_MoveCursor(TTY_CURSOR_UP, 5);
                Stdout_Push("\e[1J");
                Stdout_Flush();
                // Rows 4, 5, 6, 7 should be left on screen
            break;
            case 2:
                // Clear screen
                Stdout_Push("\e[2J");
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
        printf("\e[97mReinitializing devmgr...\e[0m\n");
        DeviceManager_Init();
        return;
    }

    if ((argc > 1) && (strcmp("-colour", argv[1]) == 0))
    {
    printf("\
\e[30m█\e[90m█\
\e[91m█\e[31m█\
\e[32m█\e[92m█\
\e[93m█\e[33m█\
\e[34m█\e[94m█\
\e[95m█\e[35m█\
\e[36m█\e[96m█\
\e[97m█\e[37m█\
\e[30m\n");

printf("\n\
\e[90m█\e[30m█\
\e[31m█\e[91m█\
\e[92m█\e[32m█\
\e[33m█\e[93m█\
\e[94m█\e[34m█\
\e[35m█\e[95m█\
\e[96m█\e[36m█\
\e[37m█\e[97m█\
\e[30m\n");

printf("\n\
\e[30m █ \e[90m█\
\e[31m █ \e[91m█\
\e[32m █ \e[92m█\
\e[33m █ \e[93m█\
\e[34m █ \e[94m█\
\e[35m █ \e[95m█\
\e[36m █ \e[96m█\
\e[37m █ \e[97m█\
\e[30m\n");

printf(" \
\e[30m █ \e[90m█\
\e[31m █ \e[91m█\
\e[32m █ \e[92m█\
\e[33m █ \e[93m█\
\e[34m █ \e[94m█\
\e[35m █ \e[95m█\
\e[36m █ \e[96m█\
\e[37m █ \e[97m█\
\e[30m\n");

printf("\n\
\e[30m█\e[90m█\
\e[31m█\e[91m█\
\e[32m█\e[92m█\
\e[33m█\e[93m█\
\e[34m█\e[94m█\
\e[35m█\e[95m█\
\e[36m█\e[96m█\
\e[37m█\e[97m█\
\e[30m");

printf("\n\
\e[90m█\e[30m█\
\e[91m█\e[31m█\
\e[92m█\e[32m█\
\e[93m█\e[33m█\
\e[94m█\e[34m█\
\e[95m█\e[35m█\
\e[96m█\e[36m█\
\e[97m█\e[37m█\
\e[0m\n");

/*
printf("\n\
\e[30m█\e[31m█\e[32m█\e[33m█\e[34m█\e[35m█\e[36m█\e[37m█\
\e[90m█\e[91m█\e[92m█\e[93m█\e[94m█\e[95m█\e[96m█\e[97m█\e[0m\n");
*/
//printf("\n\e[30m█\e[31m█\e[32m█\e[33m█\e[34m█\e[35m█\e[36m█\e[37m█\e[0m\n"); // ?????

printf("\n\
\e[30;47mA\
\e[90;107mA\
\e[91;106mA\
\e[31;46mA\
\e[32;45mA\
\e[92;105mA\
\e[93;104mA\
\e[33;44mA\
\e[34;43mA\
\e[94;103mA\
\e[95;102mA\
\e[35;42mA\
\e[36;41mA\
\e[96;101mA\
\e[97;100mA\
\e[37;40mA\
\e[0m\n\n");

        printf(" !\"#$%c&'()*+,-./012345678", '%');
        Stdout_Flush();
        printf("9:;<=>?\n");
        printf("@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\n");
        printf("`abcdefghijklmnopqrstuvwxyz{|}~ \n");
        Stdout_Flush();
        
        for (u8 y = 0; y < 4; y++)
        {
            for (u8 x = 0; x < 32; x++)
            {
                u8 r = 128 + ((y*32)+x);
                TTY_PrintChar(r);
            }
            TELNET_ParseRX('\n');
            Stdout_Flush();
        }

        return;
    }
}

void CMD_FlushBuffer(u8 argc, char *argv[])
{
    if (argc < 2) 
    {
        printf("Flush buffer and set to 0\n\nUsage:\nbflush <buffer>\n\nBuffers available: rx, tx, stdout\n\n");
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
            Buffer_Push(&StdoutBuffer, 0);
        }
        
        Buffer_Flush(&StdoutBuffer);
    }
}

void CMD_PrintBuffer(u8 argc, char *argv[])
{    
    if ((argc == 3) && strcmp(argv[1], "rx") == 0)
    {
        printf("$%X\n\n", RxBuffer.data[atoi16(argv[2]) % BUFFER_LEN]);
    }
    else if ((argc == 3) && strcmp(argv[1], "tx") == 0)
    {
        printf("$%X\n\n", TxBuffer.data[atoi16(argv[2]) % BUFFER_LEN]);
    }
    else if ((argc == 3) && strcmp(argv[1], "stdout") == 0)
    {
        printf("$%X\n\n", StdoutBuffer.data[atoi16(argv[2]) % BUFFER_LEN]);
    }
    else
    {
        //Stdout_Push("Print byte at <position> in <buffer>\n\nUsage:\nbprint <buffer> <position>\n\nBuffers available: rx, tx, stdout\n");
        printf("Print <buffer> to stdout or file\n\n");
        printf("Usage:\nbprint <buffer> <position>\n");
        printf("\nBuffers available: rx, tx, stdout\n\n");
        return;
    }
}

void CMD_Ping(u8 argc, char *argv[])
{
    if (argc < 2) 
    {
        printf("Ping IP address\n\nUsage:\nping <address>\n\n");
        return;
    }

    u8 r = NET_PingIP(argv[1]);

    switch (r)
    {
        case 1:
            printf("Ping response timeout!\n\n");
        break;
    
        default:
        break;
    }
}

void CMD_Run(u8 argc, char *argv[])
{
    if (argc < 2) return;
    char fn_buf[FILE_MAX_FNBUF];

    FS_ResolvePath(argv[1], fn_buf);

    void *proc = ELF_LoadProc(fn_buf);

    Stdout_Flush();

    if (proc == NULL) return;

    //kprintf("Jumping to user code at %p\n", proc);

    __asm__ __volatile__ (
        "movem.l %%d0-%%d7/%%a0-%%a6, -(%%sp)   \n\t"
        "move.l %[pc], -(%%sp)                  \n\t" // push PC
        "move.w #0x0400, -(%%sp)                \n\t" // push SR
        "rte                                    \n\t"
        :
        : [pc] "a"(proc)
        : "%d0", "%a0"
    );

    return;
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
        FS_ListDir(argv[1]);
    }
    else 
    {
        FS_ListDir(FS_GetCWD());
    }
}

void CMD_MoveFile(u8 argc, char *argv[])
{
    if (argc < 3) return;

    CMD_CopyFile(argc, argv);

    char fn_buf1[FILE_MAX_FNBUF];
    char fn_buf2[FILE_MAX_FNBUF];

    FS_ResolvePath(argv[1], fn_buf1);
    FS_ResolvePath(argv[2], fn_buf2);

    SM_File *f = F_Open(fn_buf2, FM_RDONLY);
    if (f == NULL)
    {
        printf("Failed to move file \"%s\" to \"%s\"\n\n", argv[1], argv[2]);
        goto OnExit;
    }

    F_Close(f);
    FS_Remove(fn_buf1);

    OnExit:
    return;
}

void CMD_CopyFile(u8 argc, char *argv[])
{
    if (argc < 3) return;

    SM_File *f1 = NULL;
    SM_File *f2 = NULL;
    char fn_buf1[FILE_MAX_FNBUF];
    char fn_buf2[FILE_MAX_FNBUF];

    FS_ResolvePath(argv[1], fn_buf1);
    FS_ResolvePath(argv[2], fn_buf2);
    
    f1 = F_Open(fn_buf1, FM_RDONLY);

    if (f1 == NULL)
    {
        printf("File \"%s\" does not exist\n\n", argv[1]);
        goto OnExit;
    }

    f2 = F_Open(fn_buf2, FM_CREATE | FM_WRONLY);

    if (f2 == NULL)
    {
        printf("Failed to create file \"%s\"\n\n", argv[2]);
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
    F_Close(f1);
    F_Close(f2);
    free(buf);
    return;
}

void CMD_Concatenate(u8 argc, char *argv[]) 
{
    if (argc < 2)
    {
        return;
    }

    char fn_buf[FILE_MAX_FNBUF];
    FS_ResolvePath(argv[1], fn_buf);

    SM_File *f = F_Open(fn_buf, FM_RDWR);

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

            bAutoFlushStdout = TRUE;
            u16 offset = 0;

            while (offset < size) 
            {
                u16 nsize = (size - offset > BUFFER_LEN) ? BUFFER_LEN : (size - offset);

                F_Write(buf + offset, nsize, 1, stdout);

                offset += nsize;
            }
            bAutoFlushStdout = FALSE;
        }

        free(buf);
        F_Close(f);
    }
}

void CMD_Touch(u8 argc, char *argv[]) 
{
    if (argc < 2)
    {
        printf("Update file or create a new file if it does not exist\nNot enough arguments\n\n");
        return;
    }

    char fn_buf[FILE_MAX_FNBUF];
    FS_ResolvePath(argv[1], fn_buf);

    SM_File *f = F_Open(fn_buf, FM_CREATE);
    F_Close(f);
}

void CMD_MakeDirectory(u8 argc, char *argv[]) 
{
    if (argc < 2)
    {
        printf("Create a directory if it does not already exist\nNot enough arguments\n\n");
        return;
    }

    char fn_buf[FILE_MAX_FNBUF];
    FS_ResolvePath(argv[1], fn_buf);

    FS_MkDir(fn_buf);
}

void CMD_RemoveLink(u8 argc, char *argv[]) 
{
    if (argc < 2)
    {
        printf("Remove a file/directory\nNot enough arguments\n\n");
        return;
    }

    char fn_buf[FILE_MAX_FNBUF];
    FS_ResolvePath(argv[1], fn_buf);

    FS_Remove(fn_buf);
}

void CMD_HexView(u8 argc, char *argv[])
{
    if (argc < 2)
    {
        printf("File HexViewer\n\nUsage: %s <filename>\n\n", argv[0]);
        return;
    }

    char *fn[] = {argv[1]};
    WinMgr_Open(W_HexView, 1, fn);
}

void CMD_Attr(u8 argc, char *argv[])
{
    //if (argc < 4)
    {
        printf("Modifiy file attributes\n\nUsage: %s -s <attribute> <filename>\n       %s -g <filename>\n\nNOT IMPLEMENTED\n\n", argv[0], argv[0]);
        return;
    }
}

void CMD_ListBlock(u8 argc, char *argv[])
{
    FS_PrintBlockDevices();
}

void CMD_Uptime(u8 argc, char *argv[])
{
    SM_Time t;
    
    SecondsToDateTime(&t, SystemUptime);
    SecondsToDateTime(&SystemTime, GetTimeSync());

    printf("%02u:%02u:%02u up %u days, %u:%02u\n\n", SystemTime.hour, SystemTime.minute, SystemTime.second, t.day-1, t.hour, t.minute);
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
                printf("Connection to %s failed\n\n", sync_server);
            break;
            case 2:
                printf("Time was synchronized too recently.\n\n");
            break;
        
            default:
            break;
        }
    }
    else if ((argc > 6) && (strcmp("-set", argv[1]) == 0))  // Set date + time (Format: yyyy mm dd hh mm)
    {
    }
    else if ((argc > 4) && (strcmp("-set", argv[1]) == 0))  // Set date (Format: yyyy mm dd)
    {
    }
    else if ((argc > 3) && (strcmp("-set", argv[1]) == 0))  // Set time (Format: hh mm)
    {
    }
    else if ((argc > 1) && (strcmp("-help", argv[1]) == 0))
    {
        printf("Show/Set date and time\n\nUsage:\n\
date -sync  - Synchronize date & time\n\
date -help  - This screen\n\
date        - Show date and time\n\n");
    }
    else 
    {
        SecondsToDateTime(&SystemTime, GetTimeSync());
        printf("%u-%u-%u %02u:%02u:%02u\n\n", SystemTime.day, SystemTime.month, SystemTime.year, SystemTime.hour, SystemTime.minute, SystemTime.second);
    }
}

void CMD_About(u8 argc, char *argv[])
{
    if (sv_Font)
    {
        printf(" %s - a dumb project created by smds\n", STATUS_TEXT_SHORT);
        printf(" Copyright (c) 2026 smds\n");
        printf(" See %s github for more info:\n", STATUS_TEXT_SHORT);
        printf(" \e[36mgithub.com/SweMonkey/smdt\e[0m\n\n");

        printf(" This project incorporates some code by b1tsh1ft3r\n");
        printf(" Copyright (c) 2023 B1tsh1ft3r\n");
        printf(" See retro.link github for more info:\n");
        printf(" \e[36mgithub.com/b1tsh1ft3r/retro.link\e[0m\n\n");

        printf(" This project makes use of littleFS\n");
        printf(" Copyright (c) 2022, The littlefs authors.\n");
        printf(" Copyright (c) 2017, Arm Limited. All rights reserved.\n");
        printf(" See littleFS github for more info:\n");
        printf(" \e[36mgithub.com/littlefs-project/littlefs\e[0m\n\n");
    }
    else
    {
        printf("%s - a dumb project created by smds\n", STATUS_TEXT_SHORT);
        printf("Copyright (c) 2026 smds\n");
        printf("See %s github for more info:\n", STATUS_TEXT_SHORT);
        printf("\e[36mgithub.com/SweMonkey/smdt\e[0m\n\n");

        printf("This project incorporates some code -\n - by b1tsh1ft3r\n");
        printf("Copyright (c) 2023 B1tsh1ft3r\n");
        printf("See retro.link github for more info:\n");
        printf("\e[36mgithub.com/b1tsh1ft3r/retro.link\e[0m\n\n");

        printf("This project makes use of littleFS\n");
        printf("Copyright (c) 2022, The littlefs authors");
        printf("Copyright (c) 2017, Arm Limited. -\n - All rights reserved\n");
        printf("See littleFS github for more info:\n");
        printf("\e[36mgithub.com/littlefs-project/littlefs\e[0m\n\n");
    }
}

void CMD_PSGBeep(u8 argc, char *argv[])
{    
    if (argc > 3)
    {
        // argv1 = Frequency
        // argv2 = Volume
        // argv3 = Time in ms

        u32 time = atoi32(argv[1]);
            time = time > 60000 ? 60000 : time;
        u8 vol = atoi(argv[2]);
           vol = vol > 0xF ? 0xF : vol;
        u16 freq = atoi16(argv[3]);
            freq = freq > 0xFFF ? 0xFFF : freq;

        PSG_setFrequency(0, freq);
        PSG_setEnvelope(0, vol);
        waitMs(time);
        PSG_setEnvelope(0, PSG_ENVELOPE_MIN);
        waitMs(20);
    }
    else
    {
        printf("PSG Beep\n\nUsage:\n\
psgbeep <Time in ms><Volume><Frequency>\n\n");
    }
}

void CMD_HTTPWeb(u8 argc, char *argv[])
{
    HTTP_Listen();
    return;
}

void CMD_ViewANS(u8 argc, char *argv[])
{
    if (argc < 2) return;

    char fn_buf[FILE_MAX_FNBUF];
    FS_ResolvePath(argv[1], fn_buf);

    SM_File *f = F_Open(fn_buf, FM_RDONLY);

    s32 mem = (s32)MEM_getLargestFreeBlock();

    if (f)
    {
        F_Seek(f, 0, SEEK_END);
        u16 size = F_Tell(f);
        
        //printf("mem: %ld - size: %u\n", mem, size+1);
        mem -= size+1;

        if (mem <= 0)
        {
            printf("\e[91mFailed to open %s - File does not fit in RAM!\e[0m\n", argv[1]);
        }
        else 
        {
            char *buf = (char*)malloc(size+1);

            if (buf)
            {
                memset(buf, 0, size+1);
                F_Seek(f, 0, SEEK_SET);
                F_Read(buf, size, 1, f);

                Stdout_Flush();
                TTY_Init(TF_ClearScreen | TF_ResetVariables);

                bool old_utf8 = bEnableUTF8;
                bEnableUTF8 = FALSE;
                bANSI_SYS_Emulation = TRUE;

                u16 offset = 0;
                while (offset < size) 
                {
                    TELNET_ParseRX(buf[offset]);
                    offset++;

                    if (buf[offset] == 0x1A) break;
                }

                bEnableUTF8 = old_utf8;
                bANSI_SYS_Emulation = FALSE;
            }

            free(buf);
        }

        F_Close(f);
    }
    else printf("\e[91mFailed to open %s!\e[0m\n", argv[1]);
    
    printf("\e[0m");
    Stdout_Flush();
}
