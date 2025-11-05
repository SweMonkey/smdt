#ifndef TELNET_H_INCLUDED
#define TELNET_H_INCLUDED

#include <genesis.h>
#include "Terminal.h"

// Telnet Linemode submode commands
#define LMSM_EDIT    1
#define LMSM_TRAPSIG 2
#define LMSM_MODEACK 4

typedef enum {NC_Data = 0, NC_UTF8 = 1, NC_Escape = 3, NC_IAC = 4, NC_SkipUTF = 5} NextCommand;
extern NextCommand NextByte;

extern u8 sv_AllowRemoteEnv;
extern u8 vDECCKM;
extern u8 sv_EnableUTF8;

// DECSTBM
extern s16 DMarginTop;
extern s16 DMarginBottom;

// DECLRMM
extern s16 DMarginLeft;
extern s16 DMarginRight;

void TELNET_Init(TTY_InitFlags tty_flags);
void Telnet_Quit();
void Telnet_MouseTrack();
void TELNET_ParseRX(u8 byte);

#endif // TELNET_H_INCLUDED
