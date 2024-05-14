#include "Terminal.h"
#include "Buffer.h"
#include "Telnet.h"
#include "../res/system.h"
#include "Utils.h"
#include "Network.h"
#include "Cursor.h"
#include "Screensaver.h"

#ifdef EMU_BUILD
#include "StateCtrl.h"
#endif

// Max number of columns available
#define DCOL4_64  126   // Using 4x8 font with 64 wide tilemap
#define DCOL4_128 254   // Using 4x8 font with 128 wide tilemap
#define DCOL8_64  39    // Using 8x8 font with 64 wide tilemap
#define DCOL8_128 126   // Using 8x8 font with 128 wide tilemap

// Modifiable variables
u8 vNewlineConv = 0;    // 0 = none (\n = \n) -- 1 = \n becomes \n\r
u8 vTermType = 0;       // See TermType table further down
u8 vDoEcho = 0;         // 0 = Rely on remote server to echo back typed characters -- 1 = Do echo typed characters back to screen
u8 vLineMode = 0;       // Line edit mode
char vSpeed[5] = "4800";// Report this baud speed to remote servers if they ask

// Font
u8 FontSize = 1;        // 0=8x8 16 colour - 1=4x8 8 colour AA - 2=4x8 monochrome AA
u8 EvenOdd = 0;         // Even/Odd character being printed
static u8 LastPlane = 0;

// TTY
s32 sx = 0, sy = C_YSTART;              // Character x and y output position
s8 D_HSCROLL = 0;                       // Default HScroll offset
s16 HScroll = 0;                        // VDP horizontal scroll position
s16 VScroll = 0;                        // VDP vertical scroll position
u8 C_XMAX = 63;                         // Cursor max X position
u8 C_YMAX = C_YMAX_PAL;                 // Cursor max Y position
u8 ColorBG = CL_BG, ColorFG = CL_FG;    // Selected BG/FG colour
u8 bIntense = FALSE;                    // Text highlighed
u8 bInverse = FALSE;                    // Text BG/FG reversed
u8 bWrapAround = TRUE;                  // Force wrap around at column 40/80
u8 TermColumns = D_COLUMNS_80;          // Number of terminal character columns (40/80)

// Colours
u16 Custom_BGCL = 0;
u16 Custom_FG0CL = 0xEEE;   // Custom text colour for 4x8 font
u16 Custom_FG1CL = 0x666;   // Custom text antialiasing colour for 4x8 font
u8 bHighCL = TRUE;          // Use the upper 8 colours instead when using FontSize=1

static const u16 pColors[16] =
{
    0x000, 0x00c, 0x0c0, 0x0cc, 0xc00, 0xc0c, 0xcc0, 0xccc,   // Normal
    0x444, 0x66e, 0x6e6, 0x6ee, 0xe66, 0xe6e, 0xee6, 0xeee,   // Highlighted
};

static const u16 pColorsHalf[8] =
{
    0x000, 0x006, 0x060, 0x066, 0x600, 0x606, 0x660, 0x666,   // Shadowed (For AA)
};

// Palette and font lookup table
static const u16 PF_Table[8] = 
{
    AVR_FONT1, AVR_FONT1 + 0x2000, AVR_FONT1 + 0x4000, AVR_FONT1 + 0x6000,
    AVR_FONT0, AVR_FONT0 + 0x2000, AVR_FONT0 + 0x4000, AVR_FONT0 + 0x6000
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


void TTY_Init(u8 bHardReset)
{
    if (bHardReset)
    {
        TTY_SetColumns(TermColumns);
        RXBytes = 0;
        TXBytes = 0;
    }
    
    TTY_Reset(TRUE);
}

void TTY_Reset(u8 bClearScreen)
{
    TTY_SetSX(0);
    sy = C_YSTART;
    C_YMAX = IS_PAL_SYSTEM ? C_YMAX_PAL : C_YMAX_NTSC;
    HScroll = D_HSCROLL;
    VScroll = D_VSCROLL;
    ColorBG = CL_BG;
    ColorFG = CL_FG;
    bIntense = FALSE;
    bInverse = FALSE;
    bDoCursorBlink = TRUE;

    TTY_SetFontSize(FontSize);

    VDP_setVerticalScroll(BG_A, VScroll);
    VDP_setVerticalScroll(BG_B, VScroll);

    TRM_SetStatusIcon(ICO_NET_IDLE_RECV, ICO_POS_1);
    TRM_SetStatusIcon(ICO_NET_IDLE_SEND, ICO_POS_2);

    if (bClearScreen)
    {
        TRM_FillPlane(BG_A, 0);
        TRM_FillPlane(BG_B, 0);
    }

    TTY_MoveCursor(TTY_CURSOR_DUMMY);

    InactiveCounter = 0;
}

void TTY_SetColumns(u8 col)
{
    TermColumns = col;

    switch (TermColumns)
    {
        case D_COLUMNS_80:
        {
            VDP_setPlaneSize(128, 32, FALSE);

            if (!FontSize) C_XMAX = DCOL8_128;
            else C_XMAX = DCOL4_128;

            break;
        }
        case D_COLUMNS_40:
        {
            //VDP_setPlaneSize(64, 32, FALSE);
            VDP_setPlaneSize(128, 32, FALSE);

            if (!FontSize) C_XMAX = DCOL8_64;
            else C_XMAX = DCOL4_64;
            
            break;
        }
    }
}

void TTY_ReloadPalette()
{
    if (FontSize == 1)   // 4x8
    {
        // Font glyph set 0 (Colours 0-3)
        PAL_setColor(0x0C, pColorsHalf[0]);
        PAL_setColor(0x0D, pColors[(bHighCL ?  8 : 8)]);    // Always use the bright colour here

        PAL_setColor(0x1C, pColorsHalf[1]);
        PAL_setColor(0x1D, pColors[(bHighCL ?  9 : 1)]);

        PAL_setColor(0x2C, pColorsHalf[2]);
        PAL_setColor(0x2D, pColors[(bHighCL ? 10 : 2)]);

        PAL_setColor(0x3C, pColorsHalf[3]);
        PAL_setColor(0x3D, pColors[(bHighCL ? 11 : 3)]);

        // Font glyph set 1 (Colours 4-7)
        PAL_setColor(0x0E, pColorsHalf[4]);
        PAL_setColor(0x0F, pColors[(bHighCL ? 12 : 4)]);

        PAL_setColor(0x1E, pColorsHalf[5]);
        PAL_setColor(0x1F, pColors[(bHighCL ? 13 : 5)]);

        PAL_setColor(0x2E, pColorsHalf[6]);
        PAL_setColor(0x2F, pColors[(bHighCL ? 14 : 6)]);

        PAL_setColor(0x3E, pColorsHalf[7]);
        PAL_setColor(0x3F, pColors[(bHighCL ? 15 : 7)]);

        //Cursor_CL = 0x0E0;
    }
    else if (FontSize == 2)   // 4x8 AA
    {
        PAL_setColor(47, Custom_FG0CL);    // FG colour
        PAL_setColor(46, Custom_FG1CL);    // AA colour

        //Cursor_CL = Custom_FG0CL;
    }
    else        // 8x8
    {
        PAL_setPalette(PAL2, pColors, DMA);
        //Cursor_CL = 0x0E0;
    }
    
    PAL_setColor( 0, Custom_BGCL);
    PAL_setColor( 5, Custom_BGCL);
    PAL_setColor(17, Custom_BGCL);
    PAL_setColor(50, Custom_BGCL);
}

// Todo: Clean up plane A/B when switching
void TTY_SetFontSize(u8 size)
{
    FontSize = size;

    TTY_ReloadPalette();

    if (FontSize == 1)   // 4x8 colour AA
    {
        VDP_loadTileSet(&GFX_ASCII_TERM_SMALL_AA, AVR_FONT0, DMA);
        VDP_loadTileSet(&GFX_ASCII_TERM_SMALL_AA_ALT, AVR_FONT1, DMA);

        VDP_setHorizontalScroll(BG_A, HScroll+4);   // -4
        VDP_setHorizontalScroll(BG_B, HScroll  );   // -8

        LastCursor = 0x13;

        EvenOdd = 1;
        LastPlane = 0;

        if (TermColumns == D_COLUMNS_80) C_XMAX = DCOL4_128;
        else C_XMAX = DCOL4_64;
    }
    else if (FontSize == 2)   // 4x8 mono AA
    {
        VDP_loadTileSet(&GFX_ASCII_TERM_SMALL_AA, AVR_FONT0, DMA);

        VDP_setHorizontalScroll(BG_A, HScroll+4);   // -4
        VDP_setHorizontalScroll(BG_B, HScroll  );   // -8

        LastCursor = 0x13;

        EvenOdd = 1;
        LastPlane = 0;

        if (TermColumns == D_COLUMNS_80) C_XMAX = DCOL4_128;
        else C_XMAX = DCOL4_64;
    }
    else        // 8x8
    {
        VDP_loadTileSet(&GFX_ASCII_TERM, AVR_FONT0, DMA);

        VDP_setHorizontalScroll(BG_A, HScroll);
        VDP_setHorizontalScroll(BG_B, HScroll);

        LastCursor = 0x10;
        
        if (TermColumns == D_COLUMNS_80) C_XMAX = DCOL8_128;
        else C_XMAX = DCOL8_64;
    }

    // Update visual cursor position    
    u16 sprx = ((FontSize?(sx << 2):(sx << 3))) + HScroll + 128;
    u16 spry = (sy << 3) - VScroll + 128;

    // Clamp position
    sprx = sprx >= 504 ? 504 : sprx;
    spry = spry >= 504 ? 504 : spry;

    // Setup cursor sprite
    SetSprite_Y(CURSOR_SPRITE_NUM, spry);
    SetSprite_X(CURSOR_SPRITE_NUM, sprx);
    SetSprite_SIZELINK(CURSOR_SPRITE_NUM, SPR_SIZE_1x1, 0);
    SetSprite_TILE(CURSOR_SPRITE_NUM, LastCursor);
}

inline void TTY_PrintChar(u8 c)
{
    u16 addr = 0;

    switch (FontSize)
    {
        case 0: // 8x8
        {
            addr = ((sx & 127) << 1) + YAddr_Table[sy & 31];

            *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR(AVR_PLANE_A + addr);
            *((vu16*) VDP_DATA_PORT) = AVR_FONT0 + c + (bInverse ? 0x2000 : 0x2100);

            *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR(AVR_PLANE_B + addr);
            *((vu16*) VDP_DATA_PORT) = 0x4000 + ColorFG;
            break;
        }
        case 1: // 4x8 Colour
        {
            addr = (((sx >> 1) & 127) << 1) + YAddr_Table[sy & 31];

            switch (EvenOdd)
            {
                case 0: // Plane A
                {
                    *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR(AVR_PLANE_A + addr);
                    break;
                }

                case 1: // Plane B
                {
                    *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR(AVR_PLANE_B + addr);
                    break;
                }
                
                default:
                break;
            }

            *((vu16*) VDP_DATA_PORT) = PF_Table[ColorFG & 0x7] + c + (bInverse ? 0 : 0x100);
            EvenOdd = sx & 1;            
            break;
        }
        case 2: // 4x8 Mono
        {
            addr = (((sx >> 1) & 127) << 1) + YAddr_Table[sy & 31];

            switch (EvenOdd)
            {
                case 0: // Plane A
                {
                    *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR(AVR_PLANE_A + addr);
                    break;
                }

                case 1: // Plane B
                {
                    *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR(AVR_PLANE_B + addr);
                    break;
                }
                
                default:
                break;
            }

            *((vu16*) VDP_DATA_PORT) = AVR_FONT0 + c + (bInverse ? 0x4000 : 0x4100);
            EvenOdd = sx & 1;
            break;
        }
    
        default:
        break;
    }

    TTY_MoveCursor(TTY_CURSOR_RIGHT, 1);
}

inline void TTY_PrintString(const char *str)
{
    u16 i = 0;
    while (str[i] != '\0') TTY_PrintChar(str[i++]);
}

inline void TTY_ClearLine(u16 y, u16 line_count)
{
    u16 addr = YAddr_Table[y & 31];
    u16 j;

    //*((vu16*) VDP_CTRL_PORT) = 0x8F02;  // Set VDP autoinc to 2
    *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR((u32) AVR_PLANE_B + addr);

    u16 i = line_count;
    while (i--)
    {

        j = 32; // 16 = 64 column tilemap - 32 = 128 column tilemap
        while (j--)
        {
            *((vu16*) VDP_DATA_PORT) = 0;
            *((vu16*) VDP_DATA_PORT) = 0;
            *((vu16*) VDP_DATA_PORT) = 0;
            *((vu16*) VDP_DATA_PORT) = 0;
        }
    }

    // Clear BGA too if using 4x8 font
    if (FontSize)
    {
        *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR((u32) AVR_PLANE_A + addr);

        i = line_count;
        while (i--)
        {

            j = 32; // 16 = 64 column tilemap - 32 = 128 column tilemap
            while (j--)
            {
                *((vu16*) VDP_DATA_PORT) = 0;
                *((vu16*) VDP_DATA_PORT) = 0;
                *((vu16*) VDP_DATA_PORT) = 0;
                *((vu16*) VDP_DATA_PORT) = 0;
            }
        }
    }
}

inline void TTY_ClearLineSingle(u16 y)
{
    u16 addr = YAddr_Table[y & 31];
    u16 j;

    //*((vu16*) VDP_CTRL_PORT) = 0x8F02;  // Set VDP autoinc to 2
    *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR((u32) AVR_PLANE_B + addr);

    j = 32; // 16 = 64 column tilemap - 32 = 128 column tilemap
    while (j--)
    {
        *((vu16*) VDP_DATA_PORT) = 0;
        *((vu16*) VDP_DATA_PORT) = 0;
        *((vu16*) VDP_DATA_PORT) = 0;
        *((vu16*) VDP_DATA_PORT) = 0;
    }

    // Clear BGA too if using 4x8 font
    if (FontSize)
    {
        *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR((u32) AVR_PLANE_A + addr);

        j = 32; // 16 = 64 column tilemap - 32 = 128 column tilemap
        while (j--)
        {
            *((vu16*) VDP_DATA_PORT) = 0;
            *((vu16*) VDP_DATA_PORT) = 0;
            *((vu16*) VDP_DATA_PORT) = 0;
            *((vu16*) VDP_DATA_PORT) = 0;
        }
    }
}

inline void TTY_ClearPartialLine(u16 y, u16 from_x, u16 to_x)
{
    u16 j;

    *((vu16*) VDP_CTRL_PORT) = 0x8F02;  // Set VDP autoinc to 2

    switch (FontSize)
    {
        case 0: // 8x8
        {
            *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR((u32) AVR_PLANE_B + (((from_x & 127) + ((y & 31) << 7)) << 1));

            j = (to_x - from_x) >> 3;   // NumChar / 8 --> Below sends 2 bytes * 4 (8 bytes every loop)
            while (j--)
            {
                *((vu16*) VDP_DATA_PORT) = 0;
                *((vu16*) VDP_DATA_PORT) = 0;
                *((vu16*) VDP_DATA_PORT) = 0;
                *((vu16*) VDP_DATA_PORT) = 0;
            }
            break;
        }

        case 1: // 4x8 Colour
        case 2: // 4x8 Mono
        {
            u16 from_x_ = from_x >> 1;
            u16 to_x_ = to_x >> 1;
            u16 num = (to_x_ - from_x_) >> 3;   // Should be >> 2 without setting autoinc to 2?

            *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR((u32) AVR_PLANE_A + ((( (from_x_) & 127) + ((y & 31) << 7)) << 1));        
            j = num;    // NumChar / 8 --> Below sends 2 bytes * 4 (8 bytes every loop)
            while (j--)
            {
                *((vu16*) VDP_DATA_PORT) = 0;
                *((vu16*) VDP_DATA_PORT) = 0;
                *((vu16*) VDP_DATA_PORT) = 0;
                *((vu16*) VDP_DATA_PORT) = 0;
            }
            
            *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR((u32) AVR_PLANE_B + ((( (from_x_ + !EvenOdd ) & 127) + ((y & 31) << 7)) << 1));
            j = num;    // NumChar / 8 --> Below sends 2 bytes * 4 (8 bytes every loop)
            while (j--)
            {
                *((vu16*) VDP_DATA_PORT) = 0;
                *((vu16*) VDP_DATA_PORT) = 0;
                *((vu16*) VDP_DATA_PORT) = 0;
                *((vu16*) VDP_DATA_PORT) = 0;
            }
            break;
        }
    
        default:
        break;
    }
}

inline void TTY_SetAttribute(u8 v)
{
    // Normal intensity color
    if ((v >= 30) && (v <= 37))
    {
        ColorFG = v - 30;

        if (bIntense) ColorFG += 8;

        #ifdef ATT_LOGGING
        kprintf("N ColorFG: $%X", ColorFG);
        #endif
    }
    else if ((v >= 40) && (v <= 47))
    {
        ColorBG = v - 40;

        if (bIntense) ColorBG += 8; // Should this exist?

        #ifdef ATT_LOGGING
        kprintf("N ColorBG: $%X", ColorBG);
        #endif
    }
    // Light intensity color
    else if ((v >= 90) && (v <= 97))
    {
        ColorFG = v - 82;

        #ifdef ATT_LOGGING
        kprintf("L ColorFG: $%X", ColorFG);
        #endif
    }
    else if ((v >= 100) && (v <= 107))
    {
        ColorBG = v - 92;

        #ifdef ATT_LOGGING
        kprintf("L ColorBG: $%X", ColorBG);
        #endif
    }

    switch (v)
    {
        case 0:     // Reset
            ColorFG = CL_FG;//15;
            ColorBG = CL_BG;//0;

            bIntense = FALSE;
            bInverse = FALSE;

            #ifdef ATT_LOGGING
            kprintf("Text reset");
            #endif
        break;

        case 1:     // Bold / Increased intensity
            if (ColorFG <= 7) ColorFG += 8;

            bIntense = TRUE;

            #ifdef ATT_LOGGING
            kprintf("Text increased intensity");
            #endif
        break;

        case 2:     // Decreased intensity
            if (ColorFG >= 8) ColorFG -= 8;

            bIntense = FALSE;

            #ifdef ATT_LOGGING
            kprintf("Text decreased intensity");
            #endif
        break;

        case 7:     // Inverse on
            bInverse = TRUE;

            #ifdef ATT_LOGGING
            kprintf("Text inverse on");
            #endif
        break;

        case 22:     // Normal intensity
            if (ColorFG >= 8) ColorFG -= 8;

            bIntense = FALSE;

            #ifdef ATT_LOGGING
            kprintf("Text Normal intensity");
            #endif
        break;

        case 27:    // Inverse off
            bInverse = FALSE;

            #ifdef ATT_LOGGING
            kprintf("Text inverse off");
            #endif
        break;

        case 38:    // Set foreground color
            #ifdef ATT_LOGGING
            kprintf("Text set foreground color");
            #endif
        break;

        case 39:    // Reset FG color
            ColorFG = CL_FG;

            #ifdef ATT_LOGGING
            kprintf("Text FG color reset");
            #endif
        break;

        case 49:    // Reset BG color
            ColorBG = CL_BG;

            #ifdef ATT_LOGGING
            kprintf("Text BG color reset");
            #endif
        break;

        default:
        break;
    }
}

inline void TTY_SetSX(s32 x)
{
    sx = x<0?0:x;               // sx less than 0? set to 0
    sx = sx>C_XMAX?C_XMAX:sx;   // sx greater than max_x? set to max_x

    EvenOdd = !(sx & 1);
}

inline s32 TTY_GetSX()
{
    return sx;
}

inline void TTY_SetSY_A(s32 y)
{
    sy = y<0?0:y;               // sy less than 0? set to 0
    sy = sy>C_YMAX?C_YMAX:sy;   // sy greater than max_y? set to max_y
    sy += ((VScroll >> 3) + C_YSTART);
}

inline s32 TTY_GetSY_A()
{
    return sy - ((VScroll >> 3) + C_YSTART);
}

inline void TTY_SetSY(s32 y)
{
    sy = y<0?0:y;
}

inline s32 TTY_GetSY()
{
    return sy;
}

inline void TTY_MoveCursor(u8 dir, u8 num)
{
    u16 sprx, spry;

    switch (dir)
    {
        case TTY_CURSOR_RIGHT:
            if (sx+num > C_XMAX)
            {
                if (bWrapAround) 
                {
                    sy++;

                    if (sy > (C_YMAX + (VScroll >> 3)))
                    {
                        TTY_ClearLineSingle(sy);
                        VScroll += 8;
                        
                        // Update vertical scroll
                        *((vu32*) VDP_CTRL_PORT) = 0x40000010;
                        *((vu16*) VDP_DATA_PORT) = VScroll;
                        *((vu32*) VDP_CTRL_PORT) = 0x40020010;
                        *((vu16*) VDP_DATA_PORT) = VScroll;
                    }
                    
                    spry = (sy << 3) - VScroll + 128;
                    SetSprite_Y(CURSOR_SPRITE_NUM, spry);
                }
                TTY_SetSX((sx+num)-C_XMAX-2);
            }
            else
            {
                TTY_SetSX(sx+num);
            }

            sprx = ((FontSize?(sx << 2):(sx << 3))) + HScroll + 128;
            SetSprite_X(CURSOR_SPRITE_NUM, sprx);
        break;

        case TTY_CURSOR_DOWN:
            TTY_ClearLine(sy+1, num);
            sy += num;
            
            if (sy > (C_YMAX + (VScroll >> 3)))
            {
                VScroll += 8 * num;

                // Update vertical scroll
                *((vu32*) VDP_CTRL_PORT) = 0x40000010;
                *((vu16*) VDP_DATA_PORT) = VScroll;
                *((vu32*) VDP_CTRL_PORT) = 0x40020010;
                *((vu16*) VDP_DATA_PORT) = VScroll;
            }

            spry = (sy << 3) - VScroll + 128;
            SetSprite_Y(CURSOR_SPRITE_NUM, spry);
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
                spry = (sy << 3) - VScroll + 128;
            }

            SetSprite_Y(CURSOR_SPRITE_NUM, spry);
        break;

        case TTY_CURSOR_LEFT:
            if (sx-num < 0)
            {
                TTY_SetSX(C_XMAX-(sx-num));
                if (bWrapAround && (sy > 0)) 
                {
                    sy--;
                    spry = (sy << 3) - VScroll + 128;
                    SetSprite_Y(CURSOR_SPRITE_NUM, spry);
                }
            }
            else
            {
                TTY_SetSX(sx-num);
            }

            sprx = ((FontSize?(sx << 2):(sx << 3))) + HScroll + 128;
            SetSprite_X(CURSOR_SPRITE_NUM, sprx);
        break;

        default:
            // Update visual cursor position
            sprx = ((FontSize?(sx << 2):(sx << 3))) + HScroll + 128;
            spry = (sy << 3) - VScroll + 128;

            // Update sprite position
            SetSprite_Y(CURSOR_SPRITE_NUM, spry);
            SetSprite_X(CURSOR_SPRITE_NUM, sprx);
        break;
    }

    // Reset screensaver activation counter
    InactiveCounter = 0;
}
