
#include "main.h"
#include "StateCtrl.h"
#include "Input.h"
#include "Terminal.h"
#include "Keyboard_PS2.h"
#include "../res/system.h"

u8 PrintDelay = 0;
bool bPALSystem;


int main(bool hardReset)
{
    SYS_disableInts();

    Z80_unloadDriver();
    Z80_requestBus(TRUE);
    //Z80_getAndRequestBus(TRUE);

    VDP_setHScrollTableAddress(0xA000); // 0xA000 - 0xA3FF
    VDP_setSpriteListAddress(0xAC00);   // 0xAC00 - 0xAFFF
    VDP_setWindowAddress(0x8000);   // 0xB000 - 0xBFFF
    VDP_setBGAAddress(0xC000);    // 0xC000 - 0xDFFF
    VDP_setBGBAddress(0xE000);    // 0xE000 - 0xFFFF

    bPALSystem = IS_PAL_SYSTEM;

    if (bPALSystem)
    {
        VDP_setScreenHeight240();
    }

    PAL_setPalette(PAL0, palette_black, DMA);
    PAL_setPalette(PAL1, palette_black, DMA);
    PAL_setPalette(PAL2, palette_black, DMA);
    PAL_setPalette(PAL3, palette_black, DMA);
    
    PAL_setColor( 1, 0x00e);    // Icon Red
    PAL_setColor( 2, 0x0e0);    // Icon Green
    PAL_setColor(14, 0x000);    // Icon BG
    PAL_setColor(15, 0xeee);    // Icon Normal
    PAL_setColor(17, 0x000);    // Window text BG Normal / Terminal text BG
    PAL_setColor(18, 0xeee);    // Window text FG Normal
    PAL_setColor(49, 0xeee);    // Window text BG Inverted / Terminal text FG? Was used for something there...
    PAL_setColor(50, 0x000);    // Window text FG Inverted

    VDP_loadTileSet(&GFX_ASCII_MENU, 0x220, DMA);
    VDP_loadTileSet(&GFX_ICONS, 0x18, DMA);

    Input_Init();

    VDP_setWindowVPos(FALSE, 1);
    TRM_clearTextArea(0, 0, 40, 10);
    TRM_drawText(STATUS_TEXT, 1, 0, PAL1);

    print_charXY_WP(ICO_KB_UNKNOWN, STATUS_KB_POS, CHAR_WHITE);
    print_charXY_WP(ICO_NET_IDLE_RECV, STATUS_NET_RECV_POS, CHAR_WHITE);
    print_charXY_WP(ICO_NET_IDLE_SEND, STATUS_NET_SEND_POS, CHAR_WHITE);

    SYS_enableInts();

    SYS_doVBlankProcess();

    //ChangeState(PS_Debug, 0, NULL);
    //ChangeState(PS_Telnet, 0, NULL);
    ChangeState(PS_Entry, 0, NULL);

    while(TRUE)
    {
        StateTick();
        SYS_doVBlankProcess();
    }

    return 0;
}

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
    *pwdata = TILE_ATTR_FULL(palette, 0, 0, 0, 0x220 + c);
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
    VDP_setTileMapDataRowEx(WINDOW, data, TILE_ATTR(palette, TRUE, 0, 0), y, x, len, CPU);
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
        VDP_setTileMapDataRowEx(WINDOW, data, TILE_ATTR(PAL1, TRUE, 0, 0), ya++, x, wa, CPU);
}