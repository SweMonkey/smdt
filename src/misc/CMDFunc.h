#ifndef CMDFUNC_H_INCLUDED
#define CMDFUNC_H_INCLUDED

#include <genesis.h>

typedef const struct s_cmdlist
{
    const char *id;
    void (*fptr)(u8, char *[]);
    const char *desc;
} SM_CMDList;

extern SM_CMDList CMDList[];

void CMD_LaunchTelnet(u8 argc, char *argv[]);
void CMD_LaunchIRC(u8 argc, char *argv[]);
void CMD_LaunchMenu(u8 argc, char *argv[]);
void CMD_Test(u8 argc, char *argv[]);
void CMD_Echo(u8 argc, char *argv[]);
void CMD_KeyboardSend(u8 argc, char *argv[]);
void CMD_Help(u8 argc, char *argv[]);
void CMD_xpico(u8 argc, char *argv[]);
void CMD_UName(u8 argc, char *argv[]);
void CMD_SetConn(u8 argc, char *argv[]);
void CMD_ClearScreen(u8 argc, char *argv[]);
void CMD_TestSRAM(u8 argc, char *argv[]);
void CMD_SetVar(u8 argc, char *argv[]);
void CMD_GetIP(u8 argc, char *argv[]);
void CMD_Run(u8 argc, char *argv[]);
void CMD_Free(u8 argc, char *argv[]);

#endif // CMDFUNC_H_INCLUDED
