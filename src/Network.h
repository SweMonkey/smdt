#ifndef NETWORK_H_INCLUDED
#define NETWORK_H_INCLUDED

#include <genesis.h>
#include "Buffer.h"

// Tx flags
#define TXF_NOBUFFER 1

#define ICO_NET_SEND 0x1A       // Up arrow
#define ICO_NET_RECV 0x1B       // Down arrow
#define ICO_NET_IDLE_SEND 0x18
#define ICO_NET_IDLE_RECV 0x19
#define ICO_NET_ERROR 0x1D

extern Buffer RxBuffer;
extern Buffer TxBuffer;

void Ext_IRQ();
void NET_SendChar(const u8 c, u8 flags);
void NET_TransmitBuffer();
void NET_SendString(const char *str);

#endif // NETWORK_H_INCLUDED
