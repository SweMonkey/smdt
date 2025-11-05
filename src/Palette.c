#include "Palette.h"

s8 sv_CBrightness = 0;
static u16 gPalette[64];
static bool bNeedsUpdate = FALSE;


void UploadPalette()
{
    if (!bNeedsUpdate) return;

    DMA_doDma(DMA_CRAM, gPalette, 0, 64, 2);
    DMA_waitCompletion();

    bNeedsUpdate = FALSE;
}

static u16 AdjustColor(u16 value)
{
    s8 r =  value       & 0xE;
    s8 g = (value >> 4) & 0xE;
    s8 b = (value >> 8) & 0xE;

    if (sv_CBrightness > 0)
    {
        r = r > 0 ? r + sv_CBrightness : 0;
        g = g > 0 ? g + sv_CBrightness : 0;
        b = b > 0 ? b + sv_CBrightness : 0;

        r = (r > 0xE) ? 0xE : r;
        g = (g > 0xE) ? 0xE : g;
        b = (b > 0xE) ? 0xE : b;
    }
    else if (sv_CBrightness < 0)
    {
        r += sv_CBrightness;
        g += sv_CBrightness;
        b += sv_CBrightness;

        r = (r < 0) ? 0 : r & 0xE;
        g = (g < 0) ? 0 : g & 0xE;
        b = (b < 0) ? 0 : b & 0xE;
    }
    else return value;

    return ((b << 8) | (g << 4) | r);
}

void SetColor(u16 index, u16 value)
{
    gPalette[index & 0x3F] = AdjustColor(value);
    bNeedsUpdate = TRUE;
}

void SetPalette(u16 numPal, const u16 *pal)
{
    u16 p = (numPal & 3) * 16;

    for (u8 i = 0; i < 16; i++)
    {
        gPalette[p + i] = AdjustColor(pal[i]);
    }

    bNeedsUpdate = TRUE;
}
