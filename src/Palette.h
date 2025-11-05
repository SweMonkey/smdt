#ifndef PALETTE_H_INCLUDED
#define PALETTE_H_INCLUDED

#include <genesis.h>

extern s8 sv_CBrightness;

void UploadPalette();
void SetColor(u16 index, u16 value);
void SetPalette(u16 numPal, const u16 *pal);

#endif // PALETTE_H_INCLUDED
