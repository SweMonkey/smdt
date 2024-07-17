#include "Buffer.h"


/// @brief Check if buffer is full
/// @param b Pointer to buffer
/// @return 0xFF if full, otherwise 0
u8 Buffer_IsFull(Buffer *b)
{
    if ((b->head + 1) == b->tail) // if the head + 1 == tail, circular buffer is full
        return 0xFF;

    return 0;  // return false
}

/// @brief Check if buffer is empty
/// @param b Pointer to buffer
/// @return 0xFF if empty, otherwise 0
u8 Buffer_IsEmpty(Buffer *b)
{
    if (b->head == b->tail) // if the head == tail, circular buffer is empty
        return 0xFF;

    return 0;  // return false
}

/// @brief Get the number of bytes in buffer
/// @param b Pointer to buffer
/// @return Number of bytes in buffer
u16 Buffer_GetNum(Buffer *b)
{
    u16 num = 0;

    if (b->tail < b->head)
    {
        num = b->head - b->tail;
    }
    else if (b->tail > b->head)
    {
        num = (BUFFER_LEN - b->head) + b->tail;
    }

    return num;
}

/// @brief Push byte into buffer at head
/// @param b Pointer to buffer
/// @param data Byte data to push into buffer
/// @return 0xFF is returned if the buffer is full (data is dropped). 0 on successful push
u8 Buffer_Push(Buffer *b, u8 data)
{
    u16 next;

    next = b->head + 1;  // next is where head will point to after this write.
    if (next >= BUFFER_LEN)
        next = 0;

    if (next == b->tail) // if the head + 1 == tail, circular buffer is full
        return 0xFF;

    b->data[b->head] = data;  // Load data and then move
    b->head = next;           // head to next data offset.

    return 0;  // return success to indicate successful push.
}

/// @brief Pop buffer data at tail into return byte
/// @param b Pointer to buffer
/// @param data Return byte to pop data into
/// @return 0xFF is returned if the buffer is empty. 0 on successful pop.
u8 Buffer_Pop(Buffer *b, u8 *data)
{
    u16 next;

    if (b->head == b->tail)  // if the head == tail, we don't have any data
        return 0xFF;

    next = b->tail + 1;   // next is where tail will point to after this read.
    if(next >= BUFFER_LEN)
        next = 0;

    *data = b->data[b->tail];  // Read data and then move
    b->tail = next;            // tail to next offset.

    return 0;  // return success to indicate successful pop.
}

/// @brief Pop the byte at the head of buffer
/// @param b Pointer to buffer
/// @return 0xFF if the buffer is empty. 0 on successful pop.
u8 Buffer_ReversePop(Buffer *b)
{
    if (b->head == b->tail)  // if the head == tail, we don't have any data
        return 0xFF;

    if (b->head == 0) b->head = BUFFER_LEN-1;
    else b->head--;

    return 0;
}

/// @brief Clear the buffer
/// @param b Pointer to buffer
void Buffer_Flush(Buffer *b)
{
    b->head = 0;
    b->tail = 0;
}

/// @brief Clear the buffer and fill the first 32 entries with 0
/// @param b Pointer to buffer
void Buffer_Flush0(Buffer *b)
{
    b->head = 0;
    b->tail = 0;

    for (u8 i = 0; i < 32; i++) b->data[i] = 0;
}

/// @brief Get the last <num> of bytes up to head
/// @param b Pointer to buffer
/// @param num Number of bytes to return
/// @param r Array of bytes to return the popped data in
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
