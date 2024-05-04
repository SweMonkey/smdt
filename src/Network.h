#ifndef NETWORK_H_INCLUDED
#define NETWORK_H_INCLUDED

#include <genesis.h>
#include "Buffer.h"
#include "DevMgr.h"

// Tx flags
#define TXF_NOBUFFER 1

#define ICO_NET_SEND      0x1A  // Up arrow
#define ICO_NET_RECV      0x1B  // Down arrow
#define ICO_NET_IDLE_SEND 0x18
#define ICO_NET_IDLE_RECV 0x19
#define ICO_NET_ERROR     0x1D

typedef bool NET_Connect_CB(char *str);

extern Buffer RxBuffer;
extern Buffer TxBuffer;
extern SM_Device DEV_UART;      // Built-in UART

void Ext_IRQ();

void NET_SetConnectFunc(NET_Connect_CB *cb);
bool NET_Connect(char *str);

void NET_SendChar(const u8 c, u8 flags);
void NET_TransmitBuffer();
void NET_SendString(const char *str);

#endif // NETWORK_H_INCLUDED
