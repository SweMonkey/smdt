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
        printf("%10s %-28s\n", CMDList[i].id, CMDList[i].desc);

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
}

void CMD_UName(u8 argc, char *argv[])
{
    const char *Krnl_Str = "SMDT";
    const char *OS_Str = "SMDT";
    const char *Mach_Str = "m68k";
    //const char *Node_Str = "Local";
    const char *Mach_Name = "None";

    if (argc == 1)
    {
        printf("%s\n", OS_Str);
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
            printf("%s\n", OS_Str);
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

    printf("\n");
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
    TTY_Init(TF_ClearScreen);
}

void CMD_SetVar(u8 argc, char *argv[])
{
    if ((argc < 3) && (strcmp(argv[1], "-list")))
    {
        Stdout_Push("Set variable\n\nUsage:\n\n");
        Stdout_Push("setvar <variable_name> <value>\n");
        Stdout_Push("setvar -list\n");
        Stdout_Push("setvar -list <variable_name>\n");
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

        Stdout_Push("\n[96mNote: Changes to variables may not take effect until you save and reboot.[0m\n");

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
        printf("Please wait...\n");

        u8 r = NET_GetIP(ipstr);

        if (r == 0)
        {
            printf("IP: %s\n", ipstr);
        }
        else if (r == 1)
        {
            printf("Error: Generic error\n");
        }
        else if (r == 2)
        {
            printf("Error: Timed out\n");
        }

        free(ipstr);

        return;
    }

    printf("Error: Out of RAM!\n");
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

    printf("%20s %5u bytes\n", "Free:", MEM_getFree());
    printf("%20s %5u bytes\n", "Largest free block:", MEM_getLargestFreeBlock());
    printf("%20s %5u bytes\n", "Used:", MEM_getAllocated());
    printf("%20s %5u bytes\n", "System reserved:", 65536-(MEM_getFree()+MEM_getAllocated()));

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
        printf("\n─ %s ─────────────────────────────\n", bPALSystem ? "Sega CD" : "Mega CD");

        printf("%20s %6u bytes\n", "Free:", 524288);
        printf("%20s %6u bytes\n", "Largest free block:", 524288);
        printf("%20s %6u bytes\n", "Used:", 0);

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

    printf("Run \"%s -defrag\" for memory defrag.\n", argv[0]);
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

/*asm(".global testhello\ntesthello:\n.incbin \"tmp/smdt_exec/projects/hello/hello.bin\"");
extern const unsigned char testhello[];*/
void CMD_Test(u8 argc, char *argv[])
{
    Stdout_Push("[91mWarning: The test command may cause side effects on SMDT operation.\nIt may outright crash your system depending on the parameters given![0m\n\n");

    /*if ((argc > 1) && (strcmp("-test1arg", argv[1]) == 0))
    {
        return;
    }

    if ((argc > 2) && (strcmp("-test2arg", argv[1]) == 0))
    {
        return;
    }*/

    /*if ((argc > 1) && (strcmp("-wb", argv[1]) == 0))
    {
        SM_File *f = F_Open("/hi.bin", FM_CREATE | FM_RDWR);

        if (f != NULL)
        {
            F_Write(testhello, 7788, 1, f);
            F_Close(f);
        }
        return;
    }*/
    
    /*if ((argc > 2) && (strcmp("-f", argv[1]) == 0))
    {
        char buf[162] = "Hello World!\nThis is a long text file that just keeps dragging on and on and on...\nMaybe it will repeat forever? who knows, it shouldn't repeat... but it might";
        SM_File *f = F_Open(argv[2], FM_WRONLY);

        printf("Writing this:\n\n%s\n\nto file %s\n", buf, argv[2]);
        F_Write(buf, 162, 1, f);

        F_Close(f);
        
        f = F_Open(argv[2], FM_RDONLY);

        memset(buf, 0, 162);
        F_Read(buf, 162, 1, f);

        printf("Reading back file %s:\n\n%s\n", argv[2], buf);

        F_Close(f);

        return;
    }*/

    /*if ((argc > 1) && (strcmp("t", argv[1]) == 0))
    {
        kprintf("--- Pushing t garbage ---");

        Stdout_Push("[65;1\"p");
        Stdout_Push("[!p");
        Stdout_Push("[8;25;80t");
        Stdout_Push("[?1047l");
        Stdout_Push("[?1049l");
        Stdout_Push("[?47l");
        Stdout_Push("[?69l");
        Stdout_Push("[4l");
        Stdout_Push("[20l");
        Stdout_Push("[?7h");
        Stdout_Push("[?41l");
        Stdout_Push("[>0;1T");
        Stdout_Push("[>2;3t");
        Stdout_Push("[2J");
        Stdout_Push("[23;0t");
        Stdout_Push("[23;0t");
        Stdout_Push("[23;0t");
        Stdout_Push("[23;0t");
        Stdout_Push("[23;0t");
        Stdout_Push("[3g");
        Stdout_Push("[18t");
    }

    // DECERATests.test_DECERA_respectsOriginMode
    if ((argc > 1) && (strcmp("t1", argv[1]) == 0))
    {
        Stdout_Push("[65;1\"p");
        Stdout_Push("[!p");
        Stdout_Push("[8;25;80t");
        Stdout_Push("[?1047l");
        Stdout_Push("[?1049l");
        Stdout_Push("[?47l");
        Stdout_Push("[?69l");
        Stdout_Push("[4l");
        Stdout_Push("[20l");
        Stdout_Push("[?7h");
        Stdout_Push("[?41l");
        Stdout_Push("[>0;1T");
        Stdout_Push("[>2;3t");
        Stdout_Push("[2J");
        Stdout_Push("[23;0t");
        Stdout_Push("[23;0t");
        Stdout_Push("[23;0t");
        Stdout_Push("[23;0t");
        Stdout_Push("[23;0t");
        Stdout_Push("[3g");
        Stdout_Push("[18t");
        Stdout_Push("[1;1H");
        Stdout_Push("[1;9H");
        Stdout_Push("[1;17H");
        Stdout_Push("[1;25H");
        Stdout_Push("[1;33H");
        Stdout_Push("[1;41H");
        Stdout_Push("[1;49H");
        Stdout_Push("[1;57H");
        Stdout_Push("[1;65H");
        Stdout_Push("[1;73H");
        Stdout_Push("[1;1H");        

        Stdout_Push("abcdefgh\n\r");
        Stdout_Push("ijklmnop\n\r");
        Stdout_Push("qrstuvwx\n\r");
        Stdout_Push("yz012345\n\r");
        Stdout_Push("ABCDEFGH\n\r");
        Stdout_Push("IJKLMNOP\n\r");
        Stdout_Push("QRSTUVWX\n\r");
        Stdout_Push("YZ6789!@\n\r");

        Stdout_Push("[1t");
        Stdout_Push("]104;\\");
        Stdout_Push("]10;#000\\");
        Stdout_Push("]11;#ffffff\\");
        Stdout_Push("[1;1H");
        Stdout_Push("[?69h");
        Stdout_Push("[2;9s");
        Stdout_Push("[2;9r");
        Stdout_Push("[?6h");
        Stdout_Push("[1;1;3;3$z");
        Stdout_Push("[?69l");
        Stdout_Push("[r");
        Stdout_Push("[?6l");
        Stdout_Push("[212;0;1;1;1;1*y");

        Stdout_Push("[10;1H");
    }

    if ((argc > 1) && (strcmp("fuck", argv[1]) == 0))
    {
        Stdout_Push("[1t");
        Stdout_Push("]104;\\");
        Stdout_Push("]10;#000\\");
        Stdout_Push("]11;#ffffff\\");
        Stdout_Push("[?85n");

        Stdout_Push("[65;1\"p");
        Stdout_Push("[!p");
        Stdout_Push("[8;25;80t");
        Stdout_Push("[?1047l");
        Stdout_Push("[?1049l");
        Stdout_Push("[?47l");
        Stdout_Push("[?69l");
        Stdout_Push("[4l");
        Stdout_Push("[20l");
        Stdout_Push("[?7h");
        Stdout_Push("[?41l");
        Stdout_Push("[>0;1T");
        Stdout_Push("[>2;3t");
        Stdout_Push("[2J");
        Stdout_Push("[23;0t");
        Stdout_Push("[23;0t");
        Stdout_Push("[23;0t");
        Stdout_Push("[23;0t");
        Stdout_Push("[23;0t");
        Stdout_Push("[3g");
        Stdout_Push("[18t");
        Stdout_Push("[65;1\"p");
        Stdout_Push("[!p");
        Stdout_Push("[8;25;80t");
        Stdout_Push("[?1047l");
        Stdout_Push("[?1049l");
        Stdout_Push("[?47l");
        Stdout_Push("[?69l");
        Stdout_Push("[4l");
        Stdout_Push("[20l");
        Stdout_Push("[?7h");
        Stdout_Push("[?41l");
        Stdout_Push("[>0;1T");
        Stdout_Push("[>2;3t");
        Stdout_Push("[2J");
        Stdout_Push("[23;0t");
        Stdout_Push("[23;0t");
        Stdout_Push("[23;0t");
        Stdout_Push("[23;0t");
        Stdout_Push("[23;0t");
        Stdout_Push("[3g");
        Stdout_Push("[18t");
        Stdout_Push("[1;1H");
        Stdout_Push("H");
        Stdout_Push("[1;9H");
        Stdout_Push("H");
        Stdout_Push("[1;17H");
        Stdout_Push("H");
        Stdout_Push("[1;25H");
        Stdout_Push("H");
        Stdout_Push("[1;33H");
        Stdout_Push("H");
        Stdout_Push("[1;41H");
        Stdout_Push("H");
        Stdout_Push("[1;49H");
        Stdout_Push("H");
        Stdout_Push("[1;57H");
        Stdout_Push("H");
        Stdout_Push("[1;65H");
        Stdout_Push("H");
        Stdout_Push("[1;73H");
        Stdout_Push("H");
        Stdout_Push("[1;1H");
    }

    if ((argc > 1) && (strcmp("xz", argv[1]) == 0))
    {
        Stdout_Push("[37;1;1;8;8$x");
        Stdout_Push("[1;1;3;3$z");
    }

    if ((argc > 1) && (strcmp("z", argv[1]) == 0))
    {
        //Stdout_Push("[$z");
        Stdout_Push("[3;1;1t");
    }

    if ((argc > 1) && (strcmp("title", argv[1]) == 0))
    {
        Stdout_Push("]0;smd@m68k:~\7");
    }

    if ((argc > 1) && (strcmp("st", argv[1]) == 0))
    {
        Stdout_Push("]10;#000\\");
        Stdout_Push("]11;#ffffff\\");

        Stdout_Push("]11;#ffffff\\_xyz\\A");
    }

    if ((argc > 1) && (strcmp("y", argv[1]) == 0))
    {
        //Stdout_Push("[2J");

        / *Stdout_Push("[1;1HABC");
        Stdout_Push("[2;1HDEF");
        Stdout_Push("[3;1HGHI");
        Stdout_Push("[0;0;1;1;3;3*y");        
        
        Stdout_Push("[1;10HAAA");
        Stdout_Push("[2;10HAAB");
        Stdout_Push("[3;10HAAC");
        Stdout_Push("[0;0;1;10;3;12*y");

        Stdout_Push("[0;0;10;10;12;12*y");* /

        // ANSIRCTests.test_SaveRestoreCursor_ResetsOriginMode
        Stdout_Push("[65;1\"p");
        Stdout_Push("[!p");
        Stdout_Push("[8;25;80t");
        Stdout_Push("[?1047l");
        Stdout_Push("[?1049l");
        Stdout_Push("[?47l");
        Stdout_Push("[?69l");
        Stdout_Push("[4l");
        Stdout_Push("[20l");
        Stdout_Push("[?7h");
        Stdout_Push("[?41l");
        Stdout_Push("[>0;1T");
        Stdout_Push("[>2;3t");
        Stdout_Push("[2J");
        Stdout_Push("[23;0t");
        Stdout_Push("[23;0t");
        Stdout_Push("[23;0t");
        Stdout_Push("[23;0t");
        Stdout_Push("[23;0t");
        Stdout_Push("[3g");
        Stdout_Push("[18t");
        Stdout_Push("[1;1H");
        Stdout_Push("H");
        Stdout_Push("[1;9H");
        Stdout_Push("H");
        Stdout_Push("[1;17H");
        Stdout_Push("H");
        Stdout_Push("[1;25H");
        Stdout_Push("H");
        Stdout_Push("[1;33H");
        Stdout_Push("H");
        Stdout_Push("[1;41H");
        Stdout_Push("H");
        Stdout_Push("[1;49H");
        Stdout_Push("H");
        Stdout_Push("[1;57H");
        Stdout_Push("H");
        Stdout_Push("[1;65H");
        Stdout_Push("H");
        Stdout_Push("[1;73H");
        Stdout_Push("H");
        Stdout_Push("[1;1H");
        Stdout_Push("[1t");
        Stdout_Push("]104;.\\");
        Stdout_Push("]10;#000\\");
        Stdout_Push("]11;#ffffff\\");
        Stdout_Push("[6;5H");
        Stdout_Push("[s");
        Stdout_Push("[5;7r");
        Stdout_Push("[?69h");
        Stdout_Push("[5;7s");
        Stdout_Push("[?6h");
        Stdout_Push("[u");
        Stdout_Push("[1;1HX");
        Stdout_Push("[?69l");
        Stdout_Push("[r");
        Stdout_Push("[?6l");
        Stdout_Push("[4;0;1;1;1;1*y");
    }

    if ((argc > 1) && (strcmp("uni", argv[1]) == 0))
    {
        kprintf("--- Pushing UTF-8 ---");

        Stdout_PushByte(0x1B);
        Stdout_PushByte(0x5B);
        Stdout_PushByte(0x33);
        Stdout_PushByte(0x34);
        Stdout_PushByte(0x6D);
        Stdout_PushByte(0xF4);
        Stdout_PushByte(0x1B);
        Stdout_PushByte(0x5B);
        Stdout_PushByte(0x30);
        Stdout_PushByte(0x6D);
        Stdout_PushByte(0xFA);
        Stdout_PushByte(0xFA);
        Stdout_PushByte(0xFA);
        Stdout_PushByte(0xB3);
        Stdout_PushByte(0x1B);
        Stdout_PushByte(0x5B);
        Stdout_PushByte(0x30);
        Stdout_PushByte(0x6D);

        return;
    }

    if ((argc > 1) && (strcmp("attr_rgb", argv[1]) == 0))
    {
        printf("Attempt at fake RGB24...\n");
        printf("[38;2;255;0;0mR [38;2;0;255;0mG [38;2;0;0;255mB  [38;2;255;255;0mRG [38;2;255;0;255mRB [38;2;0;255;255mGB [38;2;255;255;255mRGB\n[0m");
        printf("[38;2;125;0;0mR [38;2;0;125;0mG [38;2;0;0;125mB  [38;2;125;125;0mRG [38;2;125;0;125mRB [38;2;0;125;125mGB [38;2;125;125;125mRGB\n[0m");
        return;
    }

    if ((argc > 1) && (strcmp("p", argv[1]) == 0))
    {
        printf("DECSCL; [65;1\"p \n");
        printf("DECSTR; [!p \n");
        return;
    }*/

    if ((argc > 1) && (strcmp("force_xport", argv[1]) == 0))
    {
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
    
    if ((argc > 1) && (strcmp("illegal", argv[1]) == 0))
    {
        asm("illegal");
        return;
    }
   
    if ((argc > 1) && (strcmp("lbrk", argv[1]) == 0))
    {
        printf("Testing linebreak/wraparound...\n");

        printf("This is a long string of text that should wrap around exactly at column 80, somewhere around here... and no letters should be missing in the word \"somewhere\"\n\n");

        printf("This is another long string that should have another line of text under it witho\nut any space inbetween them\n");

        return;
    }

    if ((argc > 1) && (strcmp("fprintf", argv[1]) == 0))
    {
        F_Printf(stdout, "Hello, is this thing on? %s\n", "maybe...");
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
        Stdout_Push("[97mReinitializing devmgr...[0m\n");
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
            Buffer_Push(&StdoutBuffer, 0);
        }
        
        Buffer_Flush(&StdoutBuffer);
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
        sprintf(buf, "$%X\n", StdoutBuffer.data[atoi16(argv[2]) % BUFFER_LEN]);
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

    /*asm("movem.l %d0-%d7/%a0-%a6, -(%sp)");
    asm("move.w #0x0400, %sr");
    asm ("move.l %0, %/a5" :: "r" (proc) : "a5");
    asm("jsr (%a5)");
    asm("movem.l (%sp)+,%d0-%d7/%a0-%a6");*/

    __asm__ __volatile__
    (
        "movem.l %%d0-%%d7/%%a0-%%a6, -(%%sp)   \n\t"
        "move.w #0x0400, %%sr                   \n\t"
        "move.l %[proc], %%a5                   \n\t"
        "jsr (%%a5)                             \n\t"
        /*"move #666, %%d0                        \n\t"*/
        /*"trap #0                                \n\t"*/
        "movem.l (%%sp)+,%%d0-%%d7/%%a0-%%a6    \n\t"
    : /* outputs */
    : /* inputs */
        [proc] "a"(proc)
    : /* clobbered regs */
        "%d0", "%a5"
    );

    kprintf("Returned from oblivion...");
    Stdout_Push("Returned from oblivion...\n");
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
        printf("Failed to move file \"%s\" to \"%s\"\n", argv[1], argv[2]);
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
        printf("File \"%s\" does not exist\n", fn_buf1);
        goto OnExit;
    }

    SM_File *f2 = F_Open(fn_buf2, FM_CREATE | FM_WRONLY);

    if (f2 == NULL)
    {
        printf("Failed to create file \"%s\"\n", fn_buf2);
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
        printf("File HexViewer\n\nUsage: %s <filename>\n", argv[0]);
        return;
    }

    HexView_Open(argv[1]);
}

void CMD_Attr(u8 argc, char *argv[])
{
    //if (argc < 4)
    {
        printf("Modifiy file attributes\n\nUsage: %s -s <attribute> <filename>\n       %s -g <filename>\n\nNOT IMPLEMENTED\n", argv[0], argv[0]);
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

    printf("%02lu:%02u:%02u up %lu days, %lu:%02u\n", SystemTime.hour, SystemTime.minute, SystemTime.second, t.day-1, t.hour, t.minute);
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
                printf("Connection to %s failed\n", sync_server);
            break;
            case 2:
                printf("Time was synchronized too recently.\n");
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
    else printf("%lu-%lu-%lu %02lu:%02u:%02u\n", SystemTime.day, SystemTime.month, SystemTime.year, SystemTime.hour, SystemTime.minute, SystemTime.second);
}

void CMD_About(u8 argc, char *argv[])
{
    if (sv_Font)
    {
        Stdout_Push("\n SMDT - a dumb project created by smds\n");
        Stdout_Push(" Copyright (c) 2025 smds\n");
        Stdout_Push(" See SMDT github for more info:\n");
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
        Stdout_Push("SMDT - a dumb project created by smds\n");
        Stdout_Push("Copyright (c) 2025 smds\n");
        Stdout_Push("See SMDT github for more info:\n");
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
