#include "CMDFunc.h"
#include "StateCtrl.h"
#include "Utils.h"
#include "Network.h"
#include "IRC.h"
#include "Telnet.h"
#include "Terminal.h"
#include "SRAM.h"

#include "devices/XP_Network.h"
#include "devices/Keyboard_PS2.h"

#include "misc/VarList.h"

#include "system/Stdout.h"
#include "system/Time.h"

#ifdef KERNEL_BUILD
#include "system/File.h"
#include "system/Filesystem.h"
#include "system/ELF_ldr.h"
#endif


SM_CMDList CMDList[] =
{
    {"telnet",  CMD_LaunchTelnet,   "<address:port>"},
    {"irc",     CMD_LaunchIRC,      "<address:port>"},
    {"tty",     CMD_LaunchLinuxTTY, "- Init sane linux TTY"},
    {"echo",    CMD_Echo,           "- Echo string to screen"},
    {"kbc",     CMD_KeyboardSend,   "- Send command to keyboard"},
    {"menu",    CMD_LaunchMenu,     "- Run graphical start menu"},
    {"setattr", CMD_SetAttr,        "- Set terminal attributes"},
    {"xpico",   CMD_xpico,          "- Send command to xpico"},
    {"uname",   CMD_UName,          "- Print system information"},
    {"setcon",  CMD_SetConn,        "- Set connection timeout"},
    {"clear",   CMD_ClearScreen,    "- Clear screen"},
    {"setvar",  CMD_SetVar,         "- Set variable"},
    {"getip",   CMD_GetIP,          "- Get network IP"},
    {"free",    CMD_Free,           "- List free memory"},
    {"reboot",  CMD_Reboot,         "- Reboot system"},
    {"savecfg", CMD_SaveCFG,        "- Save confg to SRAM"},
    {"test",    CMD_Test,           "- Test"},
    {"bflush",  CMD_FlushBuffer,    "- Flush specified buffer"},
    {"bprint",  CMD_PrintBuffer,    "- Print byte from buffer"},
    {"ping",    CMD_Ping,           "- Ping IP address"},
    #ifdef KERNEL_BUILD
    {"run",     CMD_Run,            "- Run binary file"},
    {"ls",      CMD_ListDir,        "- List directory"},
    {"cat",     CMD_Concatenate,    "- Concatenate file"},
    {"touch",   CMD_Touch,          "- Update or create file"},
    {"mkdir",   CMD_MakeDirectory,  "- Create directory"},
    {"rm",      CMD_RemoveLink,     "- Remove directory/file"},
    #else
    {"sram",    CMD_TestSRAM,       "- Test SRAM"},
    #endif
    {"uptime",  CMD_Uptime,         "- Show system uptime"},
    {"date",    CMD_Date,           "- Show/Set date and time"},
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

void CMD_LaunchMenu(u8 argc, char *argv[])
{
    ChangeState(PS_Entry, argc, argv);
}

void CMD_LaunchLinuxTTY(u8 argc, char *argv[]) 
{
    char *arg[] = {argv[0], "-tty"};
    ChangeState(PS_Telnet, 2, arg);
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
    char tmp[64];
    
    if ((strcmp("-help", argv[1]) == 0) || (argc < 2))
    {
        Stdout_Push("Echo string to screen\n\nUsage:\n");
        Stdout_Push(argv[0]);
        Stdout_Push(" <string>\n");
        Stdout_Push(argv[0]);
        Stdout_Push(" $variable_name\n");
        
        return;
    }

    if (argv[1][0] == '$')
    {
        u16 i = 0;
        char varname[16] = {'\0'};
        char tmp[64] = {'\0'};

        memcpy(varname, argv[1]+1, strlen(argv[1]-1));
        
        while (VarList[i].size)
        {
            if (strcmp(VarList[i].name, varname) == 0)
            {
                switch (VarList[i].size)
                {
                    case ST_BYTE:
                        sprintf(tmp, "%u\n", *((u8*)VarList[i].ptr));
                    break;
                    case ST_WORD:
                        sprintf(tmp, "%u\n", *((u16*)VarList[i].ptr));
                    break;
                    case ST_LONG:
                        sprintf(tmp, "%lu\n", *((u32*)VarList[i].ptr));
                    break;        
                    case ST_SPTR:
                        sprintf(tmp, "\"%s\"\n", (char*)VarList[i].ptr);
                    break;
                    case ST_SARR:
                        sprintf(tmp, "\"%s\"\n", (char*)VarList[i].ptr);
                    break;
                    default:
                    break;
                }

                Stdout_Push(tmp);
                break;
            }

            i++;
        }
    }
    else
    {
        sprintf(tmp, "%s\n", argv[1]);
        Stdout_Push(tmp);
    }
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
    char tmp[256];
    u16 i = 0;
    
    Stdout_Push("Commands available:\n\n");

    while (CMDList[i].id != 0)
    {
        strclr(tmp);
        sprintf(tmp, "%10s %-28s\n", CMDList[i].id, CMDList[i].desc);
        Stdout_Push(tmp);

        i++;
    }
}

void CMD_xpico(u8 argc, char *argv[])
{
    if (argc == 1)
    {
        Stdout_Push("xPico debug\n\nUsage:\n\
xpico enter       - Enter monitor mode\n\
xpico exit        - Exit monitor mode\n\
xpico <string>    - Send string to xPico\n\
xpico connect <address>\n\
xpico disconnect  - Close connection\n");
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
        char tmp[64];

        sprintf(tmp, "%-12s %s   %s\n\n", "Name", "Type", "Value");
        Stdout_Push(tmp);

        while (VarList[i].size)
        {
            switch (VarList[i].size)
            {
                case ST_BYTE:
                    sprintf(tmp, "%-12s u8     %u\n", VarList[i].name, *((u8*)VarList[i].ptr));
                break;
                case ST_WORD:
                    sprintf(tmp, "%-12s u16    %u\n", VarList[i].name, *((u16*)VarList[i].ptr));
                break;
                case ST_LONG:
                    sprintf(tmp, "%-12s u32    %lu\n", VarList[i].name, *((u32*)VarList[i].ptr));
                break;        
                case ST_SPTR:
                    sprintf(tmp, "%-12s StrPtr \"%s\"\n", VarList[i].name, (char*)VarList[i].ptr);
                break;
                case ST_SARR:
                    sprintf(tmp, "%-12s StrArr \"%s\"\n", VarList[i].name, (char*)VarList[i].ptr);
                break;
                default:
                    memset(tmp, 0, 64);
                break;
            }

            Stdout_Push(tmp);
            i++;
        }

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
    char tmp[64];
    sprintf(tmp, "%20s %5u bytes\n", "Free:", MEM_getFree());
    Stdout_Push(tmp);
    sprintf(tmp, "%20s %5u bytes\n", "Largest free block:", MEM_getLargestFreeBlock());
    Stdout_Push(tmp);
    sprintf(tmp, "%20s %5u bytes\n", "Used:", MEM_getAllocated());
    Stdout_Push(tmp);
}

void CMD_Reboot(u8 argc, char *argv[])
{
    if ((argc > 1) && (strcmp(argv[1], "-soft") == 0)) SYS_reset();

    SYS_hardReset();
}

void CMD_SaveCFG(u8 argc, char *argv[])
{
    SRAM_SaveData();
}

void CMD_Test(u8 argc, char *argv[])
{
    /*if (argc > 1)
    {
        for (u8 i = 0; i < argc; i++) 
        {
            Stdout_Push(argv[i]);
            Stdout_Push("\n");
        }
        return;
    }*/

    #ifdef KERNEL_BUILD
    if ((argc > 2) && (strcmp("-f", argv[1]) == 0))
    {
        char buf[162] = "Hello World!\nThis is a long text file that just keeps dragging on and on and on...\nMaybe it will repeat forever? who knows, it shouldn't repeat... but it might";
        SM_File *f = F_Open(argv[2], FM_WRITE);

        stdout_printf("Writing this:\n\n%s\n\nto file %s\n", buf, argv[2]);
        F_Write(buf, 162, 1, f);

        F_Close(f);
        
        f = F_Open(argv[2], FM_READ);

        memset(buf, 0, 162);
        F_Read(buf, 162, 1, f);

        stdout_printf("Reading back file %s:\n\n%s\n", argv[2], buf);

        F_Close(f);

        return;
    }
    #endif  // KERNEL_BUILD

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


#ifdef KERNEL_BUILD
void CMD_Run(u8 argc, char *argv[])
{
    if (argc < 2) return;

    void *proc = ELF_LoadProc(argv[1]);

    Stdout_Flush();

    if (proc == NULL) return;

    //SRAM_enableRO();
    kprintf("cmd proc: $%X", proc);

    asm("move.w #0x000, %sr");
    VAR2REG_L(proc, "a5");
    asm("jsr (%a5)");

    Stdout_Push("Returned from oblivion...\n");
    //SRAM_disable();
}

void CMD_ListDir(u8 argc, char *argv[]) 
{
    if (argc > 1)
    {
        char tmp[64];
        sprintf(tmp, "/%s", argv[1]);
        FS_ListDir(tmp);
    }
    else FS_ListDir("/");
}

void CMD_Concatenate(u8 argc, char *argv[]) 
{
    if (argc < 2)
    {
        return;
    }

    SM_File *f = F_Open(argv[1], FM_READ);

    if (f)
    {
        F_Seek(f, 0, SEEK_END);
        u16 size = F_Tell(f);

        char *buf = (char*)malloc(size);
        memset(buf, 0, size);

        if (buf)
        {
            F_Seek(f, 0, SEEK_SET);
            F_Read(buf, size, 1, f);

            Stdout_Push(buf);
            Stdout_Flush();
        }

        F_Close(f);
        free(buf);
    }
}

void CMD_Touch(u8 argc, char *argv[]) 
{
    if (argc < 2)
    {
        Stdout_Push("Update file or create a new file if it does not exist\nNot enough arguments\n");
        return;
    }

    SM_File *f = F_Open(argv[1], FM_WRITE);
    F_Close(f);
}

void CMD_MakeDirectory(u8 argc, char *argv[]) 
{
    if (argc < 2)
    {
        Stdout_Push("Create a directory if it does not already exist\nNot enough arguments\n");
        return;
    }

    //SM_File *f = F_Open(argv[1], FM_WRITE);
    //F_Close(f);
}

void CMD_RemoveLink(u8 argc, char *argv[]) 
{
    if (argc < 2)
    {
        Stdout_Push("Remove a file/directory\nNot enough arguments\n");
        return;
    }

    FS_Unlink(argv[1]);
}
#else   // Non Kernel build SRAM function
void CMD_TestSRAM(u8 argc, char *argv[])
{
    if (argc < 2)
    {
        Stdout_Push("Test SRAM\n\nUsage:\nsram <address> - Write/Readback test\nsram -count    - Check installed RAM\n");
        return;
    }

    char tmp[64];
    u32 addr = 0;

    if (strcmp(argv[1], "-count") == 0)
    {
        u32 c = 0;
        addr = 0x1000;

        SRAM_enable();

        while (addr < 0x1FFFF)
        {
            SRAM_writeByte(addr, 0xAC);

            if (SRAM_readByte(addr) == 0xAC)
            {
                addr++;
                c++;
            }
            else break;
        }

        SRAM_disable();

        sprintf(tmp, "Total SRAM: %c$%lX bytes\n", ((c+0x1000) >= 0x1FFFF ? '>' : ' '), c + 0x1000);
        Stdout_Push(tmp);


        return;
    }

    addr = atoi32(argv[1]);

    SRAM_enable();

    // Byte read/write
    sprintf(tmp, "Writing byte $%X to $%lX\n", 0xFF, addr);
    Stdout_Push(tmp);

    SRAM_writeByte(addr, 0xFF);

    if (SRAM_readByte(addr) == 0xFF) Stdout_Push("Byte readback OK\n");
    else Stdout_Push("Byte readback FAIL\n");

    // Word read/write
    sprintf(tmp, "Writing word $%X to $%lX\n", 0xBEEF, addr);
    Stdout_Push(tmp);

    SRAM_writeWord(addr, 0xBEEF);

    if (SRAM_readWord(addr) == 0xBEEF) Stdout_Push("Word readback OK\n");
    else Stdout_Push("Word readback FAIL\n");

    // Long read/write
    sprintf(tmp, "Writing long $%X to $%lX\n", 0xDEADBEEF, addr);
    Stdout_Push(tmp);

    SRAM_writeLong(addr, 0xDEADBEEF);

    if (SRAM_readLong(addr) == 0xDEADBEEF) Stdout_Push("Long readback OK\n");
    else Stdout_Push("Long readback FAIL\n");

    SRAM_disable();
}
#endif  // KERNEL_BUILD

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
