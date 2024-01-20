#ifndef TMBUFFER_H_INCLUDED
#define TMBUFFER_H_INCLUDED

#include <genesis.h>

#define TMB_BUFFER_SIZE 0x1000

typedef struct s_tmbuffer
{
    char Title[32];
    s32 sx, sy;         // Cursor x and y position
    s16 HScroll;        // VDP horizontal scroll position
    s16 VScroll;        // VDP vertical scroll position
    u8 ColorFG;
    u16 Updates;
    u16 LastAddr;
    u8 BufferA[TMB_BUFFER_SIZE]; // <- 128x32 saved tilemap for plane a/b (Add the rest of the attributes and tileidx needed when using)
    u8 BufferB[TMB_BUFFER_SIZE];
} TMBuffer;

void TMB_UploadBuffer(TMBuffer *tptr);
void TMB_UploadBufferFull(TMBuffer *tptr);

u8 TMB_SetActiveBuffer(TMBuffer *b);
void TMB_PrintChar(u8 c);
void TMB_ClearLine(u16 y, u16 line_count);
void TMB_SetColorFG(u8 v);
void TMB_MoveCursor(u8 dir, u8 num);

void TMB_SetSX(s32 x);
s32 TMB_GetSX();
void TMB_SetSY_A(s32 y);
s32 TMB_GetSY_A();
void TMB_SetSY(s32 y);
s32 TMB_GetSY();

void TMB_SetVScroll(s16 v);
void TMB_SetHScroll(s16 h);
void TMB_ClearBuffer();

#endif // TMBUFFER_H_INCLUDED
