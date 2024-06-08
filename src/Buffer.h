#ifndef BUFFER_H_INCLUDED
#define BUFFER_H_INCLUDED

#include <genesis.h>

#define BUFFER_LEN 1024

typedef struct s_buffer
{
    u8 data[BUFFER_LEN];
    u16 head;
    u16 tail;
} Buffer;

u8 Buffer_IsFull(Buffer *b);
u8 Buffer_IsEmpty(Buffer *b);
u8 Buffer_Push(Buffer *b, u8 data);
u8 Buffer_Pop(Buffer *b, u8 *data);
u8 Buffer_ReversePop(Buffer *b);

void Buffer_Flush(Buffer *b);
void Buffer_Flush0(Buffer *b);
void Buffer_PeekLast(Buffer *b, u16 num, u8 r[]);

#endif // BUFFER_H_INCLUDED
