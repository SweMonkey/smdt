#ifndef XP_NETWORK_H_INCLUDED
#define XP_NETWORK_H_INCLUDED

#include <genesis.h>

extern u32 vConn_time;


u8 XPN_Initialize();

void XPN_SendByte(u8 data);
bool XPN_ReadByte();
void XPN_SendMessage(char *str);

void XPN_FlushBuffers();
bool XPN_EnterMonitorMode();
bool XPN_ExitMonitorMode();

bool XPN_Connect(char *str);
void XPN_Disconnect();

u8 XPN_GetIP(char *ret);

#endif // XP_NETWORK_H_INCLUDED