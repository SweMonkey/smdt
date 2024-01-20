#ifndef IRC_H_INCLUDED
#define IRC_H_INCLUDED

#include <genesis.h>

#define PG_EMPTYNAME "EMPTY PAGE"
#define MAX_CHANNELS 3

void IRC_Init();
void IRC_Reset();

void PrintTextLine(const u8 *str);    // Text input at bottom of screen
void IRC_ParseRX(u8 byte);
void IRC_GetCurrentPageTitle(char *r);

#endif // IRC_H_INCLUDED
