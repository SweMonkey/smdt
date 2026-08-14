#include "Palette.h"
#include "DLog.h"

s8 sv_CBrightness = 0;
u8 sv_CLPalette = 0;
static u16 gPalette[64];
static bool bNeedsUpdate = FALSE;

// ANSI 16 colors
const u16 pColors_Xterm[16] =
{
    0x000, 0x00c, 0x0c0, 0x0cc, 0xc00, 0xc0c, 0xcc0, 0xccc,   // Normal
    0x444, 0x66e, 0x6e6, 0x6ee, 0xe64, 0xe6e, 0xee6, 0xeee,   // Highlighted
};
const u16 pColors_CGA[16] =
{
                        // 44a
    0x000, 0x00a, 0x0a0, 0x04a, 0xa00, 0xa0a, 0xaa0, 0xaaa,   // Normal
    0x444, 0x44e, 0x4e4, 0x4ee, 0xe44, 0xe4e, 0xee4, 0xeee,   // Highlighted
};
const u16 pColors_Windows[16] =
{
    0x000, 0x008, 0x080, 0x088, 0x800, 0x808, 0x880, 0xccc,   // Normal
    0x888, 0x00e, 0x0e0, 0x0ee, 0xe00, 0xe0e, 0xee0, 0xeee,   // Highlighted
};
const u16 *pColors[] = {pColors_Xterm, pColors_CGA, pColors_Windows};


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

// Colour conversion functions
// Inputs: 24bit, 666 colour cube, 666 colour cube grayscale
// Outputs: Standard ANSI colour index (30-37, 40-47, 90-97 and 100-107)

u8 ColorConv_24bit(u8 r, u8 g, u8 b, bool bFG)
{
    // Convert to 1-bit RGB
    u8 R = r >= 80;
    u8 G = g >= 80;
    u8 B = b >= 80;

    // Determine if this colour should use the upper 8 colours
    u8 bright = (r >= 192 || g >= 192 || b >= 192);
    
    // Attempt to use the bright gray colour (90/100) if the colour is gray and bright enough
    if (r >= 64 && r < 192 && ((r == g) == b))
    {
        R = G = B = 0;
        bright = 1;
    }

    u8 result = ((bright ? 90 : 30) + (bFG ? 0 : 10)) + (B << 2) + (G << 1) + R;

    ATT_LOG("Converted RGB24 to truncated RGBI1111 colour %u (R: %u  -- G: %u -- B: %u) %s", result, r, g, b, bFG?"FG":"BG");
    return result;
}

u8 ColorConv_666Cube(u8 index, bool bFG)
{
    // index = 216-cube colours index (16...231)
    u8 t  = index - 16;
    u8 r6 = t / 36;
    u8 g6 = (t % 36) / 6;
    u8 b6 = t % 6;

    // Convert to 1-bit RGB
    u8 R = r6 >= 3;
    u8 G = g6 >= 3;
    u8 B = b6 >= 3;

    // Determine if this colour should use the upper 8 colours
    u8 bright = (r6 + g6 + b6) >= 8;

    u8 result = ((bright ? 90 : 30) + (bFG ? 0 : 10)) + (B << 2) + (G << 1) + R;
    
    ATT_LOG("Converted 6x6x6 RGB cube colour - Idx: %u - R: %u - G: %u - B: %u - Result: %u", index, R, G, B, result);
    return result;
}

u8 ColorConv_666Cube_Grayscale(u8 index, bool bFG)
{
    u8 level = index - 232;     // Subtract non-grayscale colours (232...255)
    u8 which = bFG ? 0 : 10;    // Is this for the foreground or background ?
    u8 result = level >= 16 ? (97 + which) : (level >= 8 ? (90 + which) : (30 + which));

    ATT_LOG("Setting truncated grayscale colour - Idx: %u - Level: %u - Result: %u", index, level, result);
    return result;
}


// Text to colour conversion functions
// Inputs: 
// Outputs: 0BGR value

// Named colors: https://gitlab.freedesktop.org/xorg/app/rgb/blob/master/rgb.txt
static const char * const color_text[] =
{
    "black",
    "red",
    "green",
    "yellow",
    "blue",
    "magenta",
    "cyan",
    "white",
    "bright black",
    "bright red",
    "bright green",
    "bright yellow",
    "bright blue",
    "bright magenta",
    "bright cyan",
    "bright white"
};
u16 ColorConv_Text(char *str)
{
    for (u8 i = 0; i < 16; i++)
    {
        if (strcmp(str, color_text[i]) == 0)
        {
            return pColors[sv_CLPalette][i];
        }
    }

    return 0xFFFF;
}

