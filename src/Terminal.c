
#include "Terminal.h"
#include "Buffer.h"
#include "Telnet.h"
#include "../res/system.h"
#include "Utils.h"

#ifdef EMU_BUILD
#include "StateCtrl.h"
#endif

// Modifiable variables
u8 vNewlineConv = 0;    // 0 = none (\n = \n) -- 1 = \n becomes \n\r
u8 vTermType = 0;   // See TermType table further down
u8 vDoEcho = 0;
u8 vLineMode = 0;
char *vSpeed = "4800";

// Font
u8 FontSize = 0;    // 0=8x8 - 1=4x8 - 2=4x8 AA
u8 EvenOdd = 0;
static u8 LastPlane = 0;

// Statistics
u32 RXBytes = 0;
u32 TXBytes = 0;

// TTY
u8 TTY_Initialized = FALSE;
s32 sx = 0, sy = C_YSTART;              // Character x and y output position
s16 HScroll = D_HSCROLL;                // VDP horizontal scroll position
s16 VScroll = 0;                        // VDP vertical scroll position
u8 C_XMAX = 63;                         // Cursor max X position
u8 C_YMAX = C_YMAX_PAL;                 // Cursor max Y position
u16 ColorBG = CL_BG, ColorFG = CL_FG;   // Selected BG/FG colour
u8 bIntense = FALSE;                    // Text highlighed
u8 bInverse = FALSE;                    // Text BG/FG reversed
u8 bWrapAround = TRUE;                  // Force wrap around at column 40/80
u8 TermColumns = D_COLUMNS_80;

SM_Device DEV_UART;

extern u16 Cursor_CL;
static const u16 pColors[16] =
{
    0x000, 0x00c, 0x0c0, 0x0cc, 0xc00, 0xc0c, 0xcc0, 0xccc,   // Normal
    0x444, 0x66e, 0x6e6, 0x6ee, 0xe66, 0xe6e, 0xee6, 0xeee,   // Highlighted
};

/*static const u16 pColorsMONO[16] =
{
    0x000, 0xccc, 0xccc, 0xccc, 0xccc, 0xccc, 0xccc, 0xccc,   // Normal
    0x444, 0xeee, 0xeee, 0xeee, 0xeee, 0xeee, 0xeee, 0xeee,   // Highlighted
};*/


void TTY_Init(u8 bHardReset)
{
    if (bHardReset)
    {
        TTY_SetColumns(D_COLUMNS_80);   // 128=for 80 columns - 64=for 40 columns    -- 32=No.
        RXBytes = 0;
        TXBytes = 0;
    }
    
    TTY_Reset(FALSE);
    TTY_Initialized = TRUE;
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
    bWrapAround = TRUE;

    vNewlineConv = 0;
    vTermType = 0;
    vDoEcho = 0;
    vLineMode = 0;
    vSpeed = "4800";

    PAL_setPalette(PAL2, pColors, DMA);
    PAL_setColor(2, 0x0e0);
    PAL_setColor(31, 0x0e0);

    TTY_SetFontSize(FontSize);

    VDP_setVerticalScroll(BG_A, VScroll);
    VDP_setVerticalScroll(BG_B, VScroll);

    print_charXY_WP(ICO_NET_IDLE_RECV, STATUS_NET_RECV_POS, CHAR_WHITE);
    print_charXY_WP(ICO_NET_IDLE_SEND, STATUS_NET_SEND_POS, CHAR_WHITE);

    if (bClearScreen)
    {
        VDP_clearPlane(BG_A, TRUE);
        VDP_clearPlane(BG_B, TRUE);
    }

    TTY_MoveCursor(TTY_CURSOR_DUMMY);
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

// Todo: Clean up plane A/B when switching
void TTY_SetFontSize(u8 size)
{
    vu32 *plctrl = (u32 *)VDP_CTRL_PORT;
    vu16 *pwdata = (u16 *)VDP_DATA_PORT;
    FontSize = size;

    if (FontSize == 1)   // 4x8
    {
        VDP_loadTileSet(&GFX_ASCII_TERM_SMALL, 0x20, DMA);

        VDP_setHorizontalScroll(BG_A, HScroll+4);   // -4
        VDP_setHorizontalScroll(BG_B, HScroll  );   // -8

        PAL_setColor(47, 0xEEE);    // FG colour
        Cursor_CL = 0x0E0;

        *plctrl = VDP_WRITE_VRAM_ADDR(0xAC04);
        *pwdata = 0x21FB;

        EvenOdd = 0;
        LastPlane = 0;

        if (TermColumns == D_COLUMNS_80) C_XMAX = 254;   // this is a test, remove me if fucky
        else C_XMAX = 126;
    }
    else if (FontSize == 2)   // 4x8 AA
    {
        VDP_loadTileSet(&GFX_ASCII_TERM_SMALL_AA, 0x20, DMA);

        VDP_setHorizontalScroll(BG_A, HScroll+4);   // -4
        VDP_setHorizontalScroll(BG_B, HScroll  );   // -8

        PAL_setColor(47, 0xEEE);    // FG colour
        PAL_setColor(46, 0x666);    // AA colour
        Cursor_CL = 0x0E0;

        *plctrl = VDP_WRITE_VRAM_ADDR(0xAC04);
        *pwdata = 0x21FB;

        EvenOdd = 0;
        LastPlane = 0;

        if (TermColumns == D_COLUMNS_80) C_XMAX = 254;   // this is a test, remove me if fucky
        else C_XMAX = 126;
    }
    else        // 8x8
    {
        VDP_loadTileSet(&GFX_ASCII_TERM, 0x20, DMA);
        VDP_loadTileSet(&GFX_BGBLOCKS, 0, DMA);

        VDP_setHorizontalScroll(BG_A, HScroll);
        VDP_setHorizontalScroll(BG_B, HScroll);

        Cursor_CL = 0x0E0;
        
        *plctrl = VDP_WRITE_VRAM_ADDR(0xAC04);
        *pwdata = 0x2;

        if (TermColumns == D_COLUMNS_80) C_XMAX = 126;   // this is a test, remove me if fucky
        else C_XMAX = 62;
    }

    // Update visual cursor position    
    u16 sprx = ((FontSize?(sx << 2):sx << 3)) + HScroll + 128;
    u16 spry = (sy << 3) - VScroll + 128;

    // Clamp position
    sprx = sprx >= 504 ? 504 : sprx;
    spry = spry >= 504 ? 504 : spry;

    // Sprite position
    *plctrl = VDP_WRITE_VRAM_ADDR(0xAC00);
    *pwdata = spry;
    *plctrl = VDP_WRITE_VRAM_ADDR(0xAC06);
    *pwdata = sprx;

    // Sprite size
    *plctrl = VDP_WRITE_VRAM_ADDR(0xAC02);
    *pwdata = 0;

    // Sprite link
    *plctrl = VDP_WRITE_VRAM_ADDR(0xAC03);
    *pwdata = 0;
}

// Send byte to remote machine or buffer it depending on linemode
inline void TTY_SendChar(const u8 c, u8 flags)
{
    if ((vLineMode & LMSM_EDIT) && ((flags & TXF_NOBUFFER) == 0))
    {
        Buffer_Push(&TxBuffer, c);
        return;
    }

    vu8 *PTX = (vu8 *)DEV_UART.TxData;
    vu8 *PSCTRL = (vu8 *)DEV_UART.SCtrl;

    print_charXY_WP(ICO_NET_SEND, STATUS_NET_SEND_POS, CHAR_RED);

    while (*PSCTRL & 1) // while Txd full = 1
    {
        PSCTRL = (vu8 *)DEV_UART.SCtrl;
    }

    *PTX = c;

    TXBytes++;

    print_charXY_WP(ICO_NET_IDLE_SEND, STATUS_NET_SEND_POS, CHAR_WHITE);
}

// Pop and transmit data in TxBuffer
void TTY_TransmitBuffer()
{
    vu8 *PTX = (vu8 *)DEV_UART.TxData;
    vu8 *PSCTRL = (vu8 *)DEV_UART.SCtrl;
    u8 data;

    print_charXY_WP(ICO_NET_SEND, STATUS_NET_SEND_POS, CHAR_RED);

    while (Buffer_Pop(&TxBuffer, &data) != 0xFF)
    {
        while (*PSCTRL & 1) // while Txd full = 1
        {
            PSCTRL = (vu8 *)DEV_UART.SCtrl;
        }

        *PTX = data;

        TXBytes++;
    }

    print_charXY_WP(ICO_NET_IDLE_SEND, STATUS_NET_SEND_POS, CHAR_WHITE);
}

inline void TTY_SendString(const char *str)
{
    u16 len = strlen(str);

    for (u16 c = 0; c < len; c++)
    {
        if (str[c] == '\0') return;

        TTY_SendChar(str[c], TXF_NOBUFFER);
    }
}

inline void TTY_PrintChar(u8 c)
{
    vu32 *plctrl = (u32 *)VDP_CTRL_PORT;
    vu16 *pwdata = (u16 *)VDP_DATA_PORT;
    u16 addr = 0;

    if (FontSize)
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

        *pwdata = 0x2000 + c + (bInverse ? 0x2020 : 0x2120);    // Hmm
        EvenOdd = (sx % 2);
    }
    else
    {
        addr = (((sx & (planeWidth - 1)) + ((sy & (planeHeight - 1)) << planeWidthSft)) * 2);

        *plctrl = VDP_WRITE_VRAM_ADDR(VDP_BG_A + addr);
        *pwdata = c + (bInverse ? 0x2020 : 0x2120);

        *plctrl = VDP_WRITE_VRAM_ADDR(VDP_BG_B + addr);
        *pwdata = 0x4000 + ColorFG;
    }

    TTY_MoveCursor(TTY_CURSOR_RIGHT, 1);
}

inline void TTY_ClearLine(u16 y, u16 line_count)
{
    vu32 *plctrl = (u32 *) VDP_CTRL_PORT;
    vu32 *pldata = (u32 *) VDP_DATA_PORT;
    u16 addr = ((((y & 31) << planeWidthSft)) << 1);
    u16 j;

    VDP_setAutoInc(2);

    //u16 i = line_count; // uncomment this if line_count is actually used
    //while (i--)         // uncomment this if line_count is actually used
    //{                   // uncomment this if line_count is actually used
        *plctrl = VDP_WRITE_VRAM_ADDR((u32) VDP_BG_B + addr);

        j = (TermColumns == D_COLUMNS_40 ? 8 : 16);    // Used to be 10
        while (j--)
        {
            *pldata = 0;
            *pldata = 0;
            *pldata = 0;
            *pldata = 0;
        }
    //} // uncomment this if line_count is actually used

    // Clear BGA too if using 4x8 font
    if (FontSize)
    {
        *plctrl = VDP_WRITE_VRAM_ADDR((u32) VDP_BG_A + addr);

        j = (TermColumns == D_COLUMNS_40 ? 8 : 16);    // Used to be 10
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
    u16 addr = VDP_BG_B + (((from_x & (planeWidth - 1)) + ((y & 31) << planeWidthSft)) << 1);
    u16 j;

    VDP_setAutoInc(2);

    *plctrl = VDP_WRITE_VRAM_ADDR((u32) addr);

    j = (to_x - from_x) >> 3;   // NumChar / 8 --> Below sends 2 bytes * 4 (8 bytes every loop)
    while (j--)
    {
        *pldata = 0;
        *pldata = 0;
        *pldata = 0;
        *pldata = 0;
    }
}

inline void TTY_SetAttribute(u8 v)
{
    // Normal intensity color
    if ((v >= 30) && (v <= 37))
    {
        ColorFG = v - 30;

        if (bIntense) ColorFG += 8;

        #ifndef NO_LOGGING
        kprintf("N ColorFG: $%X", ColorFG);
        #endif
        return;
    }

    if ((v >= 40) && (v <= 47))
    {
        ColorBG = v - 40;

        if (bIntense) ColorBG += 8; // Should this exist?

        #ifndef NO_LOGGING
        kprintf("N ColorBG: $%X", ColorBG);
        #endif
        return;
    }

    // Light intensity color
    if ((v >= 90) && (v <= 97))
    {
        ColorFG = v - 82;

        #ifndef NO_LOGGING
        kprintf("L ColorFG: $%X", ColorFG);
        #endif
        return;
    }

    if ((v >= 100) && (v <= 107))
    {
        ColorBG = v - 92;

        #ifndef NO_LOGGING
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

            #ifndef NO_LOGGING
            kprintf("Text reset");
            #endif
        break;

        case 1:     // Bold / Increased intensity
            if (ColorFG <= 7) ColorFG += 8;

            bIntense = TRUE;

            #ifndef NO_LOGGING
            kprintf("Text increased intensity");
            #endif
        break;

        case 2:     // Decreased intensity
            if (ColorFG >= 8) ColorFG -= 8;

            bIntense = FALSE;

            #ifndef NO_LOGGING
            kprintf("Text decreased intensity");
            #endif
        break;

        case 7:     // Inverse on
            bInverse = TRUE;

            #ifndef NO_LOGGING
            kprintf("Text inverse on");
            #endif
        break;

        case 22:     // Normal intensity
            if (ColorFG >= 8) ColorFG -= 8;

            bIntense = FALSE;

            #ifndef NO_LOGGING
            kprintf("Text Normal intensity");
            #endif
        break;

        case 27:    // Inverse off
            bInverse = FALSE;

            #ifndef NO_LOGGING
            kprintf("Text inverse off");
            #endif
        break;

        case 38:    // Set foreground color
            #ifndef NO_LOGGING
            kprintf("Text set foreground color");
            #endif
        break;

        case 39:    // Reset FG color
            ColorFG = CL_FG;

            #ifndef NO_LOGGING
            kprintf("Text FG color reset");
            #endif
        break;

        case 49:    // Reset BG color
            ColorBG = CL_BG;

            #ifndef NO_LOGGING
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
            if (sx+num > C_XMAX)    // >=
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

    // Sprite position
    *plctrl = VDP_WRITE_VRAM_ADDR(0xAC00);
    *pwdata = spry;
    *plctrl = VDP_WRITE_VRAM_ADDR(0xAC06);
    *pwdata = sprx;
}
