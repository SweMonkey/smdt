#include "SwRenderer.h"
#include "Terminal.h"
#include "Telnet.h"
#include "Utils.h"
#include "system/PseudoFile.h"
#include "../res/system.h"

#define SCREEN_WIDTH  80
#define SCREEN_HEIGHT 32
#define SCREEN_SIZE (SCREEN_WIDTH*SCREEN_HEIGHT)

// VRAM Y address offsets
static const u16 YAddr_Table[] =
{
    0x0000, 0x0500, 0x0A00, 0x0F00, 0x1400, 0x1900, 0x1E00, 0x2300, 
    0x2800, 0x2D00, 0x3200, 0x3700, 0x3C00, 0x4100, 0x4600, 0x4B00, 
    0x5000, 0x5500, 0x5A00, 0x5F00, 0x6400, 0x6900, 0x6E00, 0x7300, 
    0x7800, 0x7D00, 0x8200, 0x8700, 0x8C00, 0x9100, 0x9600, 0x9B00
};

/*static const u16 AttrExpansion[] =
{
    0x0000, 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 
    0x8888, 0x9999, 0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD, 0xEEEE, 0xFFFF
};*/

static const u16 AttrExpansion[] =
{
    0x0000, 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0x7777, 0x8888, 0x9999, 0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD, 0xEEEE, 0xFFFF,
    0x1111, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x2222, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x3333, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x4444, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x5555, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x6666, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x7777, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x8888, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x9999, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0xAAAA, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0xBBBB, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0xCCCC, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0xDDDD, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0xEEEE, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

static u16 tb_16[8];   // Tile render buffer (4x8 pixels)

static u8 *main_scr  = NULL;
static u8 *main_attr = NULL;
static u8 *alt_scr   = NULL;
static u8 *alt_attr  = NULL;

       u8 *scrbuf;  // Screen character buffer (points to either main or alt screen)
static u8 *attrbuf; // Screen attribute buffer (points to either main or alt screen)
static u8 *protbuf; // Screen protection buffer (only one for both main/alt!)

extern u8 ColorFG;
extern u8 ColorBG;
extern bool bIntense;
extern bool bInverse;


void SW_SetBuffer()
{
    scrbuf  = BufferSelect ? alt_scr  : main_scr;
    attrbuf = BufferSelect ? alt_attr : main_attr;
}

void SW_ResetProt()
{
    memset(protbuf, 0, SCREEN_SIZE);
}

static void ClearBuffer(u16 start, u16 size, u8 c)
{
    u8 default_attr = bReverseColour ? ((CL_BG << 4) | CL_FG) : ((CL_FG << 4) | CL_BG);

    for (u16 i = start; i < start+size; i++)
    {
        if (protbuf[i] == 0)
        {
            scrbuf[i] = c;
            attrbuf[i] = default_attr;
        }
    }
}

u8 SW_GetChar(u8 x, u8 y)
{
    u8 ny = y % 80;
    return scrbuf[(ny * SCREEN_WIDTH) + x];
}

void SW_ClearScreen()
{
    ClearBuffer(0, SCREEN_SIZE, 0);

    DMA_doVRamFill(0x2000, 0xA000, 0, 1);
    DMA_waitCompletion();
}

void SW_Free()
{
    if (main_scr != NULL)
    {
        free(main_scr);
        main_scr = NULL;
    }
    if (main_attr != NULL)
    {
        free(main_attr);
        main_attr = NULL;
    }
    if (alt_scr != NULL)
    {
        free(alt_scr);
        alt_scr = NULL;
    }
    if (alt_attr != NULL)
    {
        free(alt_attr);
        alt_attr = NULL;
    }
    if (protbuf != NULL)
    {
        free(protbuf);
        protbuf = NULL;
    }

    TRM_ClearPlane(BG_A);
    TRM_ClearPlane(BG_B);
}

void SW_Setup()
{
    if (main_scr == NULL) main_scr = malloc(SCREEN_SIZE);

    if (main_scr == NULL)
    {
        kprintf("Unable to allocate screen buffer 0\n");
        return;
    }

    if (alt_scr == NULL) alt_scr = malloc(SCREEN_SIZE);

    if (alt_scr == NULL)
    {
        kprintf("Unable to allocate screen buffer 1\n");
        return;
    }

    if (main_attr == NULL) main_attr = malloc(SCREEN_SIZE);

    if (main_attr == NULL)
    {
        kprintf("Unable to allocate attribute buffer 0\n");
        return;
    }

    if (alt_attr == NULL) alt_attr = malloc(SCREEN_SIZE);

    if (alt_attr == NULL)
    {
        kprintf("Unable to allocate attribute buffer 1\n");
        return;
    }

    if (protbuf == NULL) protbuf = malloc(SCREEN_SIZE);

    if (protbuf == NULL)
    {
        kprintf("Unable to allocate protection buffer\n");
        return;
    }

    u8 default_attr = (CL_FG << 4) | CL_BG;
    memset(main_scr,  0, SCREEN_SIZE);
    memset(main_attr, default_attr, SCREEN_SIZE);
    memset(alt_scr,   0, SCREEN_SIZE);
    memset(alt_attr,  default_attr, SCREEN_SIZE);
    
    SW_ResetProt();
    SW_SetBuffer();

    memset(tb_16, 0, 16);

    TRM_ClearPlane(BG_B);
    SW_ClearScreen();

    // Setup tilemap (Plane A and B tilemap is the same at this point)
    u16 i = AVR_FONT0;
    for (u16 y = 0; y < 32; y++)
    {
        for (u16 x = 0; x < 40; x++)
        {
            VDP_setTileMapXY(BG_B, TILE_ATTR_FULL(PAL2, FALSE, FALSE, FALSE, i), x, y);
            i++;
        }
    }
}

static inline __attribute__((always_inline)) void SW_RenderChar(u16 idx)
{
    u16 Char_Off = scrbuf[idx]; Char_Off *= 16;
    u16 Attr     = attrbuf[idx];
    u16 BG_CL    = AttrExpansion[Attr & 0xF];
    u16 FG_CL    = AttrExpansion[Attr & 0xF0]; 
    const u16 *v = (const u16 *)(GFX_ASCII_TERM_SMALL_SW + Char_Off);

    for (u16 it = 0; it < 8; it++, v++)
    {
        tb_16[it] = (BG_CL & ~*v) | (FG_CL & *v);
    }
}

inline __attribute__((always_inline)) void SW_PrintChar(u8 c)
{
    s16 px = TTY_GetSX();                       // Character X position
    s16 py = (TTY_GetSY() + C_YSTART) & 0x1F;   // Character Y position
    u16 p  = (py * SCREEN_WIDTH) + px;          // Character offset

    // Assign BG/FG colour to attribute byte
    u8 attr = (bInverse || bReverseColour ? 
                                            ((ColorBG * 16) | ColorFG) : // Flip BG/FG colours when the bInverse is true
                                            ((ColorFG * 16) | ColorBG) );// Todo: add bIntense flag - force use of upper 8 colours
                                            
    if (!protbuf[p]) scrbuf[p] = c; // If cell is not protected then set cell to character 'c'
    attrbuf[p] = attr;              // Set cell colour attribute
    protbuf[p] = CharProtAttr;      // Set cell protection attribute

    u16 vram_off = AVR_FONT0_POS + (p * 16);    // Even X
    if (p & 1) vram_off -= 14;                  // Odd X    -14 = Will always result in a vram offset of +2 bytes, causing the tile upload to write to the right half of a tile

    SW_RenderChar(p);    // Render glyph into 4x8 tile buffer

    *(vu16*)VDP_CTRL_PORT = 0x8F04;                        // Set VDP autoinc to 4
    *(vu32*)VDP_CTRL_PORT = VDP_WRITE_VRAM_ADDR(vram_off); // Set VDP write address

    u16 *d = (u16*)tb_16;
    for (u16 i = 0; i < 8; i++, d++)
    {
        *(vu16*)VDP_DATA_PORT = *d;
    }
}

void SW_FillScreen(u8 c)
{
    TTY_SetSX(0);
    TTY_SetSY_A(0);

    // Clear RAM screen buffer
    ClearBuffer(0, SCREEN_SIZE, c);

    // Draw character 'c' into 4x8 tile buffer
    SW_RenderChar(0);    // Render glyph into 4x8 tile buffer
    
    // CPU Copy that 4x8 tile into both halves of a 8x8 tile in VRAM
    *(vu16*)VDP_CTRL_PORT = 0x8F02;                                // Set VDP autoinc to 2
    *(vu32*)VDP_CTRL_PORT = VDP_WRITE_VRAM_ADDR(AVR_FONT0_POS);    // Set VDP write address

    u16 *d = (u16*)tb_16;
    for (u16 i = 0; i < 8; i++, d++) 
    {
        *(vu32*)VDP_DATA_PORT = (*d << 16) | (*d);
    }

    // Copy that one 8x8 tile to cover the entire screen
    DMA_doVRamCopy(AVR_FONT0_POS, AVR_FONT0_POS + 32, 0xA000 - 32, 1);
    DMA_waitCompletion();

    TTY_SetSX(0);
    TTY_SetSY_A(0);
}

// This still has a bug; See "test -clearv 6" - it will lop the top and beginning of the text "/sram/" on the next line in the terminal
void SW_ClearLine(u16 y, u16 line_count)
{
    // Are we clearing enough lines to clean the entire screen? If so just call the dedicated function for screen wipe
    if (line_count >= 31)
    {
        SW_ClearScreen();
        return;
    }

    s16 px = 0;
    s16 py = (y) % SCREEN_HEIGHT;
    u16 p = (py * SCREEN_WIDTH) + px;
    s32 num_bytes = (1280 * line_count);    // 1280 = 40 tiles * 32 bytes per tile  -- each tile contain 2 characters, hence 40 and not 80

    // Clear the RAM screen buffer
    ClearBuffer(p, (SCREEN_WIDTH * line_count), 0);

    // Clear the VRAM buffer
    u16 vram_off = YAddr_Table[py]; // VRAM Y offset
    vram_off += AVR_FONT0_POS;      // + VRAM screen buffer offset

    //kprintf("DMA Test: vram_off: $%X - num_bytes: $%X - end: $%X - line_count: %u (HScroll: $C000)", vram_off, num_bytes, (vram_off + num_bytes), line_count);

    // Check if the current DMA will collide with the table after the screen buffer in VRAM
    // If true then we will need to split the DMA into 2; 1 that will only DMA up to the end of the screen buffer
    // and a second DMA that will start at the beginning of the screen buffer (AVR_FONT0_POS).
    // This will do the second DMA:
    if ((vram_off + num_bytes) > AVR_HSCROLL)
    {
        u16 num = (vram_off + num_bytes) - AVR_HSCROLL;
        num_bytes -= num;
        //kprintf("Warning! Attempted to write $%X bytes into HScroll table! Splitting DMA Fill", num);

        DMA_doVRamFill(AVR_FONT0_POS, num, 0, 1);
        DMA_waitCompletion();

        //kprintf("DMA 1: vram_off: $%X - num_bytes: $%X - end: $%X", AVR_FONT0_POS, num, (AVR_FONT0_POS + num));
        //kprintf("DMA 2: vram_off: $%X - num_bytes: $%X - end: $%X", vram_off, num_bytes, (vram_off + num_bytes));
    }
    
    // First and maybe only DMA that will transfer data from line start up until the end of the VRAM screen buffer (AVR_HSCROLL - 1)
    DMA_doVRamFill(vram_off, num_bytes, 0, 1);
    DMA_waitCompletion();
}

void SW_ClearLineSingle(u16 y)
{
    SW_ClearLine(y, 1);
}

void SW_ClearPartialLine(u16 y, u16 from_x, u16 to_x)
{
    if (from_x >= to_x || from_x >= SCREEN_WIDTH) return;

    if (to_x > SCREEN_WIDTH) to_x = SCREEN_WIDTH;

    u16 py = y % SCREEN_HEIGHT;

    // Clear the RAM screen buffer
    u16 p = (py * SCREEN_WIDTH) + from_x;
    u16 count = to_x - from_x;

    //kprintf(" RAM: p: $%X - count: %u - from: %u - to: %u", p, count, from_x, to_x);
    ClearBuffer(p, count, 0);

    if (count > 1)
    {
        // Debug colours, replace VRAM fill value 0 with "colour"
        //u16 a = random() & 0xF;
        //u16 colour = (a << 4) | a;

        u16 vram_off = (AVR_FONT0 + ((p+1)>>1)) * 32;
        count = ((count-1)*16) + (!(from_x & 1) * 16);

        //kprintf("VRAM: vram_off: $%X -> $%X - count: %u - odd: %u", vram_off, vram_off + count, count, from_x & 1);

        DMA_doVRamFill(vram_off, count, 0, 1);
        DMA_waitCompletion();
    }

    // Redraw one character when the count is odd
    if ((from_x & 1))
    {
        u16 vram_off = AVR_FONT0_POS + (p << 4) - 14;    // Odd X

        memset(tb_16, 0, 16);

        *(vu16*)VDP_CTRL_PORT = 0x8F04;                        // Set VDP autoinc to 4
        *(vu32*)VDP_CTRL_PORT = VDP_WRITE_VRAM_ADDR(vram_off); // Set VDP write address
        
        u16 *d = (u16*)tb_16;
        for (u16 i = 0; i < 8; i++, d++)
        {
            *(vu16*)VDP_DATA_PORT = *d;
        }
    }
}

void SW_RedrawScreen()
{
    for (u16 i = 0; i < SCREEN_SIZE; i++)
    {
        u16 vram_off;
        if (i & 1) vram_off = AVR_FONT0_POS + (i << 4) - 14;    // Odd X
        else       vram_off = AVR_FONT0_POS + (i << 4);         // Even X

        SW_RenderChar(i);    // Render glyph into 4x8 tile buffer
        
        *(vu16*)VDP_CTRL_PORT = 0x8F04;                        // Set VDP autoinc to 4
        *(vu32*)VDP_CTRL_PORT = VDP_WRITE_VRAM_ADDR(vram_off); // Set VDP write address
        
        u16 *d = (u16*)tb_16;
        for (u16 i = 0; i < 8; i++, d++)
        {
            *(vu16*)VDP_DATA_PORT = *d;
        }
    }
}

void SW_ShiftLineDown(u8 num)
{
    if (num >= SCREEN_HEIGHT)
    {
        SW_ClearScreen();
        return;
    }

    s16 left  = DMarginLeft-1;
    s16 right = DMarginRight-1;

    // Clamp margins
    if (left < 0) left = 0;
    if (right > SCREEN_WIDTH) right = SCREEN_WIDTH;
    if (left >= right) return;

    u16 col_width = right - left;
    u8 default_attr = (CL_FG << 4) | CL_BG;

    // Move rows bottom up to avoid overwrite
    for (s16 row = SCREEN_HEIGHT - 1; row >= num; row--)
    {
        u16 dst = row * SCREEN_WIDTH + left;
        u16 src = (row - num) * SCREEN_WIDTH + left;

        memmove(scrbuf  + dst, scrbuf  + src, col_width);
        memmove(attrbuf + dst, attrbuf + src, col_width);
    }

    // Clear the top <num> rows inside margins
    for (u8 row = 0; row < num; row++)
    {
        u16 off = row * SCREEN_WIDTH + left;

        memset(scrbuf  + off, 0, col_width);
        memset(attrbuf + off, default_attr, col_width);
    }

    SW_RedrawScreen();
}

void SW_ShiftLineUp(u8 num)
{
    if (num == 0) return;

    // Horizontal margins
    s16 left  = DMarginLeft  - 1;
    s16 right = DMarginRight;

    if (left < 0) left = 0;
    if (right > SCREEN_WIDTH) right = SCREEN_WIDTH;
    if (left >= right) return;

    u16 col_width = right - left;

    // Vertical margins
    s16 top    = DMarginTop;
    s16 bottom = DMarginBottom;

    if (top < 0) top = 0;
    if (bottom >= SCREEN_HEIGHT) bottom = SCREEN_HEIGHT - 1;
    if (top >= bottom) return;

    u16 region_height = bottom - top + 1;

    if (num >= region_height)
        num = region_height;

    u8 default_attr = (CL_FG << 4) | CL_BG;

    // Shift up lines inside region
    //kprintf("Left: %u - Right: %u", left, right);
    //kprintf("Top: %u - Bottom: %u", top, bottom);

    for (s16 row = top; row <= bottom - num; row++)
    {
        u16 dst = row * SCREEN_WIDTH + left;
        u16 src = (row + num) * SCREEN_WIDTH + left;

        memmove(scrbuf  + dst, scrbuf  + src, col_width);
        memmove(attrbuf + dst, attrbuf + src, col_width);
    }

    // Clear bottom lines of region
    for (s16 row = bottom - num + 1; row <= bottom; row++)
    {
        u16 off = row * SCREEN_WIDTH + left;

        memset(scrbuf  + off, 0, col_width);
        memset(attrbuf + off, default_attr, col_width); // default_attr should be whatever the SGR state was left as
    }

    SW_RedrawScreen();
}
