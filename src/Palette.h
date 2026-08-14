#ifndef PALETTE_H_INCLUDED
#define PALETTE_H_INCLUDED

#include <genesis.h>

extern s8 sv_CBrightness;
extern u8 sv_CLPalette;
extern const u16 *pColors[];

void UploadPalette();
void SetColor(u16 index, u16 value);
void SetPalette(u16 numPal, const u16 *pal);

// Converts various bitdepth colours to the standard 16 ANSI colours
u8 ColorConv_24bit(u8 r, u8 g, u8 b, bool bFG);
u8 ColorConv_666Cube(u8 index, bool bFG);
u8 ColorConv_666Cube_Grayscale(u8 index, bool bFG);

u16 ColorConv_Text(char *str);

#endif // PALETTE_H_INCLUDED
