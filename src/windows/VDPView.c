#include "VDPView.h"
#include "Input.h"
#include "UI.h"
#include "Network.h"
#include "Utils.h"
#include "WinMgr.h"
#include "Mouse.h"          // MHitRect
#include "system/Sprite.h"
#include "system/PseudoFile.h"

static SM_Window *VDPWindow = NULL;
static s8 sIdx = 0;     // Selector idx between tab + buttons
static u16 tIdx = 0;    // Selector idx for tabs
static s32 SelectedTile = 0;
static s32 SelectedX = 0;
static s32 SelectedY = 0;
static s32 SelectedPAL = PAL0;
static u8 PlaneScrollX = 0, PlaneScrollY = 0;
static u16 r = 255;         // Mouse widget collision target
static u8 LastSpriteCache = 0;
static u8 LastSpriteSizeCache = 0;
static u8 VRAM_ScrollY = 0;

static const char * const tab_text[] =
{
    "VRAM", "PA", "PB", "PW", "Spr", "Reg", "TV", "CRAM"
};

// Register viewer
static s8 sIdxReg = 255;   // Selector idx between tab + buttons
static u16 tIdxReg = 0;    // Selector idx for tabs

static const char * const tab_text_reg[] =
{
    "Mode", "Other", "Raw"
};

static const MRect mrect_data[] =
{
    {304,  32, 8,   8, 0},   // Up
    {304,  40, 8, 168, 1},   // Slider area
    {304, 208, 8,   8, 2},   // Down
    {320,   0, 0,   0, 0},   // Terminator
};

extern u16 LastCursor;   // Last terminal cursor style (font dependent)
extern u8 TTY_GetFont(); // Terminal font in use


static void DrawTileRow(u8 row)
{
    u16 tp = (VRAM_ScrollY + row) * 32;

    for (u8 x = 0; x < 32; x++)
    {
        TRM_SetTile(tp, x + 6, row + 10, SelectedPAL);
        tp++;
    }
}

static void DrawVRAM()
{
    char buf[5];

    UI_DrawVLine(4, 5, 19);
    UI_DrawHLine(0, 5, 39);
    UI_DrawVScrollbar(37, 6, 17, r, 0, 47, VRAM_ScrollY);

    UI_DrawSpinbox(0, 2, 8, "X Pos", &SelectedX, 0);
    UI_DrawSpinbox(8, 2, 8, "Y Pos", &SelectedY, 0);
    UI_DrawSpinbox(16, 2, 8, "Pal", &SelectedPAL, 0);

    for (u8 y = 0; y < 17; y++)
    {
        snprintf(buf, 5, "%04X", (VRAM_ScrollY + y) * 1024);
        UI_DrawText(0, y + 6, PAL1, buf);
    }

    UI_End();

    for (u8 y = 0; y < 17; y++)
    {
        DrawTileRow(y);
    }
}

static void DrawCRAM()
{
    u8 xoff = 6;
    u8 yoff = 12;

    UI_DrawText(6, 7, PAL1, "0 1 2 3 4 5 6 7 8 9 A B C D E F");

    UI_DrawText(0,  9, PAL1, "Pal 0");
    UI_DrawText(0, 11, PAL1, "Pal 1");
    UI_DrawText(0, 13, PAL1, "Pal 2");
    UI_DrawText(0, 15, PAL1, "Pal 3");

    UI_End();
    
    for (u8 y = 0; y < 4; y++)
    {
    for (u8 x = 0; x < 16; x++)
    {
        TRM_SetTile(x, xoff + (x*2) + 0, yoff + (y*2) + 0, y);
        TRM_SetTile(x, xoff + (x*2) + 1, yoff + (y*2) + 0, y);
        TRM_SetTile(x, xoff + (x*2) + 0, yoff + (y*2) + 1, y);
        TRM_SetTile(x, xoff + (x*2) + 1, yoff + (y*2) + 1, y);
    }
    }
}

static void DrawPlane(u16 plane)
{
    u16 tileattr = 0;
    u16 addr = 0;
    u16 base = 0;
    u8 scw = 1;

    switch (plane)
    {
        case 0: base = VDP_getBGAAddress(); scw = 91; break;  // A
        case 1: base = VDP_getBGBAddress(); scw = 91; break;  // B
        case 2: base = AVR_WINDOW;          scw = 27; break;  // W

        default: break;
    }    

    UI_DrawText(1, 2, PAL1, "Tile: 0000");
    UI_DrawText(12, 2, PAL1, "HFlip: 0");
    UI_DrawText(21, 2, PAL1, "VFlip: 0");
    UI_DrawText(30, 2, PAL1, "Pal: 0");
    UI_DrawText(1, 3, PAL1, "Prio: 0");
    UI_DrawText(9, 3, PAL1, "X: 00");
    UI_DrawText(15, 3, PAL1, "Y: 00");
    UI_DrawText(21, 3, PAL1, "Index: 00");

    UI_DrawVScrollbar(37, 5, 17, r, 0, 13, PlaneScrollY);
    UI_DrawHScrollbar(1, 22, 36, r, 0, scw, PlaneScrollX);
    UI_DrawHLine(0, 4, 38);

    UI_End();
    
    for (u8 y = 0; y < 17; y++)
    {
    for (u8 x = 0; x < 37; x++)
    {
        addr = base + (((x + PlaneScrollX) + ((y + PlaneScrollY) << (plane == 2 ? 6 : 7) )) * 2);

        *((vu32*) VDP_CTRL_PORT) = VDP_READ_VRAM_ADDR(addr);
        tileattr = (*((vu16*) VDP_DATA_PORT)) | 0x8000;

        TRM_SetTileAttr(tileattr, x + 1, y + 9);
    }
    }
}

static void DrawTv()
{
    u16 tile = 0;
    u16 addr = 0;
    u8 xoff = 2;
    u8 yoff = 10;

    UI_DrawSpinbox(0, 2, 13, "Index", &SelectedTile, 0);
    UI_DrawSpinbox(14, 2, 8, "Pal", &SelectedPAL, 0);
    UI_DrawGroupBox(xoff-2, yoff-5, 18, 18, "x16 Zoom");
    UI_DrawGroupBox(xoff-2 + 19, yoff-5, 15, 3, "Tile (x1)");

    UI_End();

    addr = SelectedTile * 32;

    TRM_SetTile(SelectedTile, 0 + xoff + 19, yoff, SelectedPAL);

    for (u8 y = 0; y < 16; y+=2)
    {
        *((vu32*) VDP_CTRL_PORT) = VDP_READ_VRAM_ADDR(addr);
        tile = *((vu16*) VDP_DATA_PORT);

        TRM_SetTile((tile & 0xF000) >> 12, 0 + xoff, y + yoff, SelectedPAL);
        TRM_SetTile((tile & 0xF000) >> 12, 1 + xoff, y + yoff, SelectedPAL);
        TRM_SetTile((tile & 0xF000) >> 12, 0 + xoff, y + 1 + yoff, SelectedPAL);
        TRM_SetTile((tile & 0xF000) >> 12, 1 + xoff, y + 1 + yoff, SelectedPAL);

        TRM_SetTile((tile & 0x0F00) >> 8 , 2 + xoff, y + yoff, SelectedPAL);
        TRM_SetTile((tile & 0x0F00) >> 8 , 3 + xoff, y + yoff, SelectedPAL);
        TRM_SetTile((tile & 0x0F00) >> 8 , 2 + xoff, y + 1 + yoff, SelectedPAL);
        TRM_SetTile((tile & 0x0F00) >> 8 , 3 + xoff, y + 1 + yoff, SelectedPAL);

        TRM_SetTile((tile & 0x00F0) >> 4 , 4 + xoff, y + yoff, SelectedPAL);
        TRM_SetTile((tile & 0x00F0) >> 4 , 5 + xoff, y + yoff, SelectedPAL);        
        TRM_SetTile((tile & 0x00F0) >> 4 , 4 + xoff, y + 1 + yoff, SelectedPAL);
        TRM_SetTile((tile & 0x00F0) >> 4 , 5 + xoff, y + 1 + yoff, SelectedPAL);

        TRM_SetTile((tile & 0x000F)      , 6 + xoff, y + yoff, SelectedPAL);
        TRM_SetTile((tile & 0x000F)      , 7 + xoff, y + yoff, SelectedPAL);
        TRM_SetTile((tile & 0x000F)      , 6 + xoff, y + 1 + yoff, SelectedPAL);
        TRM_SetTile((tile & 0x000F)      , 7 + xoff, y + 1+ yoff, SelectedPAL);

        addr += 2;

        *((vu32*) VDP_CTRL_PORT) = VDP_READ_VRAM_ADDR(addr);
        tile = *((vu16*) VDP_DATA_PORT);

        TRM_SetTile((tile & 0xF000) >> 12, 8 + xoff, y + yoff, SelectedPAL);
        TRM_SetTile((tile & 0xF000) >> 12, 9 + xoff, y + yoff, SelectedPAL);
        TRM_SetTile((tile & 0xF000) >> 12, 8 + xoff, y + 1 + yoff, SelectedPAL);
        TRM_SetTile((tile & 0xF000) >> 12, 9 + xoff, y + 1 + yoff, SelectedPAL);

        TRM_SetTile((tile & 0x0F00) >> 8 , 10 + xoff, y + yoff, SelectedPAL);
        TRM_SetTile((tile & 0x0F00) >> 8 , 11 + xoff, y + yoff, SelectedPAL);
        TRM_SetTile((tile & 0x0F00) >> 8 , 10 + xoff, y + 1 + yoff, SelectedPAL);
        TRM_SetTile((tile & 0x0F00) >> 8 , 11 + xoff, y + 1 + yoff, SelectedPAL);

        TRM_SetTile((tile & 0x00F0) >> 4 , 12 + xoff, y + yoff, SelectedPAL);
        TRM_SetTile((tile & 0x00F0) >> 4 , 13 + xoff, y + yoff, SelectedPAL);        
        TRM_SetTile((tile & 0x00F0) >> 4 , 12 + xoff, y + 1 + yoff, SelectedPAL);
        TRM_SetTile((tile & 0x00F0) >> 4 , 13 + xoff, y + 1 + yoff, SelectedPAL);

        TRM_SetTile((tile & 0x000F)      , 14 + xoff, y + yoff, SelectedPAL);
        TRM_SetTile((tile & 0x000F)      , 15 + xoff, y + yoff, SelectedPAL);
        TRM_SetTile((tile & 0x000F)      , 14 + xoff, y + 1 + yoff, SelectedPAL);
        TRM_SetTile((tile & 0x000F)      , 15 + xoff, y + 1+ yoff, SelectedPAL);
        
        addr += 2;
    }
}

static void DrawReg()
{
    char buf[40];
    u8 reg = 0;

    UI_DrawTabs(0, 2, 38, 3, tIdxReg, sIdxReg, tab_text_reg);

    switch (tIdxReg)
    {
        case 0: 
        {
            u8 x = 1;
            u8 y = 4;

            reg = VDP_getReg(0);
            UI_DrawText(x, y++, PAL1, "Mode #1");

            snprintf(buf, 40, " LCB: %u", reg & 0x20 ? 1 : 0);
            UI_DrawText(x, y++, PAL1, buf);

            snprintf(buf, 40, " IE1: %u", reg & 0x10 ? 1 : 0);
            UI_DrawText(x, y++, PAL1, buf);

            snprintf(buf, 40, "  M3: %u", reg & 0x2 ? 1 : 0);
            UI_DrawText(x, y++, PAL1, buf);
            y++;

            reg = VDP_getReg(1);
            UI_DrawText(x, y++, PAL1, "Mode #2");

            snprintf(buf, 40, "DISP: %u", reg & 0x40 ? 1 : 0);
            UI_DrawText(x, y++, PAL1, buf);

            snprintf(buf, 40, " IE0: %u", reg & 0x20 ? 1 : 0);
            UI_DrawText(x, y++, PAL1, buf);

            snprintf(buf, 40, "  M1: %u", reg & 0x10 ? 1 : 0);
            UI_DrawText(x, y++, PAL1, buf);

            snprintf(buf, 40, "  M2: %u", reg & 0x8 ? 1 : 0);
            UI_DrawText(x, y++, PAL1, buf);
            y++;

            reg = VDP_getReg(11);
            UI_DrawText(x, y++, PAL1, "Mode #3");

            snprintf(buf, 40, " IE2: %u", reg & 0x8 ? 1 : 0);
            UI_DrawText(x, y++, PAL1, buf);

            snprintf(buf, 40, "VSCR: %s", reg & 0x4 ? "2 tiles" : "full");
            UI_DrawText(x, y++, PAL1, buf);

            /*snprintf(buf, 40, "HSCR: $%X", reg & 0x3);
            UI_DrawText(x, y++, PAL1, buf);*/

            u8 r = reg & 0x3;
                 if (r == 0) UI_DrawText(x, y++, PAL1, "HSCR: full");
            else if (r == 2) UI_DrawText(x, y++, PAL1, "HSCR: tile");
            else if (r == 3) UI_DrawText(x, y++, PAL1, "HSCR: line");
            else             UI_DrawText(x, y++, PAL1, "HSCR: Unknown");
            
            y = 4;
            x = 20;

            reg = VDP_getReg(12);
            UI_DrawText(x, y++, PAL1, "Mode #4");

            snprintf(buf, 40, "  RS: %s (b%u%u)", ((reg & 0x80 >> 6) + (reg & 1)) ? "H40" : "H32", ((reg & 0x80) >> 6) ? 1 : 0, reg & 1);
            UI_DrawText(x, y++, PAL1, buf);

            snprintf(buf, 40, "S/TE: %u", reg & 0x8 ? 1 : 0);
            UI_DrawText(x, y++, PAL1, buf);

            r = reg & 0x6;
                 if (r == 0) UI_DrawText(x, y++, PAL1, " LSM: none");
            else if (r == 1) UI_DrawText(x, y++, PAL1, " LSM: mode 1");
            else if (r == 6) UI_DrawText(x, y++, PAL1, " LSM: mode 2");

            y++;

            break;
        }

        case 1: 
        {
            u8 x = 1;
            u8 y = 4;

            reg = VDP_getReg(2);
            snprintf(buf, 40, " Plane A: $%X", ((reg & 0x38) >> 3) * 0x2000);
            UI_DrawText(x, y++, PAL1, buf);

            reg = VDP_getReg(3);
            snprintf(buf, 40, " Plane W: $%X", ((reg & 0x3E) >> 1) * 0x800);
            UI_DrawText(x, y++, PAL1, buf);

            reg = VDP_getReg(4);
            snprintf(buf, 40, " Plane B: $%X", (reg & 0x7) * 0x2000);
            UI_DrawText(x, y++, PAL1, buf);

            reg = VDP_getReg(5);
            snprintf(buf, 40, "  Sprite: $%X", (reg & 0x7F) * 0x200);
            UI_DrawText(x, y++, PAL1, buf);

            reg = VDP_getReg(13);
            snprintf(buf, 40, " HScroll: $%X", (reg & 0x3F) * 0x400);
            UI_DrawText(x, y++, PAL1, buf);
            y++;

            reg = VDP_getReg(7);
            snprintf(buf, 40, "BG Colour: Row $%X - Col $%X", (reg & 0x30) >> 4, (reg & 0xF));
            UI_DrawText(x, y++, PAL1, buf);
            y++;

            reg = VDP_getReg(10);
            snprintf(buf, 40, "HIT: %d", reg);
            UI_DrawText(x, y++, PAL1, buf);
            y++;

            break;
        }

        case 2: 
        {
            u8 x = 1;
            u8 y = 4;

            for (u8 r = 0; r < 24; r++)
            {
                if (r > 7 && r < 10) continue;

                snprintf(buf, 40, "%02X: $%X", r, VDP_getReg(r));
                UI_DrawText(x, y++, PAL1, buf);

                if (r == 12)
                {
                    x = 20;
                    y = 4;
                }
            }
            break;
        }

        default: break;
    }

    UI_End();
}

static void UpdateView()
{
    UI_Begin(VDPWindow);
    UI_RedrawWindowFrame();

    //sIdx = tIdx; // Temp

    UI_DrawTabs(0, 0, 38, 8, tIdx, sIdx, tab_text);

    switch (tIdx)
    {
        case 0: DrawVRAM(); break;
        case 1: DrawPlane(0); break;
        case 2: DrawPlane(1); break;
        case 3: DrawPlane(2); break;
        case 4: UI_End(); break;
        case 5: DrawReg(); break;
        case 6: DrawTv(); break;
        case 7: DrawCRAM(); break;
        default: break;
    }

    // UI_End() is / must be called within tab functions!
}

void VDPView_Input()
{
    if (bMouse)
    {
        r = Mouse_GetRect(mrect_data) & 0x7F;

        /*switch (r)
        {
            case 0: // Up
                if (is_KeyDown(sv_MBind_Click))
                {
                    if (ScrollY > 7) 
                    {
                        ScrollY -= 8;
                        UpdateView();
                    }
                }
                else // Hover
                {
                    UI_Begin(VDPWindow);                    
                    UI_DrawVScrollbar(37, 0, 23, r, 0, ScrollMax, ScrollY>>3);
                    UI_RepaintColumn(38, 1);
                    UI_EndNoPaint();
                }
            break;

            case 1: // Slider     
                if (is_KeyDown(sv_MBind_Click))
                {
                    // ...
                }
                else // Hover
                {
                    UI_Begin(VDPWindow);                    
                    UI_DrawVScrollbar(37, 0, 23, r, 0, ScrollMax, ScrollY>>3);
                    UI_RepaintColumn(38, 1);
                    UI_EndNoPaint();
                }       
            break;

            case 2: // Down
                if (is_KeyDown(sv_MBind_Click))
                {
                    if (ScrollY < (bufsize-184))
                    {
                        ScrollY += 8;
                        UpdateView();
                    }
                }
                else // Hover
                {
                    UI_Begin(VDPWindow);                    
                    UI_DrawVScrollbar(37, 0, 23, r, 0, ScrollMax, ScrollY>>3);
                    UI_RepaintColumn(38, 1);
                    UI_EndNoPaint();
                }
            break;
        
            default:
                if (is_KeyDown(sv_MBind_Click) == FALSE)
                {
                    UI_Begin(VDPWindow);                    
                    UI_DrawVScrollbar(37, 0, 23, r, 0, ScrollMax, ScrollY>>3);
                    UI_RepaintColumn(38, 1);
                    UI_EndNoPaint();
                }
            break;
        }*/
    }

    // VRAM viewer
    if (tIdx == 0)
    {
        if (is_KeyDown(KEY_UP))
        {
            if (VRAM_ScrollY > 0) 
            {
                VRAM_ScrollY--;
                
                UI_Begin(VDPWindow);
                DrawVRAM();
            }
        }

        if (is_KeyDown(KEY_DOWN))
        {
            if (VRAM_ScrollY < 47)
            {
                VRAM_ScrollY++;
                
                UI_Begin(VDPWindow);
                DrawVRAM();
            }
        }
    }

    // Plane viewer (all of them)
    if (tIdx > 0 && tIdx < 4)
    {
        u16 max_x = tIdx == 3 ? 27 : 91;

        if (is_KeyDown(KEY_KP4_LEFT))
        {
            if (PlaneScrollX > 0) 
            {
                PlaneScrollX--;
                
                UI_Begin(VDPWindow);
                DrawPlane(tIdx - 1);
            }
        }
        if (is_KeyDown(KEY_KP6_RIGHT))
        {
            if (PlaneScrollX < max_x)
            {
                PlaneScrollX++;
                
                UI_Begin(VDPWindow);
                DrawPlane(tIdx - 1);
            }
        }

        if (is_KeyDown(KEY_KP8_UP))
        {
            if (PlaneScrollY > 0) 
            {
                PlaneScrollY--;
                
                UI_Begin(VDPWindow);
                DrawPlane(tIdx - 1);
            }
        }
        if (is_KeyDown(KEY_KP2_DOWN))
        {
            if (PlaneScrollY < 15)
            {
                PlaneScrollY++;
                
                UI_Begin(VDPWindow);
                DrawPlane(tIdx - 1);
            }
        }
    }
    
    // Main tabs
    {
        // Reg tab & sub tabs selected
        if (tIdx == 5)
        {
            if (is_KeyUp(KEY_UP))
            {
                sIdx = 5;
                sIdxReg = 127;
                
                UpdateView();
            }

            if (is_KeyUp(KEY_DOWN))
            {
                sIdx = 127;
                sIdxReg = tIdxReg = 0;
                
                UpdateView();
            }

            if (sIdx == 127)
            {
                if (is_KeyDown(KEY_LEFT))
                {
                    if (tIdxReg == 0) tIdxReg = 2;
                    else tIdxReg--;

                    sIdxReg = tIdxReg; // Temp
                    
                    UpdateView();
                }
                
                if (is_KeyDown(KEY_RIGHT))
                {
                    if (tIdxReg == 2) tIdxReg = 0;
                    else tIdxReg++;

                    sIdxReg = tIdxReg; // Temp

                    UpdateView();
                }
            }
        }
        
        if (sIdx != 127)
        {
            if (is_KeyDown(KEY_LEFT))
            {
                if (tIdx == 0) tIdx = 7;
                else tIdx--;

                sIdx = tIdx; // Temp

                PlaneScrollX = 0;
                PlaneScrollY = 0;
                
                UpdateView();
            }
            
            if (is_KeyDown(KEY_RIGHT))
            {
                if (tIdx == 7) tIdx = 0;
                else tIdx++;

                sIdx = tIdx; // Temp

                PlaneScrollX = 0;
                PlaneScrollY = 0;

                UpdateView();
            }
        }

    }
    
    // Misc (to be sorted)
    {
        if (is_KeyUp(KEY_KP_ADD))
        {
            if (SelectedPAL < 3) 
            {
                SelectedPAL++;
                UpdateView();
            }
        }
        
        if (is_KeyUp(KEY_KP_SUBTRACT))
        {
            if (SelectedPAL > 0) 
            {
                SelectedPAL--;
                UpdateView();
            }
        }

        if (is_KeyUp(KEY_KP_MULTIPLY))
        {
            if (SelectedTile < 2048) 
            {
                SelectedTile++;
                UpdateView();
            }
        }
        
        if (is_KeyUp(KEY_KP_DIVIDE))
        {
            if (SelectedTile > 0) 
            {
                SelectedTile--;
                UpdateView();
            }
        }

        if (is_KeyUp(KEY_KP9_PGUP))
        {
            if (SelectedTile < 2016) 
            {
                SelectedTile += 32;
                UpdateView();
            }
        }
        
        if (is_KeyUp(KEY_KP8_UP))
        {
            if (SelectedTile > 31) 
            {
                SelectedTile -= 32;
                UpdateView();
            }
        }
    }


    if (is_KeyUp(KEY_ESCAPE) || is_KeyUp(sv_MBind_AltClick))
    {
        WinMgr_Close(W_VDPView);
    }

    return;
}

static void DrawVDPView()
{
    TRM_SetWinHeight(30);

    UI_CreateWindow(VDPWindow, "VDP Debugger - WIP", WF_None);

    UpdateView();
}

u16 VDPView_Open()
{
    // Create hex viewer window
    VDPWindow = malloc(sizeof(SM_Window));

    if (VDPWindow == NULL)
    {
        printf("\e[91mFailed to allocate memory;\nCan't create VDPWindow\e[0m\n");
        return 1;
    }

    u8 ns = 0;
    s16 sy = 192;//176;
    s16 sx = 136;
    u8 StartSprite = LastSprite + 1;

    SetSprite_SIZELINK(LastSprite, LastSpriteSize, StartSprite);

    // Cache the last sprite size (used to restore it back later on)
    LastSpriteSizeCache = LastSpriteSize;
    LastSpriteSize = SPR_WIDTH_2x1 | SPR_HEIGHT_1x3;

    for (u8 y = 0; y < 5; y++)
    {
    for (u8 x = 0; x < 13; x++)
    {
        u8 s = StartSprite + ns;

        SetSprite_Y(s, sy + (y*32));
        SetSprite_TILE(s, 0x2000 + (33));
        SetSprite_X(s, sx + (x*24));

        SetSprite_SIZELINK(s, (x < 12 ? SPR_WIDTH_3x1 : SPR_WIDTH_2x1) | (y < 4 ? SPR_HEIGHT_1x4 : SPR_HEIGHT_1x3), ((ns < 64) ? s+1 : 0));
        //SetSprite_SIZELINK(s, (SPR_WIDTH_3x1) | (y < 5 ? SPR_HEIGHT_1x4 : SPR_SIZE_1x1), s+1);

        ns++;
    }
    }

    // Cache the last sprite index (used to restore it back to first sprite later on)
    LastSpriteCache = LastSprite;
    LastSprite = LastSprite + ns - 1;

    // Hides the terminal cursor - FixMe: Terminal applications can unhide the cursor again, thus making it visible ontop of the "BG" sprites
    SetSprite_TILE(SPRITE_CURSOR, 0x16);
    LastCursor = 0x16;

    // Redraw vdp viewer window and present it
    DrawVDPView();

    return 0;
}

void VDPView_Close()
{
    TRM_SetWinHeight(1);

    // Clear vdp viewer window tiles
    TRM_ClearArea(0, 1, 40, 26 + (bPALSystem?2:0), PAL1, TRM_CLEAR_BG);
    
    // Erase bottom most row tiles (May obscure IRC text input box). 
    // Normally the entire window should be erased by this call, but not all other windows may fill in the erased (black opaque) tiles.
    TRM_ClearArea(0, 27 + (bPALSystem?2:0), 40, 1, PAL1, TRM_CLEAR_INVISIBLE);

    LastSprite = LastSpriteCache;
    LastSpriteSize = LastSpriteSizeCache;
    SetSprite_SIZELINK(LastSprite, LastSpriteSize, 0);

    // Shows the terminal cursor
    if (TTY_GetFont()) LastCursor = 0x13;
    else               LastCursor = 0x10;

    SetSprite_TILE(SPRITE_CURSOR, LastCursor);

    if (VDPWindow != NULL)
    {
        free(VDPWindow);
        VDPWindow = NULL;
        MEM_pack();
    }
}
