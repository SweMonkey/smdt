#ifndef SWRENDERER_H_INCLUDED
#define SWRENDERER_H_INCLUDED

#include <genesis.h>

void SW_SetBuffer();
void SW_ResetProt();
u8 SW_GetChar(u8 x, u8 y);
void SW_ClearScreen();
void SW_Free();
void SW_Setup();
void SW_PrintChar(u8 c);
void SW_FillScreen(u8 c);
void SW_ClearLine(u16 y, u16 line_count);
void SW_ClearLineSingle(u16 y);
void SW_ClearPartialLine(u16 y, u16 from_x, u16 to_x);
void SW_RedrawScreen();
void SW_ShiftLinesDown(u8 num);
void SW_ShiftLinesUp(u8 num);
void SW_ShiftNumLinesUp(u8 row, u8 num);
void SW_ShiftRow_Right(u8 row, u8 from, u16 num);
void SW_ShiftRow_Left(u8 row, u8 from, u16 num);

#endif // SWRENDERER_H_INCLUDED
