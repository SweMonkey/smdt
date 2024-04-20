#include "Terminal.h"
#include "Buffer.h"
#include "Telnet.h"
#include "../res/system.h"
#include "Utils.h"
#include "Network.h"
#include "IRQ.h"
#include "Screensaver.h"

#ifdef EMU_BUILD
#include "StateCtrl.h"
#endif

// Modifiable variables
u8 vNewlineConv = 0;    // 0 = none (\n = \n) -- 1 = \n becomes \n\r
u8 vTermType = 0;   // See TermType table further down
u8 vDoEcho = 0;
u8 vLineMode = 0;
char vSpeed[5] = "4800";

// Font
u8 FontSize = 1;    // 0=8x8 16 colour - 1=4x8 8 colour AA - 2=4x8 monochrome AA
u8 EvenOdd = 0;
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
u8 TermColumns = D_COLUMNS_80;

// Cursor stuff
u16 LastCursor = 0x13;

// Colours
u16 Custom_BGCL = 0;
u16 Custom_FG0CL = 0xEEE;   // Custom text colour for 4x8 font
u16 Custom_FG1CL = 0x666;   // Custom text antialiasing colour for 4x8 font
u8 bHighCL = TRUE;         // Use the upper 8 colours instead when using FontSize=1

static const u16 pColors[16] =
{
    0x000, 0x00c, 0x0c0, 0x0cc, 0xc00, 0xc0c, 0xcc0, 0xccc,   // Normal
    0x444, 0x66e, 0x6e6, 0x6ee, 0xe66, 0xe6e, 0xee6, 0xeee,   // Highlighted
};

static const u16 pColorsHalf[8] =
{
    0x000, 0x006, 0x060, 0x066, 0x600, 0x606, 0x660, 0x666,   // Shadowed (For AA)
};

const char * const TermTypeList[] =
{
    "XTERM", "ANSI", "VT100", "MEGADRIVE", "UNKNOWN"
};


void TTY_Init(u8 bHardReset)
{
    if (bHardReset)
    {
        TTY_SetColumns(TermColumns);   // 128=for 80 columns - 64=for 40 columns    -- 32=No.
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
        VDP_clearPlane(BG_A, TRUE);
        VDP_clearPlane(BG_B, TRUE);
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

            if (!FontSize) C_XMAX = 126;
            else C_XMAX = 254;

            return;
        }
        case D_COLUMNS_40:
        {
            VDP_setPlaneSize(64, 32, FALSE);

            if (!FontSize) C_XMAX = 62;
            else C_XMAX = 126;

            return;
        }
    }
}

void TTY_ReloadPalette()
{
    if (FontSize == 1)   // 4x8
    {
        // Font glyph set 0 (Colours 0-3)
        PAL_setColor(0x0C, pColorsHalf[0]);
        PAL_setColor(0x0D, pColors[(bHighCL ?  8 : 8)]);    // Always use the brigth colour here

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

        Cursor_CL = 0x0E0;
    }
    else if (FontSize == 2)   // 4x8 AA
    {
        PAL_setColor(47, Custom_FG0CL);    // FG colour
        PAL_setColor(46, Custom_FG1CL);    // AA colour

        Cursor_CL = Custom_FG0CL;
    }
    else        // 8x8
    {
        PAL_setPalette(PAL2, pColors, DMA);
        Cursor_CL = 0x0E0;
    }
}

// Todo: Clean up plane A/B when switching
void TTY_SetFontSize(u8 size)
{
    FontSize = size;

    TTY_ReloadPalette();

    if (FontSize == 1)   // 4x8
    {
        VDP_loadTileSet(&GFX_ASCII_TERM_SMALL_AA, AVR_FONT0, DMA);       // 0x20
        VDP_loadTileSet(&GFX_ASCII_TERM_SMALL_AA_ALT, AVR_FONT1, DMA);  // 0x320

        VDP_setHorizontalScroll(BG_A, HScroll+4);   // -4
        VDP_setHorizontalScroll(BG_B, HScroll  );   // -8

        LastCursor = 0x13;

        EvenOdd = 1;
        LastPlane = 0;

        if (TermColumns == D_COLUMNS_80) C_XMAX = 254;   // this is a test, remove me if fucky
        else C_XMAX = 126;
    }
    else if (FontSize == 2)   // 4x8 AA
    {
        VDP_loadTileSet(&GFX_ASCII_TERM_SMALL_AA, AVR_FONT0, DMA);   // 0x20

        VDP_setHorizontalScroll(BG_A, HScroll+4);   // -4
        VDP_setHorizontalScroll(BG_B, HScroll  );   // -8

        LastCursor = 0x13;

        EvenOdd = 1;
        LastPlane = 0;

        if (TermColumns == D_COLUMNS_80) C_XMAX = 254;   // this is a test, remove me if fucky
        else C_XMAX = 126;
    }
    else        // 8x8
    {
        VDP_loadTileSet(&GFX_ASCII_TERM, AVR_FONT0, DMA);    // 0x20

        VDP_setHorizontalScroll(BG_A, HScroll);
        VDP_setHorizontalScroll(BG_B, HScroll);

        LastCursor = 0x10;
        
        if (TermColumns == D_COLUMNS_80) C_XMAX = 126;   // this is a test, remove me if fucky
        else C_XMAX = 62;
    }

    // Update visual cursor position    
    u16 sprx = ((FontSize?(sx << 2):sx << 3)) + HScroll + 128;
    u16 spry = (sy << 3) - VScroll + 128;

    // Clamp position
    sprx = sprx >= 504 ? 504 : sprx;
    spry = spry >= 504 ? 504 : spry;

    // Setup cursor sprite
    SetSprite_Y(0, spry);
    SetSprite_X(0, sprx);
    SetSprite_SIZELINK(0, SPR_SIZE_1x1, 0);
    SetSprite_TILE(0, LastCursor);
}

inline void TTY_PrintChar(u8 c)
{
    vu32 *plctrl = (u32 *)VDP_CTRL_PORT;
    vu16 *pwdata = (u16 *)VDP_DATA_PORT;
    u16 addr = 0;

    if (FontSize == 1)
    {
        addr = ((((sx >> 1) & (planeWidth - 1)) + ((sy & (planeHeight - 1)) << planeWidthSft)) * 2);

        switch (EvenOdd)
        {
            case 0: // Plane A
            {
                *plctrl = VDP_WRITE_VRAM_ADDR(VDP_BG_A + addr);
                break;
            }

            case 1: // Plane B
            {
                *plctrl = VDP_WRITE_VRAM_ADDR(VDP_BG_B + addr);
                break;
            }
            
            default:
            break;
        }

        u16 data = AVR_FONT0;
        u8 colour =  (ColorFG > 7 ? ColorFG - 8 : ColorFG); // Change colour range from 0-15 to 0-7

        if (colour < 4) data = AVR_FONT1;  // Use second font glyph set

        // Set palette to use depending on colour
        switch (colour)
        {
            case 1:
            case 5:
                data |= 0x2000;
            break;

            case 2:
            case 6:
                data |= 0x4000;
            break;

            case 3:
            case 7:
                data |= 0x6000;
            break;
        
            case 0:
            case 4:
            default:
            break;
        }

        *pwdata = data + c + (bInverse ? 0 : 0x100); // 0x40 : 0x140
        EvenOdd = (sx % 2);
    }
    else if (FontSize == 2)
    {
        addr = ((((sx >> 1) & (planeWidth - 1)) + ((sy & (planeHeight - 1)) << planeWidthSft)) * 2);

        switch (EvenOdd)
        {
            case 0: // Plane A
            {
                *plctrl = VDP_WRITE_VRAM_ADDR(VDP_BG_A + addr);
                break;
            }

            case 1: // Plane B
            {
                *plctrl = VDP_WRITE_VRAM_ADDR(VDP_BG_B + addr);
                break;
            }
            
            default:
            break;
        }

        *pwdata = AVR_FONT0 + c + (bInverse ? 0x4000 : 0x4100);   // 0x40 : 0x140
        EvenOdd = (sx % 2);
    }
    else
    {
        addr = (((sx & (planeWidth - 1)) + ((sy & (planeHeight - 1)) << planeWidthSft)) * 2);

        *plctrl = VDP_WRITE_VRAM_ADDR(VDP_BG_A + addr);
        *pwdata = AVR_FONT0 + c + (bInverse ? 0x2000 : 0x2100); // 2040 : 2140

        *plctrl = VDP_WRITE_VRAM_ADDR(VDP_BG_B + addr);
        *pwdata = 0x4000 + ColorFG;
    }

    TTY_MoveCursor(TTY_CURSOR_RIGHT, 1);
}

inline void TTY_PrintString(const char *str)
{
    for (u16 c = 0; c < strlen(str); c++)
    {
        TTY_PrintChar(str[c]);
    }
}

inline void TTY_ClearLine(u16 y, u16 line_count)
{
    vu32 *plctrl = (u32 *) VDP_CTRL_PORT;
    vu32 *pldata = (u32 *) VDP_DATA_PORT;
    u16 addr = ((((y & 31) << 7)) << 1);
    u16 j;

    VDP_setAutoInc(2);

    u16 i = line_count; // uncomment this if line_count is actually used
    while (i--)         // uncomment this if line_count is actually used
    {                   // uncomment this if line_count is actually used
        *plctrl = VDP_WRITE_VRAM_ADDR((u32) VDP_BG_B + addr);

        j = 16;//(TermColumns == D_COLUMNS_40 ? 8 : 16);    // Used to be 10
        while (j--)
        {
            *pldata = 0;
            *pldata = 0;
            *pldata = 0;
            *pldata = 0;
        }
    } // uncomment this if line_count is actually used

    // Clear BGA too if using 4x8 font
    if (FontSize)
    {
        *plctrl = VDP_WRITE_VRAM_ADDR((u32) VDP_BG_A + addr);

        j = 16;//(TermColumns == D_COLUMNS_40 ? 8 : 16);    // Used to be 10
        while (j--)
        {
            *pldata = 0;
            *pldata = 0;
            *pldata = 0;
            *pldata = 0;
        }
    }
}

inline void TTY_ClearPartialLine(u16 y, u16 from_x, u16 to_x)
{
    vu32 *plctrl = (u32 *) VDP_CTRL_PORT;
    vu32 *pldata = (u32 *) VDP_DATA_PORT;
    u16 j;

    VDP_setAutoInc(2);

    // This part may still be very iffy
    if (FontSize)
    {
        u16 from_x_ = from_x >> 1;
        u16 to_x_ = to_x >> 1;
        u16 num = (to_x_ - from_x_) >> 3;

        //kprintf("4x8 Erase from %u to %u at line %u", from_x_, to_x_, y);

        *plctrl = VDP_WRITE_VRAM_ADDR((u32) VDP_BG_A + ((( (from_x_) & 127) + ((y & 31) << 7)) << 1));        
        j = num;    // NumChar / 8 --> Below sends 2 bytes * 4 (8 bytes every loop)
        while (j--)
        {
            *pldata = 0;
            *pldata = 0;
            *pldata = 0;
            *pldata = 0;
        }
        
        *plctrl = VDP_WRITE_VRAM_ADDR((u32) VDP_BG_B + ((( (from_x_ + !EvenOdd ) & 127) + ((y & 31) << 7)) << 1));
        j = num;    // NumChar / 8 --> Below sends 2 bytes * 4 (8 bytes every loop)
        while (j--)
        {
            *pldata = 0;
            *pldata = 0;
            *pldata = 0;
            *pldata = 0;
        }
    }
    else
    {
        *plctrl = VDP_WRITE_VRAM_ADDR((u32) VDP_BG_B + (((from_x & 127) + ((y & 31) << 7)) << 1));

        j = (to_x - from_x) >> 3;   // NumChar / 8 --> Below sends 2 bytes * 4 (8 bytes every loop)
        while (j--)
        {
            *pldata = 0;
            *pldata = 0;
            *pldata = 0;
            *pldata = 0;
        }
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
        return;
    }

    if ((v >= 40) && (v <= 47))
    {
        ColorBG = v - 40;

        if (bIntense) ColorBG += 8; // Should this exist?

        #ifdef ATT_LOGGING
        kprintf("N ColorBG: $%X", ColorBG);
        #endif
        return;
    }

    // Light intensity color
    if ((v >= 90) && (v <= 97))
    {
        ColorFG = v - 82;

        #ifdef ATT_LOGGING
        kprintf("L ColorFG: $%X", ColorFG);
        #endif
        return;
    }

    if ((v >= 100) && (v <= 107))
    {
        ColorBG = v - 92;

        #ifdef ATT_LOGGING
        kprintf("L ColorBG: $%X", ColorBG);
        #endif
        return;
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

    EvenOdd = !(sx % 2);
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
    vu32 *plctrl = (u32*) VDP_CTRL_PORT;
    vu16 *pwdata = (u16*) VDP_DATA_PORT;

    switch (dir)
    {
        case TTY_CURSOR_UP:
            if (sy-num <= 0) sy = 0;
            else sy -= num;
        break;

        case TTY_CURSOR_DOWN:
            TTY_ClearLine(sy+1, num);
            sy += num;
            
            if (sy > (C_YMAX + (VScroll >> 3)))
            {
                VScroll += 8 * num;

                // Update vertical scroll
                *plctrl = VDP_WRITE_VSRAM_ADDR((u32) 0);    // 0x40000010;
                *pwdata = VScroll;
                *plctrl = VDP_WRITE_VSRAM_ADDR((u32) 2);    // 0x40020010;
                *pwdata = VScroll;
            }
        break;

        case TTY_CURSOR_LEFT:
            if (sx-num < 0)
            {
                TTY_SetSX(C_XMAX-(sx-num));
                if (bWrapAround && (sy > 0)) sy--;
            }
            else
            {
                TTY_SetSX(sx-num);
            }
        break;

        case TTY_CURSOR_RIGHT:
            if (sx+num > C_XMAX)
            {
                if (bWrapAround) 
                {
                    sy++;

                    if (sy > (C_YMAX + (VScroll >> 3)))
                    {
                        TTY_ClearLine(sy, 1);
                        VScroll += 8;// * (((sx+num)-C_XMAX)-1);
                        
                        // Update vertical scroll
                        *plctrl = VDP_WRITE_VSRAM_ADDR((u32) 0);    // 0x40000010;
                        *pwdata = VScroll;
                        *plctrl = VDP_WRITE_VSRAM_ADDR((u32) 2);    // 0x40020010;
                        *pwdata = VScroll;
                    }
                }
                TTY_SetSX((sx+num)-C_XMAX-2);
            }
            else
            {
                EvenOdd = (sx % 2);
                TTY_SetSX(sx+num);
            }
        break;

        default:
        break;
    }

    // Update visual cursor position    
    u16 sprx = ((FontSize?(sx << 2):sx << 3)) + HScroll + 128;
    u16 spry = (sy << 3) - VScroll + 128;

    // Clamp position
    sprx = sprx >= 504 ? 504 : sprx;
    spry = spry >= 504 ? 504 : spry;

    // Update sprite position
    SetSprite_Y(0, spry);
    SetSprite_X(0, sprx);

    // Reset screensaver activation counter
    InactiveCounter = 0;
}
