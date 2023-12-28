
#include "Utils.h"

u8 PrintDelay = 0;
bool bPALSystem;

#define SMDT_CONST_WADDR(x) VDP_WINDOW + ((x & (windowWidth - 1)) * 2)
void print_charXY_W(const char c, u16 x)
{
    vu32 *plctrl = (u32 *)VDP_CTRL_PORT;
    vu16 *pwdata = (u16 *)VDP_DATA_PORT;

    *plctrl = VDP_WRITE_VRAM_ADDR(SMDT_CONST_WADDR(x));
    *pwdata = TILE_ATTR_FULL(PAL1, 0, 0, 0, c + 0x220);
}

inline void print_charXY_WP(const char c, u16 x, u8 palette)
{
    vu32 *plctrl = (u32 *)VDP_CTRL_PORT;
    vu16 *pwdata = (u16 *)VDP_DATA_PORT;

    *plctrl = VDP_WRITE_VRAM_ADDR(SMDT_CONST_WADDR(x));
    *pwdata = TILE_ATTR_FULL(palette, 0, 0, 0, c);  //+0x220
}

void TRM_drawChar(const u8 c, u8 x, u8 y, u8 palette)
{
    vu32 *plctrl = (u32 *)VDP_CTRL_PORT;
    vu16 *pwdata = (u16 *)VDP_DATA_PORT;

    *plctrl = VDP_WRITE_VRAM_ADDR(VDP_WINDOW + (((x & (63)) + ((y & (31)) << 6)) * 2));
    *pwdata = TILE_ATTR_FULL(palette, 1, 0, 0, 0x220 + c);
}

void TRM_drawText(const char *str, u16 x, u16 y, u8 palette)
{
    u16 data[128];
    const u8 *s;
    u16 *d;
    u16 i, pw, ph, len;

    // get the horizontal plane size (in cell)
    pw = windowWidth;
    ph = 32;

    // string outside plane --> exit
    if ((x >= pw) || (y >= ph))
        return;

    // get string len
    len = strlen(str);
    // if string don't fit in plane, we cut it
    if (len > (pw - x))
        len = pw - x;

    // prepare the data
    s = (const u8*) str;
    d = data;
    i = len;
    while(i--)
        *d++ = 0x220 + (*s++);

    // VDP_setTileMapDataRowEx(..) take care of using temporary buffer to build the data so we are ok here
    VDP_setTileMapDataRowEx(WINDOW, data, TILE_ATTR(palette, 1, 0, 0), y, x, len, CPU);
}

void TRM_clearTextArea(u16 x, u16 y, u16 w, u16 h)
{
    u16 data[128];
    u16 i, ya;
    u16 pw, ph;
    u16 wa, ha;

    // get the horizontal plane size (in cell)
    pw = windowWidth;
    ph = 32;

    // string outside plane --> exit
    if ((x >= pw) || (y >= ph))
        return;

    // adjust width
    wa = w;
    // if don't fit in plane, we cut it
    if (wa > (pw - x))
        wa = pw - x;
    // adjust height
    ha = h;
    // if don't fit in plane, we cut it
    if (ha > (ph - y))
        ha = ph - y;

    // prepare the data
    memsetU16(data, 0x220, wa);

    ya = y;
    i = ha;
    while(i--)
        // VDP_setTileMapDataRowEx(..) take care of using temporary buffer to build the data so we are ok here
        VDP_setTileMapDataRowEx(WINDOW, data, TILE_ATTR(PAL1, 1, 0, 0), ya++, x, wa, CPU);
}

inline u8 atoi(char *c)
{
    u8 r = 0;

    for (u8 i = 0; c[i] != '\0'; ++i)
    {
        r = r * 10 + c[i] - '0';
    }

    return r;
}

inline u16 atoi16(char *c)
{
    u16 value = 0;

    while (isdigit(*c)) 
    {
        value *= 10;
        value += (u16) (*c - '0');
        c++;
    }

    return value;
}