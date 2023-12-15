#ifndef IRQ_H_INCLUDED
#define IRQ_H_INCLUDED

#include <genesis.h>

#define BUFFER_LEN 4096

typedef struct s_buffer
{
    u8 data[BUFFER_LEN];
    u16 head;
    u16 tail;
} Buffer;

extern Buffer RxBuffer;
extern Buffer TxBuffer;

u8 Buffer_Push(Buffer *b, u8 data);
u8 Buffer_Pop(Buffer *b, u8 *data);
u8 Buffer_ReversePop(Buffer *b);

void VB_IRQ();
void VB2_IRQ();
void HB_IRQ();
void Ext_IRQ();

#endif // IRQ_H_INCLUDED
