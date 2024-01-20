
#include "Buffer.h"


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

void Buffer_Flush(Buffer *b)
{
    b->head = 0;
    b->tail = 0;
}

// Get the last <num> bytes up to head
void Buffer_PeekLast(Buffer *b, u16 num, u8 r[])
{
    u16 tmpTail = b->tail;
    u16 c = 0;
    u16 next;

    // Get the size difference between the head and tail to determine amount of bytes we can pull from it
    if (b->tail < b->head)
    {
        tmpTail = (b->head - num) > 0 ? b->head - num : 0;  // Tail smaller than head. We must check if we actually have 'num' bytes available
    }
    else if (b->tail > b->head)
    {
        tmpTail = ((b->head-1-((num-1)-c))+BUFFER_LEN) % BUFFER_LEN;    // Tail larger than head. Wrap around and get 'num' bytes available
    }
    
    tmpTail = (tmpTail < b->tail ? b->tail : tmpTail);  // Make sure the temporary tail is not behind the real tail
    
    if (tmpTail == b->head)
    {
        // Tail == head. No bytes available, clear 'r' and return
        while (c < num)
        {
            r[c] = 0;
            c++;
        }
        return;
    }

    // Fill the array 'r' with all the bytes from head back to the tail in reverse order
    while (c < num)
    {
        if (b->head == tmpTail)  // if the head == tail, we don't have any data
            break;

        next = tmpTail + 1;   // next is where tail will point to after this read.
        if(next >= BUFFER_LEN)
            next = 0;

        r[c] = b->data[tmpTail];  // Read data and then move
        tmpTail = next;           // tail to next offset.
        c++;
    }

    // If the array 'r' is not full then fill the rest of the array with NULL
    while (c < num)
    {
        r[c] = 0;
        c++;
    }

    return;
}
