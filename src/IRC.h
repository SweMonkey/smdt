#ifndef IRC_H_INCLUDED
#define IRC_H_INCLUDED

#include <genesis.h>
#include "TMBuffer.h"

#define PG_EMPTYNAME "EMPTY PAGE"
#define IRC_MAX_CHANNELS 5      // 3 if using a tilemap width of 128, otherwise 6. If changed; search and replace "%-22s" in IRCState.c/IRC.c, this changes the number of characters to fit in title string (-21 = 6 channels, -22 = 5 etc)
#define IRC_MAX_USERLIST 320    // 280-512
#define IRC_MAX_USERNAME_LEN 12 // 11

extern char sv_Username[];     // Saved preferred IRC nickname
extern char v_UsernameReset[]; // Your IRC nickname modified to suit the server (nicklen etc)
extern char sv_QuitStr[];      // IRC quit message
extern u8 sv_ShowJoinQuitMsg;
extern u8 sv_WrapAtScreenEdge;
extern u8 PG_CurrentIdx;

extern TMBuffer *PG_Buffer[];
extern char **PG_UserList;
extern u16 PG_UserNum;
extern u8 bPG_UpdateUserlist;
extern bool bPG_HasNewMessages[];
extern bool bPG_UpdateMessage;

void IRC_Init();
void IRC_Reset();
void IRC_Exit();

void PrintTextLine(const u8 *str);    // Text input at bottom of screen
void IRC_ParseRX(u8 byte);
void IRC_RegisterNick();
void IRC_PrintString(char *string);

#endif // IRC_H_INCLUDED
