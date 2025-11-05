#include "StateCtrl.h"
#include "Telnet.h"
#include "Terminal.h"
#include "Buffer.h"
#include "Input.h"
#include "Utils.h"
#include "Network.h"
#include "Cursor.h"             // bDoCursorBlink
#include "Keyboard.h"           // bKB_ScrLock
#include "devices/RL_Network.h"
#include "system/PseudoFile.h"

#ifdef DEBUG_STREAM
#include "misc/DebugStream.h"
#endif

static u8 rxdata;
u8 sv_TelnetFont = FONT_4x8_8;


u16 Enter_Telnet(u8 argc, char *argv[])
{
    sv_Font = sv_TelnetFont;
    TELNET_Init(TF_Everything);
    TRM_SetStatusText(STATUS_TEXT_SHORT);

    Buffer_Flush(&TxBuffer);
    Buffer_Flush(&RxBuffer);

    #ifdef DEBUG_STREAM
    if ((argc > 1) && (strcmp("-ds", argv[1]) == 0))
    {
        Run_DebugStream(argc > 2 ? atoi32(argv[2]) : 0);
    }
    else
    #endif 
    if (argc > 1)
    {
        if (NET_Connect(argv[1]) == FALSE) 
        {
            // Connection failed; Inform the user here
        }
    }

    return EXIT_SUCCESS;
}

void ReEnter_Telnet()
{
}

void Exit_Telnet()
{
    bDoCursorBlink = TRUE;

    Stdout_Flush();
    NET_Disconnect();
    Buffer_Flush(&TxBuffer);
    Buffer_Flush(&RxBuffer);

    Telnet_Quit();
}

void Reset_Telnet()
{
    TRM_SetStatusText(STATUS_TEXT_SHORT);
    TTY_Init(TF_ClearScreen);
}

void Run_Telnet()
{
    u16 it = 0;
    
    if (bKB_ScrLock) return;

    while (Buffer_Pop(&RxBuffer, &rxdata))
    {
        TELNET_ParseRX(rxdata);

        if (it++ > 16) break;   // May need tweaking, 8 is too little (buffer will overflow)
    }

    //Telnet_MouseTrack();
}

void Input_Telnet()
{
    if (bWindowActive) return;

    if (is_KeyDown(KEY_RETURN) || is_KeyDown(KEY_KP_RETURN))
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
            NET_BufferChar(0xD); // Send \r - carridge return
            NET_BufferChar(0xA); // Send \n - line feed
            NET_TransmitBuffer();
        }
        else
        {
            NET_SendChar(0xD); // Send \r - carridge return
            //NET_SendChar(0xA); // Send \n - line feed
        }
    }

    if (is_KeyDown(KEY_ESCAPE))
    {
        NET_SendChar(0x1B);   // Send \ESC
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
            NET_SendChar(0x8);    // ^H
        }
        else
        {
            NET_SendChar(0x7F);   // DEL
        }
    }
    
    if (is_KeyDown(KEY_UP))
    {
        NET_SendChar(0x1B);                   // ESC
        NET_SendChar(vDECCKM ? 0x4F : 0x5B);  // O or [
        NET_SendChar('A');                    // A
    }

    if (is_KeyDown(KEY_DOWN))
    {
        NET_SendChar(0x1B);                   // ESC
        NET_SendChar(vDECCKM ? 0x4F : 0x5B);  // O or [
        NET_SendChar('B');                    // B
    }

    if (is_KeyDown(KEY_LEFT))
    {
        NET_SendChar(0x1B);                   // ESC
        NET_SendChar(vDECCKM ? 0x4F : 0x5B);  // O or [
        NET_SendChar('D');                    // D
    }

    if (is_KeyDown(KEY_RIGHT))
    {
        NET_SendChar(0x1B);                   // ESC
        NET_SendChar(vDECCKM ? 0x4F : 0x5B);  // O or [
        NET_SendChar('C');                    // C
    }

    if (is_KeyDown(KEY_HOME))
    {
        NET_SendChar(0x1B);                   // ESC
        NET_SendChar(vDECCKM ? 0x4F : 0x5B);  // 0 or [
        NET_SendChar('H');                    // H
    }

    if (is_KeyDown(KEY_END))// || is_KeyDown(KEY_KP1_END))
    {
        NET_SendChar(0x1B);                   // ESC
        NET_SendChar(vDECCKM ? 0x4F : 0x5B);  // 0 or [
        NET_SendChar('F');                    // F
    }

    if (is_KeyDown(KEY_INSERT))
    {
        NET_SendChar(0x1B);   // ESC
        NET_SendChar(0x5B);   // [
        NET_SendChar('2');    // 2
        NET_SendChar('~');    // ~
    }

    if (is_KeyDown(KEY_DELETE))
    {
        NET_SendChar(0x1B);   // ESC
        NET_SendChar(0x5B);   // [
        NET_SendChar('3');    // 3
        NET_SendChar('~');    // ~
    }

    if (is_KeyDown(KEY_PGUP))
    {
        NET_SendChar(0x1B);   // ESC
        NET_SendChar(0x5B);   // [
        NET_SendChar('5');    // 3
        NET_SendChar('~');    // ~
    }

    if (is_KeyDown(KEY_PGDN))
    {
        NET_SendChar(0x1B);   // ESC
        NET_SendChar(0x5B);   // [
        NET_SendChar('6');    // 6
        NET_SendChar('~');    // ~
    }

    // Keys F1-F4 is actually supposed to be local? and to be prefixed with SS3, not CSI (xterm deprecated)
    if (is_KeyDown(KEY_F1))
    {
        NET_SendChar(0x1B);
        //NET_SendChar(0x5B);
        //NET_SendChar('1');
        //NET_SendChar('1');
        //NET_SendChar('~');
        NET_SendChar('O');
        NET_SendChar('P');
    }
    if (is_KeyDown(KEY_F2))
    {
        NET_SendChar(0x1B);
        //NET_SendChar(0x5B);
        //NET_SendChar('1');
        //NET_SendChar('2');
        //NET_SendChar('~');
        NET_SendChar('O');
        NET_SendChar('Q');
    }
    if (is_KeyDown(KEY_F3))
    {
        NET_SendChar(0x1B);
        //NET_SendChar(0x5B);
        //NET_SendChar('1');
        //NET_SendChar('3');
        //NET_SendChar('~');
        NET_SendChar('O');
        NET_SendChar('R');
    }
    if (is_KeyDown(KEY_F4))
    {
        NET_SendChar(0x1B);
        //NET_SendChar(0x5B);
        //NET_SendChar('1');
        //NET_SendChar('4');
        //NET_SendChar('~');
        NET_SendChar('O');
        NET_SendChar('S');
    }

    if (is_KeyDown(KEY_F5))
    {
        NET_SendChar(0x1B);
        NET_SendChar(0x5B);
        NET_SendChar('1');
        NET_SendChar('5');
        NET_SendChar('~');
    }
    if (is_KeyDown(KEY_F6))
    {
        NET_SendChar(0x1B);
        NET_SendChar(0x5B);
        NET_SendChar('1');
        NET_SendChar('7');
        NET_SendChar('~');
    }
    if (is_KeyDown(KEY_F7))
    {
        NET_SendChar(0x1B);
        NET_SendChar(0x5B);
        NET_SendChar('1');
        NET_SendChar('8');
        NET_SendChar('~');
    }
    /* Temp disable - Assume the user is trying to open SMDT's QuickMenu and not trying to send this to a remote server...
    if (is_KeyDown(KEY_F8))
    {
        NET_SendChar(0x1B);
        NET_SendChar(0x5B);
        NET_SendChar('1');
        NET_SendChar('9');
        NET_SendChar('~');
    }*/
    if (is_KeyDown(KEY_F9))
    {
        NET_SendChar(0x1B);
        NET_SendChar(0x5B);
        NET_SendChar('2');
        NET_SendChar('0');
        NET_SendChar('~');
    }
    if (is_KeyDown(KEY_F10))
    {
        NET_SendChar(0x1B);
        NET_SendChar(0x5B);
        NET_SendChar('2');
        NET_SendChar('1');
        NET_SendChar('~');
    }
    if (is_KeyDown(KEY_F11))
    {
        NET_SendChar(0x1B);
        NET_SendChar(0x5B);
        NET_SendChar('2');
        NET_SendChar('3');
        NET_SendChar('~');
    }
    if (is_KeyDown(KEY_F12))
    {
        NET_SendChar(0x1B);
        NET_SendChar(0x5B);
        NET_SendChar('2');
        NET_SendChar('4');
        NET_SendChar('~');
    }
}

const PRG_State TelnetState = 
{
    Enter_Telnet, ReEnter_Telnet, Exit_Telnet, Reset_Telnet, Run_Telnet, Input_Telnet
};

