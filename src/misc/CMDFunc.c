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

void PrintOutput(const char *str);

SM_CMDList CMDList[] =
{
    {"telnet",  CMD_LaunchTelnet,   "<address:port>"},
    {"irc",     CMD_LaunchIRC,      "<address:port>"},
    {"echo",    CMD_Echo,           "- Echo string to screen"},
    {"kbc",     CMD_KeyboardSend,   "- Send command to keyboard"},
    {"menu",    CMD_LaunchMenu,     "- Run graphical start menu"},
    {"setattr", CMD_Test,           "- Set terminal attributes"},
    {"xpico",   CMD_xpico,          "- Send command to xpico"},
    {"uname",   CMD_UName,          "- Print system information"},
    {"setcon",  CMD_SetConn,        "- Set connection timeout"},
    {"clear",   CMD_ClearScreen,    "- Clear screen"},
    {"sram",    CMD_TestSRAM,       "- Test SRAM"},
    {"setvar",  CMD_SetVar,         "- Set variable"},
    {"getip",   CMD_GetIP,          "- Get network IP"},
    {"help",    CMD_Help,           "- This command"},
    {0, 0, 0}  // Terminator
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

void CMD_Test(u8 argc, char *argv[])
{
    char tmp[32];

    switch (argc)
    {
        case 2:
            sprintf(tmp, "[%um\n", atoi(argv[1]));
            PrintOutput(tmp);
        break;
        case 3:
            sprintf(tmp, "[%u;%um\n", atoi(argv[1]), atoi(argv[2]));
            PrintOutput(tmp);
        break;
    
        default:
            PrintOutput("Set terminal attribute\n\nUsage:\n");
            PrintOutput(argv[0]);
            PrintOutput(" <number> <number>\n");
            PrintOutput(argv[0]);
            PrintOutput(" <number>\n");
        return;
    }

    PrintOutput("Attributes set.\n");
}

void CMD_Echo(u8 argc, char *argv[])
{
    char tmp[64];

    if (argc < 2) return;

    sprintf(tmp, "%s\n", argv[1]);
    PrintOutput(tmp);
}

void CMD_KeyboardSend(u8 argc, char *argv[])
{
    char tmp[64];

    if (argc < 2) 
    {
        PrintOutput("Send command to keyboard\n\nUsage:\n");
        PrintOutput(argv[0]);
        PrintOutput(" <decimal number between 0 and 255>\n");
        return;
    }

    u8 kbcmd = atoi(argv[1]);
    u8 ret = 0;

    sprintf(tmp, "Sending command $%X to keyboard...\n", kbcmd);
    PrintOutput(tmp);
    
    ret = KB_PS2_SendCommand(kbcmd);
    
    sprintf(tmp, "Recieved byte $%X from keyboard   \n", ret);
    PrintOutput(tmp);
}

void CMD_Help(u8 argc, char *argv[])
{
    char tmp[256];
    u16 i = 0;
    
    PrintOutput("Commands available:\n\n");

    while (CMDList[i].id != 0)
    {
        strclr(tmp);
        sprintf(tmp, "%10s %-28s\n", CMDList[i].id, CMDList[i].desc);
        PrintOutput(tmp);

        i++;
    }
}

void CMD_xpico(u8 argc, char *argv[])
{
    if (argc == 1)
    {
        PrintOutput("xPico debug\n\nUsage:\n\
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

        PrintOutput("Entering monitor mode...\n");
        XPN_FlushBuffers();    
        XPN_SendMessage("C0.0.0.0/0\n");

        waitMs(200);

        while (timeout++ < 50000)
        {
            response_code = end_char;
            Buffer_Pop(&RxBuffer, &end_char);

            if (end_char == '>') 
            {
                sprintf(tmp, "Response: %c%c\n", (char)response_code, (char)end_char);
                PrintOutput(tmp);
                break;
            }
        }

        switch (response_code)
        {
            case '0':
                PrintOutput("OK; no error\n");
            break;
            case '1':
                PrintOutput("No answer from remote device\n");
            break;
            case '2':
                PrintOutput("Cannot reach remote device or no answer\n");
            break;
            case '8':
                PrintOutput("Wrong parameter(s)\n");
            break;
            case '9':
                PrintOutput("Invalid command\n");
            break;
        
            default:
            break;
        }

        if (timeout >= 50000) PrintOutput("<EnterMonitor timed out>\n");
    }
    else if ((argc > 1) && (strcmp(argv[1], "exit") == 0))
    {
        char tmp[20];
        u8 response_code = 0x4;
        u8 end_char = 0;
        u32 timeout = 0;

        PrintOutput("Exiting monitor mode...\n");
        XPN_FlushBuffers();
        XPN_SendMessage("QU\n");

        waitMs(200);

        while (timeout++ < 50000)
        {
            response_code = end_char;
            Buffer_Pop(&RxBuffer, &end_char);

            if (end_char == '>') 
            {
                sprintf(tmp, "Response: %c%c\n", (char)response_code, (char)end_char);
                PrintOutput(tmp);
                break;
            }
        }

        switch (response_code)
        {
            case '0':
                PrintOutput("OK; no error\n");
            break;
            case '1':
                PrintOutput("No answer from remote device\n");
            break;
            case '2':
                PrintOutput("Cannot reach remote device or no answer\n");
            break;
            case '8':
                PrintOutput("Wrong parameter(s)\n");
            break;
            case '9':
                PrintOutput("Invalid command\n");
            break;
        
            default:
            break;
        }

        if (timeout >= 50000) PrintOutput("<ExitMonitor timed out>\n");
    }
    else if ((argc > 2) && (strcmp(argv[1], "connect") == 0))
    {
        char tmp[64];
        u8 rxdata = 0;
        u32 timeout = 0;

        sprintf(tmp, "Connecting to %s ...\n", argv[2]);
        PrintOutput(tmp);
        snprintf(tmp, 36, "Connecting to %s", argv[2]);
        TRM_SetStatusText(tmp);

        XPN_FlushBuffers();  

        XPN_SendByte('C');
        XPN_SendMessage(argv[2]);
        XPN_SendByte(0x0A);

        waitMs(500);

        PrintOutput("Received:\n");
        while ((timeout++ < vConn_time) && ((rxdata != 'C') || (rxdata != 'N')))
        {
            if (Buffer_Pop(&RxBuffer, &rxdata) == 0) TELNET_ParseRX(rxdata);
        }

        if (timeout >= vConn_time) PrintOutput("<Connection timed out>");

        PrintOutput("\n");

        TRM_SetStatusText(STATUS_TEXT);
    }
    else if (argc > 1)
    {
        char tmp[64];
        u8 rxdata = 0;
        u32 timeout = 0;

        sprintf(tmp, "Sending \"%s\"\n", argv[1]);
        PrintOutput(tmp);

        XPN_FlushBuffers();
        XPN_SendMessage(argv[1]);
        
        waitMs(200);

        PrintOutput("Received:\n");
        while (timeout++ < vConn_time)
        {
            if (Buffer_Pop(&RxBuffer, &rxdata) == 0) TELNET_ParseRX(rxdata);
        }
        PrintOutput("\n");
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
        PrintOutput(OS_Str);
        PrintOutput("\n");
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
                strcat(tmp, vUsername);
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
                sprintf(tmp, "%s%s%s %s", OS_Str, Mach_Str, STATUS_TEXT, vUsername);
            break;
        
            default:
            break;
        }
    }

    strcat(tmp, "\n");
    PrintOutput(tmp);
}

void CMD_SetConn(u8 argc, char *argv[])
{
    char tmp[64];

    if (argc < 2) 
    {
        PrintOutput("Set connection time out\n\nUsage:\nsetcon <number of ticks>\n\n");
        sprintf(tmp, "Current time out: %lu ticks\n", vConn_time);
        PrintOutput(tmp);
        return;
    }

    vConn_time = atoi32(argv[1]);

    sprintf(tmp, "Connection time out set to %lu\n", vConn_time);
    PrintOutput(tmp);
}

void CMD_ClearScreen(u8 argc, char *argv[])
{
    TTY_Reset(TRUE);
}

void CMD_TestSRAM(u8 argc, char *argv[])
{
    if (argc < 2)
    {
        PrintOutput("Test SRAM\n\nUsage:\nsram <address> - Write/Readback test\nsram -count    - Check installed RAM\n");
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
        PrintOutput(tmp);


        return;
    }

    addr = atoi32(argv[1]);

    SRAM_enable();

    // Byte read/write
    sprintf(tmp, "Writing byte $%X to $%lX\n", 0xFF, addr);
    PrintOutput(tmp);

    SRAM_writeByte(addr, 0xFF);

    if (SRAM_readByte(addr) == 0xFF) PrintOutput("Byte readback OK\n");
    else PrintOutput("Byte readback FAIL\n");

    // Word read/write
    sprintf(tmp, "Writing word $%X to $%lX\n", 0xBEEF, addr);
    PrintOutput(tmp);

    SRAM_writeWord(addr, 0xBEEF);

    if (SRAM_readWord(addr) == 0xBEEF) PrintOutput("Word readback OK\n");
    else PrintOutput("Word readback FAIL\n");

    // Long read/write
    sprintf(tmp, "Writing long $%X to $%lX\n", 0xDEADBEEF, addr);
    PrintOutput(tmp);

    SRAM_writeLong(addr, 0xDEADBEEF);

    if (SRAM_readLong(addr) == 0xDEADBEEF) PrintOutput("Long readback OK\n");
    else PrintOutput("Long readback FAIL\n");

    SRAM_disable();
}

void CMD_SetVar(u8 argc, char *argv[])
{
    if ((argc < 3) && (strcmp(argv[1], "-list")))
    {
        PrintOutput("Set variable\n\nUsage:\nsetvar <variable_name> <value>\nsetvar -list\n");
        return;
    }

    if (argc == 2)
    {
        u16 i = 0;
        char tmp[64];

        sprintf(tmp, "%-12s %s   %s\n", "Name", "Type", "Value");
        PrintOutput(tmp);

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

            PrintOutput(tmp);
            i++;
        }

        return;
    }

    PrintOutput("hi\n");
}

void CMD_GetIP(u8 argc, char *argv[])
{
    char *ipstr = malloc(32);

    if (ipstr)
    {
        PrintOutput("Please wait...\n");

        u8 r = XPN_GetIP(ipstr);

        if (r == 0)
        {
            char tmp[64];

            sprintf(tmp, "IP: %s\n", ipstr);
            PrintOutput(tmp);
        }
        else if (r == 1)
        {
            PrintOutput("Error: IPSTR is NULL!\n");
        }
        else if (r == 2)
        {
            PrintOutput("Error: Timed out\n");
        }

        free(ipstr);

        return;
    }

    PrintOutput("Error: Out of RAM!\n");
}
