#include "CMDFunc.h"
#include "StateCtrl.h"
#include "Utils.h"
#include "Network.h"
#include "IRC.h"
#include "Telnet.h"
#include "Terminal.h"
#include "devices/XP_Network.h"
#include "devices/Keyboard_PS2.h"

void PrintOutput(const char *str);

SM_CMDList CMDList[] =
{
    {"telnet",  CMD_LaunchTelnet,   "<address:port>"},
    {"irc",     CMD_LaunchIRC,      "   <address:port>"},
    {"menu",    CMD_LaunchMenu,     "  - Run graphical start menu"},
    {"test",    CMD_Test,           "  - CMD Test"},
    {"echo",    CMD_Echo,           "  <string>"},
    {"kbc",     CMD_KeyboardSend,   "   <decimal number>"},
    {"xport",   CMD_xport,          " - Send a string to xport"},
    {"uname",   CMD_UName,          " - Print system information"},
    {"setcon",  CMD_SetConn,        "- Set connection timeout"},
    {"clear",   CMD_ClearScreen,    " - Clear screen"},
    {"help",    CMD_Help,           "  - This command"},
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

    if (argc < 2) return;

    sprintf(tmp, "%s %s\n", argv[0], argv[1]);
    PrintOutput(tmp);
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
        sprintf(tmp, "%s %s\n", CMDList[i].id, CMDList[i].desc);
        PrintOutput(tmp);

        i++;
    }
}

void CMD_xport(u8 argc, char *argv[])
{

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

        if (timeout >= 50000) PrintOutput("<EnterMonitor timed out>\n");
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
