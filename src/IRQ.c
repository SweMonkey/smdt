
#include "IRQ.h"
#include "main.h"   // Port definitions

static u8 CursorBlink = 0; // Cursor blink counter

void VB_IRQ()
{
    // Cursor blink
    if (CursorBlink == 20)
    {
        PAL_setColor(31, 0x0e0);
        CursorBlink++;
    }
    else if (CursorBlink == 40)
    {
        PAL_setColor(31, 0x000);
        CursorBlink = 0;
    }
    else CursorBlink++;
}

void HB_IRQ()
{
}

Buffer RxBuffer, TxBuffer;

u8 Buffer_Push(Buffer *b, u8 data)
{
    u16 next;

    next = b->head + 1;  // next is where head will point to after this write.
    if (next >= BUFFER_LEN)
        next = 0;

    if (next == b->tail) // if the head + 1 == tail, circular buffer is full
        return 0xFF;

    b->data[b->head] = data;  // Load data and then move
    b->head = next;          // head to next data offset.

    return 0;  // return success to indicate successful push.
}

u8 Buffer_Pop(Buffer *b, u8 *data)
{
    u16 next;

    if (b->head == b->tail)  // if the head == tail, we don't have any data
        return 0xFF;

    next = b->tail + 1;   // next is where tail will point to after this read.
    if(next >= BUFFER_LEN)
        next = 0;

    *data = b->data[b->tail];  // Read data and then move
    b->tail = next;           // tail to next offset.

    return 0;  // return success to indicate successful pop.
}

u8 Buffer_ReversePop(Buffer *b)
{
    if (b->head == b->tail)  // if the head == tail, we don't have any data
        return 0xFF;

    if (b->head == 0) b->head = BUFFER_LEN-1;
    else b->head--;

    return 0;
}

void Ext_IRQ()
{
    vu8 *byte = (u8*)PORT2_SRx;
    Buffer_Push(&RxBuffer, *byte);
}
