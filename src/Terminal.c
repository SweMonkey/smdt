#include "Terminal.h"
#include "Buffer.h"
#include "Telnet.h"         // LMSM_EDIT
#include "../res/system.h"
#include "Utils.h"
#include "Network.h"
#include "Cursor.h"
#include "Screensaver.h"
#include "Palette.h"

#define TTY_CURSOR_X (((sv_Font?(sx << 2):(sx << 3))) + HScroll + 128)
#define TTY_CURSOR_Y ((sy << 3) - VScroll + 128)

// Modifiable variables
u8 vNewlineConv = 0;     // 0 = none (\n = \n) -- 1 = \n becomes \n\r
u8 sv_TermType = 0;      // Terminal type. See TermType table further down
u8 vDoEcho = 0;          // 0 = Do echo typed characters back to screen -- 1 = Rely on remote server to echo back typed characters
u8 vLineMode = 0;        // Line edit mode - 1 = LMSM_EDIT
u8 vBackspace = 0;       // 0 = DEL (0x7F) - 1 = ^H (0x8)
char sv_Baud[5] = "4800";// Report this baud speed to remote servers if they ask

// Font
u8 sv_Font = FONT_4x8_8; // Font size. 0=8x8 16 colour - 1=4x8 8 colour AA - 2=4x8 monochrome AA - 3=4x8 16 colour AA
u8 sv_BoldFont = FALSE;  // Use bold 8x8 font
u8 EvenOdd = 0;          // Even/Odd character being printed

// TTY
s16 sx = 0, sy = C_YSTART;              // Character x and y output position
s8 sv_HSOffset = 0;                     // HScroll offset
s16 HScroll = 0;                        // VDP horizontal scroll position
s16 VScroll = 0;                        // VDP vertical scroll position
u8 C_XMAX = 63;                         // Cursor max X position
u8 C_YMAX = C_YMAX_PAL;                 // Cursor max Y position
u8 C_XMIN = 0;                          // Cursor min X position
u8 C_YMIN = 0;                          // Cursor min Y position
u8 ColorBG = CL_BG, ColorFG = CL_FG;    // Selected BG/FG colour
u8 bIntense = FALSE;                    // Text highlighed
u8 bInverse = FALSE;                    // Text BG/FG reversed
u8 sv_bWrapAround = TRUE;               // Force wrap around at column 40/80
u8 sv_TermColumns = D_COLUMNS_80;       // Number of terminal character columns (40/80)
u8 PendingWrap = FALSE;
u8 PendingScroll = FALSE;
u16 BufferSelect = 0;
s16 Saved_VScroll = 0;

// Colours
u16 sv_CBGCL = 0;       // Custom BG colour
u16 sv_CFG0CL = 0x0AE;  // Custom text colour for 4x8 font
u16 sv_CFG1CL = 0x046;  // Custom text antialiasing colour for 4x8 font
u8 sv_bHighCL = TRUE;   // Use the upper 8 colours instead when using sv_Font=1

// Normal 4x8 and 8x8 font colours
static const u16 pColors[16] =
{
    0x222, 0x00c, 0x0c0, 0x0cc, 0xc00, 0xc0c, 0xcc0, 0xccc,   // Normal
    0x444, 0x66e, 0x6e6, 0x6ee, 0xe64, 0xe6e, 0xee6, 0xeee,   // Highlighted
};

// Inverted black/white 8x8 font colours
static const u16 pInvColors[16] =
{
    0xccc, 0x00a, 0x0a0, 0x0aa, 0xa00, 0xa0a, 0xaa0, 0x000,   // Normal
    0xeee, 0x44c, 0x4c4, 0x4cc, 0xc42, 0xc4c, 0xcc4, 0x222,   // Highlighted
};

// Normal 4x8 font antialias colours
static const u16 pColorsHalf[16] =
{
    0x000, 0x006, 0x060, 0x066, 0x600, 0x606, 0x660, 0x666,   // Shadowed (For AA)
    0x222, 0x337, 0x373, 0x377, 0x732, 0x737, 0x773, 0x777,   // Shadowed (For AA)
};

// Inverted light mode antialias colours for 4x8 font
static const u16 pInvColorsDouble[16] =
{
    0xccc, 0x00c, 0x0c0, 0x0cc, 0xc00, 0xc0c, 0xcc0, 0x444,   // Shadowed (For AA)
    0xeee, 0x66e, 0x6e6, 0x6ee, 0xe64, 0xe6e, 0xee6, 0x666,   // Shadowed (For AA)
};

// Palette and font lookup table
static const u16 PF_Table[16] = 
{
    AVR_FONT1, AVR_FONT1 + 0x2000, AVR_FONT1 + 0x4000, AVR_FONT1 + 0x6000,
    AVR_FONT0, AVR_FONT0 + 0x2000, AVR_FONT0 + 0x4000, AVR_FONT0 + 0x6000,

    AVR_FONT1+0x100, AVR_FONT1 + 0x2100, AVR_FONT1 + 0x4100, AVR_FONT1 + 0x6100,
    AVR_FONT0+0x100, AVR_FONT0 + 0x2100, AVR_FONT0 + 0x4100, AVR_FONT0 + 0x6100
};

// Plane Y lookup table
static const u16 YAddr_Table[32] = 
{
    0x0000, 0x0100, 0x0200, 0x0300, 0x0400, 0x0500, 0x0600, 0x0700, 
    0x0800, 0x0900, 0x0A00, 0x0B00, 0x0C00, 0x0D00, 0x0E00, 0x0F00, 
    0x1000, 0x1100, 0x1200, 0x1300, 0x1400, 0x1500, 0x1600, 0x1700, 
    0x1800, 0x1900, 0x1A00, 0x1B00, 0x1C00, 0x1D00, 0x1E00, 0x1F00
};

const char * const TermTypeList[] =
{
    "XTERM", "ANSI", "VT100", "MEGADRIVE", "UNKNOWN"
};


void TTY_Init(TTY_InitFlags flags)
{
    if (flags & TF_ClearScreen)
    {
        TRM_FillPlane(BG_A, 0);
        TRM_FillPlane(BG_B, 0);
    }

    switch (sv_TermColumns)
    {
        case D_COLUMNS_80:
        {
            if (flags & TF_ResetPlaneSize) VDP_setPlaneSize(128, 32, FALSE);

            if (!sv_Font) C_XMAX = DCOL8_128;
            else C_XMAX = DCOL4_128;

            break;
        }
        case D_COLUMNS_40:
        {
            if (flags & TF_ResetPlaneSize) VDP_setPlaneSize(128, 32, FALSE);

            if (!sv_Font) C_XMAX = DCOL8_64;
            else C_XMAX = DCOL4_64;
            
            break;
        }
    }

    TTY_SetSX(0);
    sy = C_YSTART;
    C_YMAX = C_SYSTEM_YMAX;
    HScroll = sv_HSOffset;
    VScroll = D_VSCROLL;
    ColorBG = CL_BG;
    ColorFG = CL_FG;
    bIntense = FALSE;
    bInverse = FALSE;
    bDoCursorBlink = TRUE;
    PendingWrap = FALSE;
    PendingScroll = FALSE;

    BufferSelect = 0;
    Saved_VScroll = 0;

    VDP_setVerticalScroll(BG_A, VScroll);
    VDP_setVerticalScroll(BG_B, VScroll);

    if (flags & TF_ReloadFont)
    {
        TTY_SetFontSize(sv_Font);
    }

    if (flags & TF_ResetPalette)
    {
        TTY_ReloadPalette();
    }

    #if (ESC_LOGGING | EMU_BUILD)// | TRM_LOGGING)
    if (flags & TF_ResetNetCount)
    {
        RXBytes = 0;
        TXBytes = 0;
    }
    #endif

    TTY_MoveCursor(TTY_CURSOR_DUMMY);
}

void TTY_SetDarkColours()
{
    if (sv_Font == FONT_4x8_1)   // 4x8 AA
    {
        SetColor(46, sv_CFG0CL);    // FG colour
        SetColor(47, sv_CFG1CL);    // AA colour
    }
    else if (sv_Font)
    {
        // Font glyph set 0 (Colours 0-3)
        SetColor(0x0A, pInvColorsDouble[0]);
        SetColor(0x0B, pInvColors[0]);

        SetColor(0x1A, pInvColorsDouble[1]);
        SetColor(0x1B, pInvColors[1]);

        SetColor(0x2A, pInvColorsDouble[2]);
        SetColor(0x2B, pInvColors[2]);

        SetColor(0x3A, pInvColorsDouble[3]);
        SetColor(0x3B, pInvColors[3]);

        // Font glyph set 1 (Colours 4-7)
        SetColor(0x08, pInvColorsDouble[4]);
        SetColor(0x09, pInvColors[4]);

        SetColor(0x18, pInvColorsDouble[5]);
        SetColor(0x19, pInvColors[5]);

        SetColor(0x28, pInvColorsDouble[6]);
        SetColor(0x29, pInvColors[6]);

        SetColor(0x38, pInvColorsDouble[7]);
        SetColor(0x39, pInvColors[7]);

        // Font glyph set 2 (Colours 12-15)
        SetColor(0x0C, pInvColorsDouble[8]);
        SetColor(0x0D, pInvColors[8]);

        SetColor(0x1C, pInvColorsDouble[9]);
        SetColor(0x1D, pInvColors[9]);

        SetColor(0x2C, pInvColorsDouble[10]);
        SetColor(0x2D, pInvColors[10]);

        SetColor(0x3C, pInvColorsDouble[11]);
        SetColor(0x3D, pInvColors[11]);

        // Font glyph set 3 (Colours 8-11)
        SetColor(0x0E, pInvColorsDouble[12]);
        SetColor(0x0F, pInvColors[12]);

        SetColor(0x1E, pInvColorsDouble[13]);
        SetColor(0x1F, pInvColors[13]);

        SetColor(0x2E, pInvColorsDouble[14]);
        SetColor(0x2F, pInvColors[14]);

        SetColor(0x3E, pInvColorsDouble[15]);
        SetColor(0x3F, pInvColors[15]);
    }
    else
    {
        SetPalette(PAL2, pInvColors);
    }
}

void TTY_ReloadPalette()
{
    if (sv_CBGCL == 0xAAA)  // Special case (Light mode)
    {
        TTY_SetDarkColours();
    }
    else if (sv_Font == FONT_4x8_16)   // 4x8
    {
        // Font glyph set 0 (Colours 0-3)
        SetColor(0x0A, pColorsHalf[0]);
        SetColor(0x0B, pColors[0]);

        SetColor(0x1A, pColorsHalf[1]);
        SetColor(0x1B, pColors[1]);

        SetColor(0x2A, pColorsHalf[2]);
        SetColor(0x2B, pColors[2]);

        SetColor(0x3A, pColorsHalf[3]);
        SetColor(0x3B, pColors[3]);

        // Font glyph set 1 (Colours 4-7)
        SetColor(0x08, pColorsHalf[4]);
        SetColor(0x09, pColors[4]);

        SetColor(0x18, pColorsHalf[5]);
        SetColor(0x19, pColors[5]);

        SetColor(0x28, pColorsHalf[6]);
        SetColor(0x29, pColors[6]);

        SetColor(0x38, pColorsHalf[7]);
        SetColor(0x39, pColors[7]);

        // Font glyph set 2 (Colours 12-15)
        SetColor(0x0C, pColorsHalf[8]);
        SetColor(0x0D, pColors[8]);

        SetColor(0x1C, pColorsHalf[9]);
        SetColor(0x1D, pColors[9]);

        SetColor(0x2C, pColorsHalf[10]);
        SetColor(0x2D, pColors[10]);

        SetColor(0x3C, pColorsHalf[11]);
        SetColor(0x3D, pColors[11]);

        // Font glyph set 3 (Colours 8-11)
        SetColor(0x0E, pColorsHalf[12]);
        SetColor(0x0F, pColors[12]);

        SetColor(0x1E, pColorsHalf[13]);
        SetColor(0x1F, pColors[13]);

        SetColor(0x2E, pColorsHalf[14]);
        SetColor(0x2F, pColors[14]);

        SetColor(0x3E, pColorsHalf[15]);
        SetColor(0x3F, pColors[15]);
    }
    else if (sv_Font == FONT_4x8_8)   // 4x8
    {
        // Font glyph set 0 (Colours 0-3)
        SetColor(0x0C, pColorsHalf[0]);
        SetColor(0x0D, pColors[(sv_bHighCL ?  8 : 8)]);    // Always use the bright colour here

        SetColor(0x1C, pColorsHalf[1]);
        SetColor(0x1D, pColors[(sv_bHighCL ?  9 : 1)]);

        SetColor(0x2C, pColorsHalf[2]);
        SetColor(0x2D, pColors[(sv_bHighCL ? 10 : 2)]);

        SetColor(0x3C, pColorsHalf[3]);
        SetColor(0x3D, pColors[(sv_bHighCL ? 11 : 3)]);

        // Font glyph set 1 (Colours 4-7)
        SetColor(0x0E, pColorsHalf[4]);
        SetColor(0x0F, pColors[(sv_bHighCL ? 12 : 4)]);

        SetColor(0x1E, pColorsHalf[5]);
        SetColor(0x1F, pColors[(sv_bHighCL ? 13 : 5)]);

        SetColor(0x2E, pColorsHalf[6]);
        SetColor(0x2F, pColors[(sv_bHighCL ? 14 : 6)]);

        SetColor(0x3E, pColorsHalf[7]);
        SetColor(0x3F, pColors[(sv_bHighCL ? 15 : 7)]);
    }
    else if (sv_Font == FONT_4x8_1)   // 4x8 AA
    {
        SetColor(47, sv_CFG0CL);    // FG colour
        SetColor(46, sv_CFG1CL);    // AA colour
    }
    else        // 8x8
    {
        SetPalette(PAL2, pColors);
    }
    
    SetColor( 0, sv_CBGCL); // VDP BG Colour
    SetColor(17, sv_CBGCL); // Window text BG Normal / Terminal text BG
    SetColor(50, sv_CBGCL); // Window text FG Inverted
}

// Todo: Clean up plane A/B when switching
void TTY_SetFontSize(u8 size)
{
    sv_Font = size;

    if (sv_Font == FONT_4x8_16)   // 4x8 16 colour AA
    {
        VDP_loadTileSet(&GFX_ASCII_TERM_SMALL_AA_16, AVR_FONT0, DMA);
        VDP_loadTileSet(&GFX_ASCII_TERM_SMALL_AA_14, AVR_FONT1, DMA); 
        VDP_loadTileSet(&GFX_ASCII_TERM_SMALL_AA_12, AVR_FONT0, DMA);   // This overwrites first half of AA_16 (Inverted characters)
        VDP_loadTileSet(&GFX_ASCII_TERM_SMALL_AA_10, AVR_FONT1, DMA);   // This overwrites first half of AA_14 (Inverted characters)

        if (BufferSelect)
        {
            VDP_setHorizontalScroll(BG_A, (HScroll+4-320));
            VDP_setHorizontalScroll(BG_B, (HScroll-320));
        }
        else
        {
            VDP_setHorizontalScroll(BG_A, HScroll+4);
            VDP_setHorizontalScroll(BG_B, HScroll  );
        }

        LastCursor = 0x13;
        EvenOdd = 1;

        if (sv_TermColumns == D_COLUMNS_80) C_XMAX = DCOL4_128;
        else C_XMAX = DCOL4_64;
    }
    else if (sv_Font == FONT_4x8_8)   // 4x8 8 colour AA
    {
        VDP_loadTileSet(&GFX_ASCII_TERM_SMALL_AA_16, AVR_FONT0, DMA);
        VDP_loadTileSet(&GFX_ASCII_TERM_SMALL_AA_14, AVR_FONT1, DMA);

        if (BufferSelect)
        {
            VDP_setHorizontalScroll(BG_A, (HScroll+4-320));
            VDP_setHorizontalScroll(BG_B, (HScroll-320));
        }
        else
        {
            VDP_setHorizontalScroll(BG_A, HScroll+4);
            VDP_setHorizontalScroll(BG_B, HScroll  );
        }

        LastCursor = 0x13;
        EvenOdd = 1;

        if (sv_TermColumns == D_COLUMNS_80) C_XMAX = DCOL4_128;
        else C_XMAX = DCOL4_64;
    }
    else if (sv_Font == FONT_4x8_1)   // 4x8 mono AA
    {
        VDP_loadTileSet(&GFX_ASCII_TERM_SMALL_AA_16, AVR_FONT0, DMA);

        if (BufferSelect)
        {
            VDP_setHorizontalScroll(BG_A, (HScroll+4-320));
            VDP_setHorizontalScroll(BG_B, (HScroll-320));
        }
        else
        {
            VDP_setHorizontalScroll(BG_A, HScroll+4);
            VDP_setHorizontalScroll(BG_B, HScroll  );
        }

        LastCursor = 0x13;
        EvenOdd = 1;

        if (sv_TermColumns == D_COLUMNS_80) C_XMAX = DCOL4_128;
        else C_XMAX = DCOL4_64;
    }
    else        // 8x8
    {
        VDP_loadTileSet((sv_BoldFont ? &GFX_ASCII_TERM_BOLD : &GFX_ASCII_TERM_NORMAL), AVR_FONT0, DMA);

        if (BufferSelect)
        {
            VDP_setHorizontalScroll(BG_A, HScroll-320);
            VDP_setHorizontalScroll(BG_B, HScroll-320);
        }
        else
        {
            VDP_setHorizontalScroll(BG_A, HScroll);
            VDP_setHorizontalScroll(BG_B, HScroll);
        }

        LastCursor = 0x10;
        
        if (sv_TermColumns == D_COLUMNS_80) C_XMAX = DCOL8_128;
        else C_XMAX = DCOL8_64;
    }

    // Update visual cursor position    
    u16 sprx = TTY_CURSOR_X;
    u16 spry = TTY_CURSOR_Y;

    // Clamp position
    sprx = sprx >= 504 ? 504 : sprx;
    spry = spry >= 504 ? 504 : spry;

    // Setup cursor sprite
    SetSprite_Y(SPRITE_ID_CURSOR, spry);
    SetSprite_X(SPRITE_ID_CURSOR, sprx);
    SetSprite_TILE(SPRITE_ID_CURSOR, LastCursor);
}

void TTY_PrintChar(u8 c)
{
    u16 addr = 0;

    // I really don't want this here, but its required to work around a "complex" wraparound issue
    if (sx >= C_XMAX)
    {
        sx = 0;
        TTY_MoveCursor(TTY_CURSOR_DOWN, 1);
    }

    switch (sv_Font)
    {
        case FONT_4x8_16: // 4x8 16 Colour
        {
            addr = ((sx+BufferSelect) & 254) + YAddr_Table[sy & 31];
            
            *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR((EvenOdd ? AVR_PLANE_B : AVR_PLANE_A) + addr);   // Set plane VRAM address
            *((vu16*) VDP_DATA_PORT) = PF_Table[ColorFG & 0xF] + c;                                         // Set plane tilemap data

            EvenOdd = sx & 1;

            break;
        }
        case FONT_4x8_8: // 4x8 8 Colour
        {
            addr = ((sx+BufferSelect) & 254) + YAddr_Table[sy & 31];
            
            *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR((EvenOdd ? AVR_PLANE_B : AVR_PLANE_A) + addr);   // Set plane VRAM address
            *((vu16*) VDP_DATA_PORT) = PF_Table[ColorFG & 0x7] + c + (bInverse ? 0 : 0x100);                // Set plane tilemap data

            EvenOdd = sx & 1;
            break;
        }
        case FONT_4x8_1: // 4x8 Mono
        {
            addr = ((sx+BufferSelect) & 254) + YAddr_Table[sy & 31];
            
            *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR((EvenOdd ? AVR_PLANE_B : AVR_PLANE_A) + addr);   // Set plane VRAM address
            *((vu16*) VDP_DATA_PORT) = AVR_FONT0 + c + (bInverse ? 0x4000 : 0x4100);                        // Set plane tilemap data

            EvenOdd = sx & 1;
            break;
        }
        case FONT_8x8_16: // 8x8
        {
            addr = (((sx+BufferSelect) & 127) << 1) + YAddr_Table[sy & 31];
            
            *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR(AVR_PLANE_B + addr);         // Set plane B VRAM address
            *((vu16*) VDP_DATA_PORT) = 0x4000 + ColorFG;                                // Set plane B tilemap data

            *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR(AVR_PLANE_A + addr);         // Set plane A VRAM address
            *((vu16*) VDP_DATA_PORT) = AVR_FONT0 + c + (bInverse ? 0x2000 : 0x2100);    // Set plane A tilemap data
            
            break;
        }
    
        default: break;
    }

    TTY_MoveCursor(TTY_CURSOR_RIGHT, 1);
}

inline void TTY_PrintString(const char *str)
{
    while (*str) TTY_PrintChar(*str++);
}

void TTY_ClearLine(u16 y, u16 line_count)
{
    u16 j;
    u16 i = line_count;
    *((vu16*) VDP_CTRL_PORT) = 0x8F02;  // Set VDP autoinc to 2

    switch (sv_Font)
    {
        case FONT_8x8_16: // 8x8
        {
            while (i--)
            {
                *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR((u32) AVR_PLANE_B + YAddr_Table[(y+i) & 31] + (BufferSelect<<1));

                j = 10; // 16 = 64 column tilemap - 32 = 128 column tilemap
                while (j--)
                {
                    *((vu16*) VDP_DATA_PORT) = 0;
                    *((vu16*) VDP_DATA_PORT) = 0;
                    *((vu16*) VDP_DATA_PORT) = 0;
                    *((vu16*) VDP_DATA_PORT) = 0;
                }
            }

            break;
        }

        default:    // 4x8
        {
            while (i--)
            {
                *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR((u32) AVR_PLANE_B + YAddr_Table[(y+i) & 31] + (BufferSelect));

                j = 10; // 16 = 64 column tilemap - 32 = 128 column tilemap
                while (j--)
                {
                    *((vu16*) VDP_DATA_PORT) = 0;
                    *((vu16*) VDP_DATA_PORT) = 0;
                    *((vu16*) VDP_DATA_PORT) = 0;
                    *((vu16*) VDP_DATA_PORT) = 0;
                }
            }

            i = line_count;
            while (i--)
            {
                *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR((u32) AVR_PLANE_A + YAddr_Table[(y+i) & 31] + (BufferSelect));

                j = 10; // 16 = 64 column tilemap - 32 = 128 column tilemap
                while (j--)
                {
                    *((vu16*) VDP_DATA_PORT) = 0;
                    *((vu16*) VDP_DATA_PORT) = 0;
                    *((vu16*) VDP_DATA_PORT) = 0;
                    *((vu16*) VDP_DATA_PORT) = 0;
                }
            }

            break;
        }
    }
}

void TTY_ClearLineSingle(u16 y)
{
    u16 j;
    *((vu16*) VDP_CTRL_PORT) = 0x8F02;  // Set VDP autoinc to 2

    switch (sv_Font)
    {
        case FONT_8x8_16: // 8x8
        {
            *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR((u32) AVR_PLANE_B + YAddr_Table[y & 31] + (BufferSelect<<1));

            j = 10; // 16 = 64 column tilemap - 32 = 128 column tilemap
            while (j--)
            {
                *((vu16*) VDP_DATA_PORT) = 0;
                *((vu16*) VDP_DATA_PORT) = 0;
                *((vu16*) VDP_DATA_PORT) = 0;
                *((vu16*) VDP_DATA_PORT) = 0;
            }

            break;
        }

        default:    // 4x8
        {
            *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR((u32) AVR_PLANE_B + YAddr_Table[y & 31] + (BufferSelect));

            j = 10; // 16 = 64 column tilemap - 32 = 128 column tilemap
            while (j--)
            {
                *((vu16*) VDP_DATA_PORT) = 0;
                *((vu16*) VDP_DATA_PORT) = 0;
                *((vu16*) VDP_DATA_PORT) = 0;
                *((vu16*) VDP_DATA_PORT) = 0;
            }

            *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR((u32) AVR_PLANE_A + YAddr_Table[y & 31] + (BufferSelect));

            j = 10; // 16 = 64 column tilemap - 32 = 128 column tilemap
            while (j--)
            {
                *((vu16*) VDP_DATA_PORT) = 0;
                *((vu16*) VDP_DATA_PORT) = 0;
                *((vu16*) VDP_DATA_PORT) = 0;
                *((vu16*) VDP_DATA_PORT) = 0;
            }

            break;
        }
    }
}

void TTY_ClearPartialLine(u16 y, u16 from_x, u16 to_x)
{
    u16 j;
    *((vu16*) VDP_CTRL_PORT) = 0x8F02;  // Set VDP autoinc to 2

    switch (sv_Font)
    {
        case FONT_8x8_16: // 8x8
        {
            *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR((u32) AVR_PLANE_B + (((from_x & 127) + ((y & 31) << 7)) << 1) + (BufferSelect<<1));

            j = (to_x - from_x) >> 2;   // NumChar / 8 --> Below sends 2 bytes * 4 (8 bytes every loop)
            while (j--)
            {
                *((vu16*) VDP_DATA_PORT) = 0;
                *((vu16*) VDP_DATA_PORT) = 0;
                *((vu16*) VDP_DATA_PORT) = 0;
                *((vu16*) VDP_DATA_PORT) = 0;
            }
            break;
        }

        default:    // 4x8
        {
            u16 from_x_ = from_x >> 1;
            u16 to_x_ = to_x >> 1;
            u16 num = (to_x_ - from_x_);   // >>3

            *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR((u32) AVR_PLANE_A + ((( (from_x_) & 127) + ((y & 31) << 7)) << 1) + (BufferSelect));        
            j = num;    // NumChar / 8 --> Below sends 2 bytes * 4 (8 bytes every loop)
            while (j--)
            {
                *((vu16*) VDP_DATA_PORT) = 0;
                //*((vu16*) VDP_DATA_PORT) = 0;
                //*((vu16*) VDP_DATA_PORT) = 0;
                //*((vu16*) VDP_DATA_PORT) = 0;
            }
            
            *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR((u32) AVR_PLANE_B + ((( (from_x_ + !EvenOdd ) & 127) + ((y & 31) << 7)) << 1) + (BufferSelect));
            j = num;    // NumChar / 8 --> Below sends 2 bytes * 4 (8 bytes every loop)
            while (j--)
            {
                *((vu16*) VDP_DATA_PORT) = 0;
                //*((vu16*) VDP_DATA_PORT) = 0;
                //*((vu16*) VDP_DATA_PORT) = 0;
                //*((vu16*) VDP_DATA_PORT) = 0;
            }
            break;
        }
    }
}

void TTY_SetAttribute(u8 v)
{
    // Foreground color: 30–37 normal, 90–97 high intensity
    if ((v >= 30 && v <= 37) || (v >= 90 && v <= 97))
    {
        // Base color
        ColorFG = (v >= 90 ? v - 82 : v - 30);

        // Apply intensity if active
        if (bIntense && v < 90) ColorFG += 8;

        return;
    }

    switch (v)
    {
        case 0:  // Reset
            ColorFG = CL_FG;
            bIntense = FALSE;
            bInverse = FALSE;
        break;

        case 1:  // Bold / increased intensity
            if (ColorFG <= 7) ColorFG += 8;
            bIntense = TRUE;
        break;

        case 2:  // Dim / decreased intensity
        case 22: // Normal intensity
            if (ColorFG >= 8) ColorFG -= 8;
            bIntense = FALSE;
        break;

        case 7:  // Inverse on
            bInverse = TRUE;
        break;

        case 27: // Inverse off
            bInverse = FALSE;
        break;

        case 39: // Reset FG color
            ColorFG = CL_FG;
        break;

        default: break;
    }
}

inline void TTY_SetSX(s16 x)
{
    sx = x < C_XMIN ? C_XMIN : x;     // sx less than 0? set to 0

    // sx greater than max_x? set to max_x
    if (sx >= C_XMAX-1)
    {
        sx = C_XMAX-1;
        PendingWrap = TRUE;
    }

    EvenOdd = !(sx & 1);
}

inline void TTY_SetSX_Relative(s16 v)
{
    s16 x = sx + v;
    sx = x < C_XMIN ? C_XMIN : x;     // sx less than 0? set to 0

    // sx greater than max_x? set to max_x
    if (sx >= C_XMAX-1)
    {
        sx = C_XMAX-1;
        PendingWrap = TRUE;
    }

    EvenOdd = !(sx & 1);
}

inline s16 TTY_GetSX()
{
    return sx;
}

inline void TTY_SetSY_A(s16 y)
{
    sy = y < C_YMIN ? C_YMIN : y;     // sy less than 0? set to 0
    
    // sy greater than max_y? set to max_y    
    if (sy >= C_YMAX-1)
    {
        sy = C_YMAX-1;
        PendingScroll = TRUE;
    }

    sy += ((VScroll >> 3) + C_YSTART);
}

inline s16 TTY_GetSY_A()
{
    return sy - ((VScroll >> 3) + C_YSTART);
}

inline void TTY_SetSY(s16 y)
{
    sy = y < C_YMIN ? C_YMIN : y;     // sy less than 0? set to 0

    // sy greater than max_y? set to max_y
    if (sy >= C_YMAX-1)
    {
        sy = C_YMAX-1;
        PendingScroll = TRUE;
    }
}

inline void TTY_SetSY_Relative(s16 v)
{
    s16 y = sy + v;
    sy = y < C_YMIN ? C_YMIN : y;     // sy less than 0? set to 0
    
    // sy greater than max_y? set to max_y
    if (sy >= C_YMAX-1)
    {
        sy = C_YMAX-1;
        PendingScroll = TRUE;
    }
}

inline s16 TTY_GetSY()
{
    return sy;
}

inline void TTY_SetVScroll(s16 v)
{
    VScroll += v;
                            
    *((vu32*) VDP_CTRL_PORT) = 0x40000010;
    *((vu16*) VDP_DATA_PORT) = VScroll;
    *((vu32*) VDP_CTRL_PORT) = 0x40020010;
    *((vu16*) VDP_DATA_PORT) = VScroll;
}

inline void TTY_SetVScrollAbs(s16 v)
{
    VScroll = v;
                            
    *((vu32*) VDP_CTRL_PORT) = 0x40000010;
    *((vu16*) VDP_DATA_PORT) = VScroll;
    *((vu32*) VDP_CTRL_PORT) = 0x40020010;
    *((vu16*) VDP_DATA_PORT) = VScroll;
}

inline void TTY_ResetVScroll()
{
    VScroll = D_VSCROLL;
                            
    *((vu32*) VDP_CTRL_PORT) = 0x40000010;
    *((vu16*) VDP_DATA_PORT) = VScroll;
    *((vu32*) VDP_CTRL_PORT) = 0x40020010;
    *((vu16*) VDP_DATA_PORT) = VScroll;
}

void TTY_MoveCursor(u8 dir, u8 num)
{
    u16 sprx, spry;

    switch (dir)
    {
        case TTY_CURSOR_RIGHT:
        {
            if (sx+num > C_XMAX)    // >=
            {
                if (sv_bWrapAround) 
                {
                    if (BufferSelect == 0)
                    {
                        sy++;

                        if (sy > (C_YMAX + (VScroll >> 3)))
                        {
                            TTY_ClearLineSingle(sy);

                            // Update vertical scroll
                            TTY_SetVScroll(8);
                        }
                    }
                    else
                    {
                        if ((sy + 1) <= C_YMAX) sy++;
                    }
                    
                    spry = TTY_CURSOR_Y;
                    SetSprite_Y(SPRITE_ID_CURSOR, spry);

                    // Disable line below and Enable line +6 below to allow cursor X to wrap back to 0
                    //TTY_SetSX(num-1);
                    sx = num-1;
                }
                else
                {
                    // Enable line below to allow cursor X to wrap back to 0
                    //TTY_SetSX(num-1);
    
                    // Disable line below if line above is Enabled!
                    //TTY_SetSX(C_XMAX);
                    sx = C_XMAX;
                }
            }
            else
            {
                //TTY_SetSX(sx+num);
                sx = sx+num;

                if (sx > C_XMAX) PendingWrap = TRUE;
            }

            sprx = TTY_CURSOR_X;
            SetSprite_X(SPRITE_ID_CURSOR, sprx);
            
            break;
        }

        case TTY_CURSOR_DOWN:
            if (BufferSelect == 0)
            {
                sy += num;
                if (sy > (C_YMAX + (VScroll >> 3)))
                {
                    TTY_ClearLine((sy-num)+1, num); // This may not be needed if the statement below is true
                    
                    if (C_YMAX < C_SYSTEM_YMAX)
                    {
                        TTY_ClearLineSingle(sy + (C_SYSTEM_YMAX - C_YMAX) + 1);
                    }

                    // Update vertical scroll
                    TTY_SetVScroll(8 * num);
                }
            }
            else
            {
                if ((sy + num) <= C_YMAX) sy += num;
            }

            spry = TTY_CURSOR_Y;
            SetSprite_Y(SPRITE_ID_CURSOR, spry);
        break;

        case TTY_CURSOR_UP:
            if (sy-num <= 0)
            {
                sy = 0;
                spry = VScroll + 128;
            }
            else 
            {
                sy -= num;
                spry = TTY_CURSOR_Y;
            }

            SetSprite_Y(SPRITE_ID_CURSOR, spry);
        break;

        case TTY_CURSOR_LEFT:
            if (sx-num < 0)
            {
                if (sv_bWrapAround) 
                {
                    TTY_SetSX(C_XMAX-(sx-num));

                    if (sy > 0)
                    {
                        sy--;
                        spry = TTY_CURSOR_Y;
                        SetSprite_Y(SPRITE_ID_CURSOR, spry);
                    }
                }
                else
                {
                    TTY_SetSX(0);
                }
            }
            else
            {
                TTY_SetSX(sx-num);
            }

            sprx = TTY_CURSOR_X;
            SetSprite_X(SPRITE_ID_CURSOR, sprx);
        break;

        default:
            // Update visual cursor position
            sprx = TTY_CURSOR_X;
            spry = TTY_CURSOR_Y;

            // Update sprite position
            SetSprite_Y(SPRITE_ID_CURSOR, spry);
            SetSprite_X(SPRITE_ID_CURSOR, sprx);
        break;
    }
}

void TTY_DrawScrollback(u8 num)
{
    if ((DMarginTop == 0) && (DMarginBottom == C_SYSTEM_YMAX))
    {
        TTY_SetVScroll(8 * num);    // +
        return;
    }

    const  u8 n       = num-1;                                 // Number of lines to scroll up
    const u16 Top     = DMarginTop + n;                        // Top row which source address will be based on
    const u16 VScrOff = (VScroll >> 3) * 256;                  // VDP VScroll offset
          u16 Rows    = (DMarginBottom - Top);                 // Number of rows to copy
    const u16 Src     = (VScrOff + (Top*256) + 256)  % 0x2000; // Source address for start of DMA copy
    const u16 Dst     = (VScrOff + (DMarginTop*256)) % 0x2000; // Destination address for DMA copy

    #if ESC_LOGGING >= 3
    kprintf("[93mScrollback Normal - num: %u[0m", num);
    kprintf("VScrOff = $%X", VScrOff);
    kprintf("n       = %u ", n);
    kprintf("Rows    = %u ", Rows);
    kprintf("Src     = $%X", Src);
    kprintf("Dst     = $%X", Dst);
    #endif

    Rows *= 256;

    if ((AVR_PLANE_A + Dst + Rows) <= (AVR_PLANE_A + 0x2000))
    {
        DMA_doVRamCopy(AVR_PLANE_A + Src, AVR_PLANE_A + Dst, Rows, 1);
        DMA_waitCompletion();
    }
    #if ESC_LOGGING >= 3
    else
    {
        kprintf("Scrollback skipped on PLANE_A; Dst = $%04X", (AVR_PLANE_A + Dst + Rows));
    }
    #endif

    if ((AVR_PLANE_B + Dst + Rows) <= (AVR_PLANE_B + 0x2000))
    {
        DMA_doVRamCopy(AVR_PLANE_B + Src, AVR_PLANE_B + Dst, Rows, 1);
        DMA_waitCompletion();
    }
    #if ESC_LOGGING >= 3
    else
    {
        kprintf("Scrollback skipped on PLANE_B; Dst = $%04X", (AVR_PLANE_A + Dst + Rows));
    }
    #endif

    TTY_ClearLine(DMarginBottom-n, n+1);
}

void TTY_DrawScrollback_RI(u8 num)
{
    // Reverse index code removed due to a few issues with it

    // Note to self; backup code is in ../tmp/reverse index.txt
}

u16 TTY_ReadCharacter(u8 x, u8 y)
{
    const u16 plane = x & 1 ? AVR_PLANE_A : AVR_PLANE_B;
    u16 addr = 0;
    s16 ny = (VScroll>>3) + y + C_YSTART;
    
    if (sv_Font)
    {
        addr = ((x+BufferSelect) & 254) + YAddr_Table[ny & 31];
    }
    else
    {
        addr = (((x+BufferSelect) & 127) << 1) + YAddr_Table[ny & 31];
    }

    *((vu32*) VDP_CTRL_PORT) = VDP_READ_VRAM_ADDR(plane + addr);
    u16 r = *((vu16*) VDP_DATA_PORT);

    //kprintf("VRAM word: $%X - VS: %d - y: %u", r, (VScroll>>3), y);

    if (r)
    {
        r -= 0x40;  // Remove font offset
        r &= 0xFF;  // Mask off attributes and fontset - This should be done outside this function later on, in case a function needs to know the attributes
    }

    //kprintf("Returning VRAM character: $%X", r);

    return r;
}
