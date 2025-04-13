#ifndef BUFFER_H_INCLUDED
#define BUFFER_H_INCLUDED

#include <genesis.h>

#define BUFFER_LEN 1024     // Using buffer sizes below 2320 may cause the 'more' function to fail due to excessive flushing

typedef struct
{
    u16 head;
    u16 tail;
    u8 data[BUFFER_LEN];
} Buffer;

bool Buffer_IsFull(Buffer *b);
bool Buffer_IsEmpty(Buffer *b);
u16 Buffer_GetNum(Buffer *b);
bool Buffer_Push(Buffer *b, u8 data);
bool Buffer_Pop(Buffer *b, u8 *data);
bool Buffer_ReversePop(Buffer *b);

void Buffer_Flush(Buffer *b);
void Buffer_Flush0(Buffer *b);
void Buffer_PeekLast(Buffer *b, u16 num, u8 r[]);

#endif // BUFFER_H_INCLUDED
