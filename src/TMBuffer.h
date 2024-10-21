#ifndef TMBUFFER_H_INCLUDED
#define TMBUFFER_H_INCLUDED

#include <genesis.h>

#define TMB_SIZE_SELECTOR 6                      // Tilemap width selector used in TMB - 7 = 128, 6 = 64    - DO MODIFIY THIS
#define TMB_TILEMAP_WIDTH (1<<TMB_SIZE_SELECTOR) // VDP tilemap width - 64/128      - Do not modifiy this value
#define TMB_BUFFER_SIZE (TMB_TILEMAP_WIDTH*32)   // Buffer size       - 2048/4096   - Do not modifiy this value
#define TMB_TM_W (TMB_TILEMAP_WIDTH-1)           // VDP tilemap range - 63/127      - Do not modifiy this value
#define TMB_TM_S (TMB_BUFFER_SIZE-1)             // Buffer size range - 2047/4095   - Do not modifiy this value

typedef struct s_tmbuffer
{
    char Title[32];
    s32 sx, sy;         // Cursor x and y position
    s16 HScroll;        // VDP horizontal scroll position
    s16 VScroll;        // VDP vertical scroll position
    u8 ColorFG;
    u16 Updates;
    u16 LastAddr;
    u8 BufferA[TMB_BUFFER_SIZE]; // <- 128x32 or 64x32 saved tilemap for plane a/b (Add the rest of the attributes and tileidx needed when using)
    u8 BufferB[TMB_BUFFER_SIZE];
} TMBuffer;

void TMB_UploadBuffer(TMBuffer *tptr);
void TMB_UploadBufferFull(TMBuffer *tptr);

u8 TMB_SetActiveBuffer(TMBuffer *b);
void TMB_ZeroCurrentBuffer();
void TMB_PrintChar(u8 c);
void TMB_PrintString(const char *str);
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
