#ifndef IRC_H_INCLUDED
#define IRC_H_INCLUDED

#include <genesis.h>
#include "TMBuffer.h"

#define PG_EMPTYNAME "EMPTY PAGE"
#define IRC_MAX_CHANNELS 3
#define IRC_MAX_USERLIST 384    //512
#define IRC_MAX_USERNAME_LEN 12 //11

extern char sv_Username[];     // Saved preferred IRC nickname
extern char v_UsernameReset[]; // Your IRC nickname modified to suit the server (nicklen etc)
extern char sv_QuitStr[];      // IRC quit message
extern u8 PG_CurrentIdx;

extern TMBuffer *PG_Buffer[];

extern u16 PG_UserNum;
extern char **PG_UserList;

void IRC_Init();
void IRC_Reset();
void IRC_Exit();

void PrintTextLine(const u8 *str);    // Text input at bottom of screen
void IRC_ParseRX(u8 byte);
void IRC_RegisterNick();

#endif // IRC_H_INCLUDED
