#ifndef XP_NETWORK_H_INCLUDED
#define XP_NETWORK_H_INCLUDED

#include <genesis.h>

extern u32 sv_ConnTimeout;
extern u32 sv_ReadTimeout;
extern u32 sv_DelayTime;


u8 XPN_Initialize();

bool XPN_RXReady();
void XPN_SendByte(u8 data);
u8 XPN_ReadByte();
void XPN_SendMessage(char *str);

void XPN_FlushBuffers();
bool XPN_EnterMonitorMode();
bool XPN_ExitMonitorMode();

bool XPN_Connect(char *str);
void XPN_Disconnect();

u8 XPN_GetIP(char *ret);
void XPN_PingIP(char *ip);

#endif // XP_NETWORK_H_INCLUDED