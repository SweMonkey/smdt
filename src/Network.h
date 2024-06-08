#ifndef NETWORK_H_INCLUDED
#define NETWORK_H_INCLUDED

#include <genesis.h>
#include "Buffer.h"
#include "DevMgr.h"
#include "devices/RL_Network.h" // RLN_SendByte

// Tx flags
#define TXF_NOBUFFER 1

#define ICO_NET_SEND      0x1A  // Up arrow
#define ICO_NET_RECV      0x1B  // Down arrow
#define ICO_NET_IDLE_SEND 0x18
#define ICO_NET_IDLE_RECV 0x19
#define ICO_NET_ERROR     0x1D

typedef bool NET_Connect_CB(char *str);
typedef void NET_Disconnect_CB();
typedef u8 NET_GetIP_CB(char *str);
typedef void NET_PingIP_CB(char *ip);

extern Buffer RxBuffer;
extern Buffer TxBuffer;
extern SM_Device DEV_UART;      // Built-in UART

void NET_RxIRQ();
void NET_SendChar(const u8 c, u8 flags);
void NET_TransmitBuffer();
void NET_SendString(const char *str);


void NET_SetConnectFunc(NET_Connect_CB *cb);
bool NET_Connect(char *str);

void NET_SetDisconnectFunc(NET_Disconnect_CB *cb);
void NET_Disconnect();

void NET_SetGetIPFunc(NET_GetIP_CB *cb);
bool NET_GetIP(char *str);

void NET_SetPingFunc(NET_PingIP_CB *cb);
void NET_PingIP(char *ip);

#endif // NETWORK_H_INCLUDED
