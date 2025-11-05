#ifndef CMDFUNC_H_INCLUDED
#define CMDFUNC_H_INCLUDED

#include <genesis.h>
#include "Utils.h"

typedef const struct s_cmdlist
{
    const char *id;
    void (*fptr)(u8, char *[]);
    const char *desc;
} SM_CMDList;

extern SM_CMDList CMDList[];

void CMD_LaunchTelnet(u8 argc, char *argv[]);
void CMD_LaunchIRC(u8 argc, char *argv[]);
void CMD_LaunchGopher(u8 argc, char *argv[]);
void CMD_SetAttr(u8 argc, char *argv[]);
void CMD_Echo(u8 argc, char *argv[]);
void CMD_KeyboardSend(u8 argc, char *argv[]);
void CMD_Help(u8 argc, char *argv[]);
void CMD_xport(u8 argc, char *argv[]);
void CMD_UName(u8 argc, char *argv[]);
void CMD_SetConn(u8 argc, char *argv[]);
void CMD_ClearScreen(u8 argc, char *argv[]);
void CMD_SetVar(u8 argc, char *argv[]);
void CMD_GetIP(u8 argc, char *argv[]);
void CMD_Free(u8 argc, char *argv[]);
void CMD_Reboot(u8 argc, char *argv[]);
void CMD_SaveCFG(u8 argc, char *argv[]);
void CMD_Test(u8 argc, char *argv[]);
void CMD_FlushBuffer(u8 argc, char *argv[]);
void CMD_PrintBuffer(u8 argc, char *argv[]);
void CMD_Ping(u8 argc, char *argv[]);

void CMD_Run(u8 argc, char *argv[]);
void CMD_ChangeDir(u8 argc, char *argv[]);
void CMD_ListDir(u8 argc, char *argv[]);
void CMD_MoveFile(u8 argc, char *argv[]);
void CMD_CopyFile(u8 argc, char *argv[]);
void CMD_Concatenate(u8 argc, char *argv[]);
void CMD_Touch(u8 argc, char *argv[]);
void CMD_MakeDirectory(u8 argc, char *argv[]);
void CMD_RemoveLink(u8 argc, char *argv[]);
void CMD_HexView(u8 argc, char *argv[]);
void CMD_Attr(u8 argc, char *argv[]);
void CMD_ListBlock(u8 argc, char *argv[]);

void CMD_Uptime(u8 argc, char *argv[]);
void CMD_Date(u8 argc, char *argv[]);
void CMD_About(u8 argc, char *argv[]);

void CMD_PSGBeep(u8 argc, char *argv[]);
void CMD_HTTPWeb(u8 argc, char *argv[]);

#endif // CMDFUNC_H_INCLUDED
