
#include "Exception.h"
#include "Utils.h"
#include "Terminal.h"
#include "Telnet.h"
#include "system/Stdout.h"
#include "Input.h"
#include "Keyboard.h"
#include "StateCtrl.h"

extern u32 registerState[];
extern u32 pcState;
extern u32 addrState;
extern u16 ext1State;
extern u16 ext2State;
extern u16 srState;


static inline void InitException()
{
    SYS_setInterruptMaskLevel(7);
    VDP_setEnable(TRUE);

    if (getState() != PS_Terminal) RevertState();

    Buffer_Flush(&StdoutBuffer);

    TELNET_Init(TF_Everything);
    vDoEcho = 0;
    vLineMode = LMSM_EDIT;
    vNewlineConv = 1;
    TTY_SetFontSize(FONT_8x8_16);
    TRM_SetWinHeight(1);
    Stdout_Push("?25l\n"); // Hide cursor and print newline
}

static inline void WaitForInput()
{
    u8 kbdata = 0;

    Stdout_Push("\n[97m  Press any key to reboot  [30;40m");
    Stdout_Flush();
    
    while(1)
    {
        while (KB_Poll(&kbdata))
        {
            KB_Interpret_Scancode(kbdata);
        }

        if (is_AnyKey())
        {
            SYS_hardReset();
        }

        VDP_waitVSync();
    }
}

static inline void PrintPCSR()
{
    printf("  PC: [96m%08lX[0m    SR: [96m%04X[0m\n", pcState, srState);
    printf("  Func: [95m%s[0m\n", "<not available>");   // Test: __FUNCTION__
    printf("  File: [95m%s[0m\n\n", "<not available>"); // Test: __FILE__
}

static inline void PrintRegisters()
{
    printf("  D0: [96m%08lX[0m    A0: [96m%08lX[0m\n", registerState[0], registerState[8]);
    printf("  D1: [96m%08lX[0m    A1: [96m%08lX[0m\n", registerState[1], registerState[9]);
    printf("  D2: [96m%08lX[0m    A2: [96m%08lX[0m\n", registerState[2], registerState[10]);
    printf("  D3: [96m%08lX[0m    A3: [96m%08lX[0m\n", registerState[3], registerState[11]);
    printf("  D4: [96m%08lX[0m    A4: [96m%08lX[0m\n", registerState[4], registerState[12]);
    printf("  D5: [96m%08lX[0m    A5: [96m%08lX[0m\n", registerState[5], registerState[13]);
    printf("  D6: [96m%08lX[0m    A6: [96m%08lX[0m\n", registerState[6], registerState[14]);
    printf("  D7: [96m%08lX[0m    A7: [96m%08lX[0m\n\n", registerState[7], registerState[15]);
}

static inline void PrintStack()
{
    u32 *sp = (u32*)registerState[15];

    for (u8 i = 0; i < 20; i+=2)
    {
        printf("  SP+%02X: [96m%08lX[0m    SP+%02X: [96m%08lX[0m\n", i*4, *(sp + (i + 0)), (i+1)*4, *(sp + (i + 1)));
    }
}


void __attribute__ ((noinline)) int_addresserror()
{
    InitException();

    TRM_SetStatusText("Address error exception            ");

    PrintPCSR();
    PrintRegisters();
    PrintStack();

    WaitForInput();
}

void __attribute__ ((noinline)) int_illegal()
{
    InitException();

    TRM_SetStatusText("Illegal instruction exception      ");

    PrintPCSR();
    PrintRegisters();
    PrintStack();

    WaitForInput();
}

void __attribute__ ((noinline)) int_zerodivide()
{
    InitException();

    TRM_SetStatusText("Divide by zero exception           ");

    PrintPCSR();
    PrintRegisters();
    PrintStack();

    WaitForInput();
}

void __attribute__ ((noinline)) int_error()
{
    InitException();

    TRM_SetStatusText("Error exception                    ");

    PrintPCSR();
    PrintRegisters();
    PrintStack();

    WaitForInput();
}

void __attribute__ ((noinline)) int_trapv()
{
    InitException();

    TRM_SetStatusText("TRAPV exception                    ");

    PrintPCSR();
    PrintRegisters();
    PrintStack();

    WaitForInput();
}

void __attribute__ ((noinline)) int_privviolation()
{
    InitException();

    TRM_SetStatusText("Privilege violation exception      ");

    PrintPCSR();
    PrintRegisters();
    PrintStack();

    WaitForInput();
}

void __attribute__ ((noinline)) int_trace()
{
    InitException();

    TRM_SetStatusText("Trace exception                    ");

    PrintPCSR();
    PrintRegisters();
    PrintStack();

    WaitForInput();
}

void __attribute__ ((noinline)) int_line1x1x()
{
    InitException();

    TRM_SetStatusText("Line 1x1x emulator exception       ");

    PrintPCSR();
    PrintRegisters();
    PrintStack();

    WaitForInput();
}

void __attribute__ ((noinline)) int_buserror()
{
    InitException();

    TRM_SetStatusText("Bus error exception                ");

    PrintPCSR();
    PrintRegisters();
    PrintStack();

    WaitForInput();
}

void __attribute__ ((noinline)) int_chkinst()
{
    InitException();

    TRM_SetStatusText("ChkInst exception                  ");

    PrintPCSR();
    PrintRegisters();
    PrintStack();

    WaitForInput();
}

void SetupExceptions()
{
    busErrorCB = int_buserror;
    addressErrorCB = int_addresserror;
    illegalInstCB = int_illegal;
    zeroDivideCB = int_zerodivide;
    errorExceptionCB = int_error;
    trapvInstCB = int_trapv;
    privilegeViolationCB = int_privviolation;
    traceCB = int_trace;
    line1x1xCB = int_line1x1x;
    chkInstCB = int_chkinst;
}
