#include "CMDFunc.h"
#include "StateCtrl.h"
#include "Utils.h"
#include "Network.h"
#include "IRC.h"
#include "Telnet.h"
#include "Terminal.h"
#include "devices/XP_Network.h"
#include "devices/Keyboard_PS2.h"
#include "misc/VarList.h"
#include "SRAM.h"
#include "misc/Stdout.h"

//void PrintOutput(const char *str);

SM_CMDList CMDList[] =
{
    {"telnet",  CMD_LaunchTelnet,   "<address:port>"},
    {"irc",     CMD_LaunchIRC,      "<address:port>"},
    {"echo",    CMD_Echo,           "- Echo string to screen"},
    {"kbc",     CMD_KeyboardSend,   "- Send command to keyboard"},
    {"menu",    CMD_LaunchMenu,     "- Run graphical start menu"},
    {"setattr", CMD_SetAttr,        "- Set terminal attributes"},
    {"xpico",   CMD_xpico,          "- Send command to xpico"},
    {"uname",   CMD_UName,          "- Print system information"},
    {"setcon",  CMD_SetConn,        "- Set connection timeout"},
    {"clear",   CMD_ClearScreen,    "- Clear screen"},
    {"sram",    CMD_TestSRAM,       "- Test SRAM"},
    {"setvar",  CMD_SetVar,         "- Set variable"},
    {"getip",   CMD_GetIP,          "- Get network IP"},
    {"run",     CMD_Run,            "- Run binary file"},
    {"free",    CMD_Free,           "- List free memory"},
    {"reboot",  CMD_Reboot,         "- Reboot system"},
    {"savecfg", CMD_SaveCFG,        "- Save confg to SRAM"},
    {"test",    CMD_Test,           "- Test"},
    {"bflush",  CMD_FlushBuffer,    "- Flush specified buffer"},
    {"bprint",  CMD_PrintBuffer,    "- Print byte from buffer"},
    {"ping",    CMD_Ping,           "- Print IP address"},
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
xpico connect <address>\n");
        return;
    }

    if ((argc > 1) && (strcmp(argv[1], "enter") == 0))
    {
        char tmp[20];
        u8 response_code = 4;
        u8 end_char = 0;
        u32 timeout = 0;

        Stdout_Push("Entering monitor mode...\n");
        XPN_FlushBuffers();    
        XPN_SendMessage("C0.0.0.0/0\n");

        waitMs(sv_DelayTime);

        while (timeout++ < sv_ReadTimeout)
        {
            response_code = end_char;
            Buffer_Pop(&RxBuffer, &end_char);

            if (end_char == '>') 
            {
                sprintf(tmp, "Response: %c%c\n", (char)response_code, (char)end_char);
                Stdout_Push(tmp);
                break;
            }
        }

        switch (response_code)
        {
            case '0':
                Stdout_Push("OK; no error\n");
            break;
            case '1':
                Stdout_Push("No answer from remote device\n");
            break;
            case '2':
                Stdout_Push("Cannot reach remote device or no answer\n");
            break;
            case '8':
                Stdout_Push("Wrong parameter(s)\n");
            break;
            case '9':
                Stdout_Push("Invalid command\n");
            break;
        
            default:
            break;
        }

        if (timeout >= sv_ReadTimeout) Stdout_Push("<EnterMonitor timed out>\n");
    }
    else if ((argc > 1) && (strcmp(argv[1], "exit") == 0))
    {
        char tmp[20];
        u8 response_code = 0x4;
        u8 end_char = 0;
        u32 timeout = 0;

        Stdout_Push("Exiting monitor mode...\n");
        XPN_FlushBuffers();
        XPN_SendMessage("QU\n");

        //XPN_SendMessage("QU");
        //XPN_SendByte(0x0A);

        waitMs(sv_DelayTime);

        while (timeout++ < sv_ReadTimeout)
        {
            response_code = end_char;
            Buffer_Pop(&RxBuffer, &end_char);

            if (end_char == '>') 
            {
                sprintf(tmp, "Response: %c%c\n", (char)response_code, (char)end_char);
                Stdout_Push(tmp);
                break;
            }
        }

        switch (response_code)
        {
            case '0':
                Stdout_Push("OK; no error\n");
            break;
            case '1':
                Stdout_Push("No answer from remote device\n");
            break;
            case '2':
                Stdout_Push("Cannot reach remote device or no answer\n");
            break;
            case '8':
                Stdout_Push("Wrong parameter(s)\n");
            break;
            case '9':
                Stdout_Push("Invalid command\n");
            break;
        
            default:
            break;
        }

        if (timeout >= sv_ReadTimeout) 
        {
            Stdout_Push("<ExitMonitor timed out>\n");
            sprintf(tmp, "Debug response: r = $%X - e = $%X\n", response_code, end_char);
            Stdout_Push(tmp);
        }
    }
    else if ((argc > 2) && (strcmp(argv[1], "connect") == 0))
    {
        char tmp[64];
        u8 rxdata = 0;
        u32 timeout = 0;

        sprintf(tmp, "Connecting to %s ...\n", argv[2]);
        Stdout_Push(tmp);
        snprintf(tmp, 36, "Connecting to %s", argv[2]);
        TRM_SetStatusText(tmp);

        XPN_FlushBuffers();  

        XPN_SendByte('C');
        XPN_SendMessage(argv[2]);
        XPN_SendByte(0x0A);

        waitMs(sv_DelayTime);

        Stdout_Push("Received:\n");
        while ((timeout++ < sv_ConnTimeout) && ((rxdata != 'C') || (rxdata != 'N')))
        {
            if (Buffer_Pop(&RxBuffer, &rxdata) == 0) TELNET_ParseRX(rxdata);
        }

        if (timeout >= sv_ConnTimeout) Stdout_Push("<Connection timed out>");

        Stdout_Push("\n");

        TRM_SetStatusText(STATUS_TEXT);
    }
    else if (argc > 1)
    {
        char tmp[64];
        u8 rxdata = 0;
        u32 timeout = 0;

        sprintf(tmp, "Sending \"%s\"\n", argv[1]);
        Stdout_Push(tmp);

        XPN_FlushBuffers();
        XPN_SendMessage(argv[1]);
        
        waitMs(sv_DelayTime);

        Stdout_Push("Received:\n");
        while (timeout++ < sv_ConnTimeout)
        {
            if (Buffer_Pop(&RxBuffer, &rxdata) == 0) TELNET_ParseRX(rxdata);
        }
        Stdout_Push("\n");
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

void CMD_Run(u8 argc, char *argv[])
{
    if (argc < 2) return;

    // TTY_Reset(TRUE);
    /*
    char tmp[64];
    sprintf(tmp, "Running %s...\n", argv[1]);
    Stdout_Push(tmp);

    SRAM_enableRO();
    
    asm("move.w #0x000, %sr");
    VAR2REG_L(0x201000, "a5");
    asm("jsr (%a5)");

    Stdout_Push("Ok ....\n");

    SRAM_disable();*/
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
    if (argc > 1)
    {
        for (u8 i = 0; i < argc; i++) 
        {
            Stdout_Push(argv[i]);
            Stdout_Push("\n");
        }
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

    Stdout_Push("[97mBABABABABABABABA - Plane\n");
    Stdout_Push("[97mLHHLLHHLLHHLLHHL - Priority\n");

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

    Stdout_Push("[97mBABABABABABABABA - Plane\n");
    Stdout_Push("[97mHLLHHLLHHLLHHLLH - Priority\n");

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
