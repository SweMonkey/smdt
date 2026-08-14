#ifndef STATUSBAR_H_INCLUDED
#define STATUSBAR_H_INCLUDED

#include <genesis.h>

// Icon x position
#define ICO_POS_0 36  // Input device icon
#define ICO_POS_1 37  // Rx icon
#define ICO_POS_2 38  // Tx icon
#define ICO_POS_3 39  // None

// Network icons
#define ICO_NET_SEND        0x1A  // Up arrow
#define ICO_NET_RECV        0x1B  // Down arrow
#define ICO_NET_IDLE_SEND   0x18
#define ICO_NET_IDLE_RECV   0x19
#define ICO_NET_ERROR       0x1D

// Input device icons
#define ICO_ID_UNKNOWN      0x1F // Unknown input device '?'
#define ICO_KB_OK           0x1C // Keyboard input device 'K'
#define ICO_JP_OK           0x1E // Joypad input device 'J'
#define ICO_ID_ERROR        0x1D // Error with input device 'X'
#define ICO_MOUSE_OK        0x20 // Mouse input device 'M'

// Undefined icon/
#define ICO_NONE  5

extern bool bStatusAtTop;
extern u8 SB_StatusY;

void SB_SetTitleMaxLen(u8 len);
void SB_SetStatusPosition(bool top);
void SB_SetStatusTextQ(const char *t);
void SB_SetStatusText(const char *t);
void SB_ScrollText();
void SB_ClearStatusText();
void SB_ResetStatusText();
void SB_ResetStatusBar();
void SB_SetStatusIcon(const char icon, u16 pos);

#endif // STATUSBAR_H_INCLUDED
