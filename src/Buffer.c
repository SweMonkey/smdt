#include "Buffer.h"


/// @brief Check if buffer is full
/// @param b Pointer to buffer
/// @return TRUE if full, otherwise FALSE
bool Buffer_IsFull(Buffer *b)
{
    return ((b->head + 1 == BUFFER_LEN ? 0 : b->head + 1) == b->tail);
}

/// @brief Check if buffer is empty
/// @param b Pointer to buffer
/// @return TRUE if empty, otherwise FALSE
bool Buffer_IsEmpty(Buffer *b)
{
    return b->head == b->tail;
}

/// @brief Get the number of bytes in buffer
/// @param b Pointer to buffer
/// @return Number of bytes in buffer
u16 Buffer_GetNum(Buffer *b)
{
    if (b->head >= b->tail)
    {
        return b->head - b->tail;
    }
    else
    {
        return BUFFER_LEN - (b->tail - b->head);
    }
}

/// @brief Push byte into buffer at head
/// @param b Pointer to buffer
/// @param data Byte data to push into buffer
/// @return FALSE is returned if the buffer is full (data is dropped). TRUE on successful push
bool Buffer_Push(Buffer *b, u8 data)
{
    u16 next;

    next = b->head + 1;  // next is where head will point to after this write.
    if (next == BUFFER_LEN)
        next = 0;

    if (next == b->tail) // if the head + 1 == tail, circular buffer is full
        return FALSE;

    b->data[b->head] = data;  // Load data and then move
    b->head = next;           // head to next data offset.

    return TRUE;  // return success to indicate successful push.
}

/// @brief Push string into buffer at head
/// @param b Pointer to buffer
/// @param str String to push into buffer
/// @return FALSE is returned if the buffer is full (data is dropped). TRUE on successful push
bool Buffer_PushString(Buffer *b, const char *str)
{
    while (*str) 
    {
        if (!Buffer_Push(b, *str)) 
        {
            return FALSE;
        }
        str++;
    }
    return TRUE; // all characters pushed successfully
}

/// @brief Pop buffer data at tail into return byte
/// @param b Pointer to buffer
/// @param data Return byte to pop data into
/// @return FALSE is returned if the buffer is empty. TRUE on successful pop.
bool Buffer_Pop(Buffer *b, u8 *data)
{
    u16 next;

    if (b->head == b->tail)  // if the head == tail, we don't have any data
        return FALSE;

    next = b->tail + 1;   // next is where tail will point to after this read.
    if (next == BUFFER_LEN) next = 0;

    *data = b->data[b->tail];  // Read data and then move
    b->tail = next;            // tail to next offset.

    return TRUE;  // return success to indicate successful pop.
}

/// @brief Pop the byte at the head of buffer
/// @param b Pointer to buffer
/// @return FALSE if the buffer is empty. TRUE on successful pop.
bool Buffer_ReversePop(Buffer *b)
{
    if (b->head == b->tail)  // if the head == tail, we don't have any data
        return FALSE;

    if (b->head == 0) b->head = BUFFER_LEN-1;
    else b->head--;

    return TRUE;
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

/// @brief Get the last <num> bytes from the buffer.
/// @param b Pointer to the buffer.
/// @param num Number of bytes to return.
/// @param r Array to store the extracted bytes.
void Buffer_PeekLast(Buffer *b, u16 num, u8 r[]) 
{
    // Calculate the size of valid data in the buffer
    u16 size = (b->head >= b->tail)
               ? (b->head - b->tail)
               : (BUFFER_LEN - b->tail + b->head);

    // Determine the actual number of bytes to copy
    u16 bytesToCopy = (num > size) ? size : num;

    // Ensure the result array is fully zero-filled if no valid data is available
    if (bytesToCopy == 0) 
    {
        for (u16 i = 0; i < num; i++) r[i] = 0;
        return;
    }

    // Calculate the starting index for reading
    u16 start = (b->head >= bytesToCopy)
                ? (b->head - bytesToCopy)
                : (BUFFER_LEN + b->head - bytesToCopy);

    // Copy the valid bytes into the result array
    for (u16 i = 0; i < bytesToCopy; i++) 
    {
        u16 index = (start + i) % BUFFER_LEN;
        r[i] = b->data[index];
    }

    // Null-terminate/zero-fill the remainder of the array
    for (u16 i = bytesToCopy; i < num; i++) r[i] = 0;
}
