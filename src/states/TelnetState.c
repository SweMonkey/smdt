#include "StateCtrl.h"
#include "Telnet.h"
#include "Terminal.h"
#include "Buffer.h"
#include "Input.h"
#include "Utils.h"
#include "Network.h"
#include "Cursor.h"             // bDoCursorBlink
#include "devices/RL_Network.h"
#include "system/Stdout.h"

static u8 rxdata;
//static u8 bOnce = FALSE;
u8 sv_TelnetFont = FONT_4x8_8;


void Enter_Telnet(u8 argc, char *argv[])
{
    sv_Font = sv_TelnetFont;
    TELNET_Init();
    TRM_SetStatusText(STATUS_TEXT);

    Buffer_Flush(&TxBuffer);
    Buffer_Flush(&RxBuffer);

    if ((argc > 1) && (strcmp("-tty", argv[1]) == 0))
    {
        vNewlineConv = 1;
        vBackspace = 1;
        vDoEcho = 1;
        vLineMode = 0;

        if (argc > 2)
        {
            NET_Connect(argv[2]);
        }
    }
    else if (argc > 1)
    {
        if (NET_Connect(argv[1]) == FALSE) 
        {
            // Connection failed; Inform the user here
        }
    }
}

void ReEnter_Telnet()
{
}

void Exit_Telnet()
{
    bDoCursorBlink = TRUE;

    Stdout_Flush();
    Buffer_Flush(&TxBuffer);
    NET_Disconnect();
}

void Reset_Telnet()
{
    TRM_SetStatusText(STATUS_TEXT);
    TTY_Reset(TRUE);
}

void Run_Telnet()
{
    while (Buffer_Pop(&RxBuffer, &rxdata))
    {
        /*if (bOnce)
        {
            TRM_SetStatusIcon(ICO_NET_RECV, ICO_POS_1);
            bOnce = !bOnce;
        }*/

        TELNET_ParseRX(rxdata);
    }
    
    /*if (!bOnce)
    {
        TRM_SetStatusIcon(ICO_NET_IDLE_RECV, ICO_POS_1);
        bOnce = !bOnce;
    }*/
}

void Input_Telnet()
{
    if (bWindowActive) return;

    if (is_KeyDown(KEY_RETURN))
    {
        /*When LINEMODE is turned on, and when in EDIT mode, when any normal
        line terminator on the client side operating system is typed, the
        line should be transmitted with "CR LF" as the line terminator.  When
        EDIT mode is turned off, a carriage return should be sent as "CR
        NUL", a line feed should be sent as LF, and any other key that cannot
        be mapped into an ASCII character, but means the line is complete
        (like a DOIT or ENTER key), should be sent as "CR LF".*/
        if (vLineMode & LMSM_EDIT)
        {
            NET_SendChar(0xD, 0); // Send \r - carridge return
            NET_SendChar(0xA, 0); // Send \n - line feed
            NET_TransmitBuffer();
        }
        /*else if (vNewlineConv == 1)
        {
            NET_SendChar(0xA, TXF_NOBUFFER); // Send \n - line feed
        }*/
        else
        {
            NET_SendChar(0xD, TXF_NOBUFFER); // Send \r - carridge return
            NET_SendChar(0xA, TXF_NOBUFFER); // Send \n - line feed
        }
    }

    if (is_KeyDown(KEY_ESCAPE))
    {
        NET_SendChar(0x1B, TXF_NOBUFFER);   // Send \ESC
    }

    if (is_KeyDown(KEY_BACKSPACE))
    {
        // 0x8  = backspace
        // 0x7F = DEL
        if (vLineMode & LMSM_EDIT)
        {
            Buffer_ReversePop(&TxBuffer);
        }
        else if (vBackspace == 1)
        {
            NET_SendChar(0x8, TXF_NOBUFFER);    // ^H
        }
        else
        {
            NET_SendChar(0x7F, TXF_NOBUFFER);   // DEL
        }
    }
    
    if (is_KeyDown(KEY_UP) || is_KeyDown(KEY_KP8_UP))
    {
        NET_SendChar(0x1B, TXF_NOBUFFER);                   // ESC
        NET_SendChar(vDECCKM ? 0x4F : 0x5B, TXF_NOBUFFER);  // O or [
        NET_SendChar('A', TXF_NOBUFFER);                    // A
    }

    if (is_KeyDown(KEY_DOWN) || is_KeyDown(KEY_KP2_DOWN))
    {
        NET_SendChar(0x1B, TXF_NOBUFFER);                   // ESC
        NET_SendChar(vDECCKM ? 0x4F : 0x5B, TXF_NOBUFFER);  // O or [
        NET_SendChar('B', TXF_NOBUFFER);                    // B
    }

    if (is_KeyDown(KEY_LEFT) || is_KeyDown(KEY_KP4_LEFT))
    {
        NET_SendChar(0x1B, TXF_NOBUFFER);                   // ESC
        NET_SendChar(vDECCKM ? 0x4F : 0x5B, TXF_NOBUFFER);  // O or [
        NET_SendChar('D', TXF_NOBUFFER);                    // D
    }

    if (is_KeyDown(KEY_RIGHT) || is_KeyDown(KEY_KP6_RIGHT))
    {
        NET_SendChar(0x1B, TXF_NOBUFFER);                   // ESC
        NET_SendChar(vDECCKM ? 0x4F : 0x5B, TXF_NOBUFFER);  // O or [
        NET_SendChar('C', TXF_NOBUFFER);                    // C
    }

    if (is_KeyDown(KEY_HOME) || is_KeyDown(KEY_KP7_HOME))
    {
        NET_SendChar(0x1B, TXF_NOBUFFER);                   // ESC
        NET_SendChar(vDECCKM ? 0x4F : 0x5B, TXF_NOBUFFER);  // 0 or [
        NET_SendChar('H', TXF_NOBUFFER);                    // H
    }

    if (is_KeyDown(KEY_END) || is_KeyDown(KEY_KP1_END))
    {
        NET_SendChar(0x1B, TXF_NOBUFFER);                   // ESC
        NET_SendChar(vDECCKM ? 0x4F : 0x5B, TXF_NOBUFFER);  // 0 or [
        NET_SendChar('F', TXF_NOBUFFER);                    // F
    }

    if (is_KeyDown(KEY_INSERT))
    {
        NET_SendChar(0x1B, TXF_NOBUFFER);   // ESC
        NET_SendChar(0x5B, TXF_NOBUFFER);   // [
        NET_SendChar('2', TXF_NOBUFFER);    // 2
        NET_SendChar('~', TXF_NOBUFFER);    // ~
    }

    if (is_KeyDown(KEY_DELETE))
    {
        NET_SendChar(0x1B, TXF_NOBUFFER);   // ESC
        NET_SendChar(0x5B, TXF_NOBUFFER);   // [
        NET_SendChar('3', TXF_NOBUFFER);    // 3
        NET_SendChar('~', TXF_NOBUFFER);    // ~
    }

    if (is_KeyDown(KEY_PGUP))
    {
        NET_SendChar(0x1B, TXF_NOBUFFER);   // ESC
        NET_SendChar(0x5B, TXF_NOBUFFER);   // [
        NET_SendChar('5', TXF_NOBUFFER);    // 3
        NET_SendChar('~', TXF_NOBUFFER);    // ~
    }

    if (is_KeyDown(KEY_PGDN))
    {
        NET_SendChar(0x1B, TXF_NOBUFFER);   // ESC
        NET_SendChar(0x5B, TXF_NOBUFFER);   // [
        NET_SendChar('6', TXF_NOBUFFER);    // 6
        NET_SendChar('~', TXF_NOBUFFER);    // ~
    }

    // Keys F1-F4 is actually supposed to be local? and to be prefixed with SS3, not CSI (xterm deprecated)
    if (is_KeyDown(KEY_F1))
    {
        NET_SendChar(0x1B, TXF_NOBUFFER);
        //NET_SendChar(0x5B, TXF_NOBUFFER);
        //NET_SendChar('1', TXF_NOBUFFER);
        //NET_SendChar('1', TXF_NOBUFFER);
        //NET_SendChar('~', TXF_NOBUFFER);
        NET_SendChar('O', TXF_NOBUFFER);
        NET_SendChar('P', TXF_NOBUFFER);
    }
    if (is_KeyDown(KEY_F2))
    {
        NET_SendChar(0x1B, TXF_NOBUFFER);
        //NET_SendChar(0x5B, TXF_NOBUFFER);
        //NET_SendChar('1', TXF_NOBUFFER);
        //NET_SendChar('2', TXF_NOBUFFER);
        //NET_SendChar('~', TXF_NOBUFFER);
        NET_SendChar('O', TXF_NOBUFFER);
        NET_SendChar('Q', TXF_NOBUFFER);
    }
    if (is_KeyDown(KEY_F3))
    {
        NET_SendChar(0x1B, TXF_NOBUFFER);
        //NET_SendChar(0x5B, TXF_NOBUFFER);
        //NET_SendChar('1', TXF_NOBUFFER);
        //NET_SendChar('3', TXF_NOBUFFER);
        //NET_SendChar('~', TXF_NOBUFFER);
        NET_SendChar('O', TXF_NOBUFFER);
        NET_SendChar('R', TXF_NOBUFFER);
    }
    if (is_KeyDown(KEY_F4))
    {
        NET_SendChar(0x1B, TXF_NOBUFFER);
        //NET_SendChar(0x5B, TXF_NOBUFFER);
        //NET_SendChar('1', TXF_NOBUFFER);
        //NET_SendChar('4', TXF_NOBUFFER);
        //NET_SendChar('~', TXF_NOBUFFER);
        NET_SendChar('O', TXF_NOBUFFER);
        NET_SendChar('S', TXF_NOBUFFER);
    }

    if (is_KeyDown(KEY_F5))
    {
        NET_SendChar(0x1B, TXF_NOBUFFER);
        NET_SendChar(0x5B, TXF_NOBUFFER);
        NET_SendChar('1', TXF_NOBUFFER);
        NET_SendChar('5', TXF_NOBUFFER);
        NET_SendChar('~', TXF_NOBUFFER);
    }
    if (is_KeyDown(KEY_F6))
    {
        NET_SendChar(0x1B, TXF_NOBUFFER);
        NET_SendChar(0x5B, TXF_NOBUFFER);
        NET_SendChar('1', TXF_NOBUFFER);
        NET_SendChar('7', TXF_NOBUFFER);
        NET_SendChar('~', TXF_NOBUFFER);
    }
    if (is_KeyDown(KEY_F7))
    {
        NET_SendChar(0x1B, TXF_NOBUFFER);
        NET_SendChar(0x5B, TXF_NOBUFFER);
        NET_SendChar('1', TXF_NOBUFFER);
        NET_SendChar('8', TXF_NOBUFFER);
        NET_SendChar('~', TXF_NOBUFFER);
    }
    if (is_KeyDown(KEY_F8))
    {
        NET_SendChar(0x1B, TXF_NOBUFFER);
        NET_SendChar(0x5B, TXF_NOBUFFER);
        NET_SendChar('1', TXF_NOBUFFER);
        NET_SendChar('9', TXF_NOBUFFER);
        NET_SendChar('~', TXF_NOBUFFER);
    }
    if (is_KeyDown(KEY_F9))
    {
        NET_SendChar(0x1B, TXF_NOBUFFER);
        NET_SendChar(0x5B, TXF_NOBUFFER);
        NET_SendChar('2', TXF_NOBUFFER);
        NET_SendChar('0', TXF_NOBUFFER);
        NET_SendChar('~', TXF_NOBUFFER);
    }
    if (is_KeyDown(KEY_F10))
    {
        NET_SendChar(0x1B, TXF_NOBUFFER);
        NET_SendChar(0x5B, TXF_NOBUFFER);
        NET_SendChar('2', TXF_NOBUFFER);
        NET_SendChar('1', TXF_NOBUFFER);
        NET_SendChar('~', TXF_NOBUFFER);
    }
    if (is_KeyDown(KEY_F11))
    {
        NET_SendChar(0x1B, TXF_NOBUFFER);
        NET_SendChar(0x5B, TXF_NOBUFFER);
        NET_SendChar('2', TXF_NOBUFFER);
        NET_SendChar('3', TXF_NOBUFFER);
        NET_SendChar('~', TXF_NOBUFFER);
    }
    if (is_KeyDown(KEY_F12))
    {
        NET_SendChar(0x1B, TXF_NOBUFFER);
        NET_SendChar(0x5B, TXF_NOBUFFER);
        NET_SendChar('2', TXF_NOBUFFER);
        NET_SendChar('4', TXF_NOBUFFER);
        NET_SendChar('~', TXF_NOBUFFER);
    }
}

const PRG_State TelnetState = 
{
    Enter_Telnet, ReEnter_Telnet, Exit_Telnet, Reset_Telnet, Run_Telnet, Input_Telnet, NULL, NULL
};

