#ifndef TELNET_H_INCLUDED
#define TELNET_H_INCLUDED

#include <genesis.h>

// Telnet Linemode submode commands
#define LMSM_EDIT    1
#define LMSM_TRAPSIG 2
#define LMSM_MODEACK 4

extern u8 sv_AllowRemoteEnv;
extern u8 vDECCKM;

void TELNET_Init();
void TELNET_ParseRX(u8 byte);

#endif // TELNET_H_INCLUDED
