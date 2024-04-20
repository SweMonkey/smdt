#include "TMBuffer.h"
#include "Terminal.h"

TMBuffer *TMB_Ptr = NULL;   // Work buffer

#define TMBATTR_BGA (FontSize?0x4140:0x2140)
#define TMBATTR_BGB (FontSize?0x4140:0x4000)


void TMB_UploadBuffer(TMBuffer *tptr)
{
    if (tptr == NULL) return;

    if (tptr->Updates == 0) return;

    vu32 *plctrl = (u32 *)VDP_CTRL_PORT;
    vu16 *pwdata = (u16 *)VDP_DATA_PORT;
    
    VDP_setAutoInc(2);
    
    u16 Updates = tptr->Updates;
    u16 Addr = tptr->LastAddr;

    *plctrl = VDP_WRITE_VRAM_ADDR(0xC000 + (Addr<<1));
    while (Updates--)
    {
        *pwdata = TMBATTR_BGA + tptr->BufferA[Addr++];

        if (Addr > 0xFFF)
        {
            Addr = 0;
            *plctrl = VDP_WRITE_VRAM_ADDR(0xC000);
        }
    }

    Updates = tptr->Updates;
    Addr = tptr->LastAddr;

    *plctrl = VDP_WRITE_VRAM_ADDR(0xE000 + (Addr<<1));
    while (Updates--)
    {
        *pwdata = TMBATTR_BGB + tptr->BufferB[Addr++];

        if (Addr > 0xFFF)
        {
            Addr = 0;
            *plctrl = VDP_WRITE_VRAM_ADDR(0xE000);
        }
    }

    tptr->Updates = 0;

    // Update vertical scroll
    *plctrl = VDP_WRITE_VSRAM_ADDR((u32) 0);    // 0x40000010;
    *pwdata = tptr->VScroll;
    *plctrl = VDP_WRITE_VSRAM_ADDR((u32) 2);    // 0x40020010;
    *pwdata = tptr->VScroll;
}

void TMB_UploadBufferFull(TMBuffer *tptr)
{
    if (tptr == NULL) return;

    vu32 *plctrl = (u32 *)VDP_CTRL_PORT;
    vu16 *pwdata = (u16 *)VDP_DATA_PORT;
    
    VDP_setAutoInc(2);
    
    u16 Updates = TMB_BUFFER_SIZE;
    u16 Addr = 0;

    *plctrl = VDP_WRITE_VRAM_ADDR(0xC000 + (Addr<<1));
    while (Updates--)
    {
        *pwdata = TMBATTR_BGA + tptr->BufferA[Addr++];
    }

    Updates = TMB_BUFFER_SIZE;
    Addr = 0;

    *plctrl = VDP_WRITE_VRAM_ADDR(0xE000 + (Addr<<1));
    while (Updates--)
    {
        *pwdata = TMBATTR_BGB + tptr->BufferB[Addr++];
    }

    tptr->Updates = 0;

    // Update vertical scroll
    *plctrl = VDP_WRITE_VSRAM_ADDR((u32) 0);    // 0x40000010;
    *pwdata = tptr->VScroll;
    *plctrl = VDP_WRITE_VSRAM_ADDR((u32) 2);    // 0x40020010;
    *pwdata = tptr->VScroll;
}

u8 TMB_SetActiveBuffer(TMBuffer *b)
{
    if (b == NULL) return 0;
    
    TMB_Ptr = b;
    return 1;
}

void TMB_PrintChar(u8 c)
{
    if (TMB_Ptr == NULL) return;

    u16 addr = 0;

    if (FontSize)
    {
        addr = ((TMB_Ptr->sy%32) *128) + (TMB_Ptr->sx/2);

        switch (TMB_Ptr->sx % 2)
        {
            case 0: // Plane B
            {
                TMB_Ptr->BufferB[addr] = c;
                break;
            }

            case 1: // Plane A
            {
                TMB_Ptr->BufferA[addr] = c;
                break;
            }
            
            default:
            break;
        }
    }
    else
    {
        addr = ((TMB_Ptr->sy%32) *128) + TMB_Ptr->sx;
        TMB_Ptr->BufferA[addr] = c;
        TMB_Ptr->BufferB[addr] = TMB_Ptr->ColorFG;
    }

    if (TMB_Ptr->Updates == 0) TMB_Ptr->LastAddr = addr;    

    TMB_Ptr->Updates++;

    TMB_MoveCursor(TTY_CURSOR_RIGHT, 1);
}

void TMB_ClearLine(u16 y, u16 line_count)
{
    if (TMB_Ptr == NULL) return;

    u16 addr = (y%32) * 128;
    u16 count = line_count * 128;

    memset(TMB_Ptr->BufferA+addr, 0, count);
    memset(TMB_Ptr->BufferB+addr, 0, count);

    TMB_Ptr->Updates += 128;
}

void TMB_SetColorFG(u8 v)
{
    if (TMB_Ptr == NULL) return;

    TMB_Ptr->ColorFG = v & 0xF;
}

void TMB_MoveCursor(u8 dir, u8 num)
{
    if (TMB_Ptr == NULL) return;

    switch (dir)
    {
        case TTY_CURSOR_UP:
            if (TMB_Ptr->sy-num <= 0) TMB_Ptr->sy = 0;
            else TMB_Ptr->sy -= num;
        break;

        case TTY_CURSOR_DOWN:
            TMB_ClearLine(TMB_Ptr->sy+1, num);
            TMB_Ptr->sy += num;
            
            if (TMB_Ptr->sy > (C_YMAX + (TMB_Ptr->VScroll >> 3)))
            {
                TMB_Ptr->VScroll += 8 * num;
                TMB_Ptr->VScroll %= 256;
            }
        break;

        case TTY_CURSOR_LEFT:
            if (TMB_Ptr->sx-num < 0)
            {
                TMB_SetSX(C_XMAX-(TMB_Ptr->sx-num));
                if (bWrapAround && (TMB_Ptr->sy > 0)) TMB_Ptr->sy--;
            }
            else
            {
                TMB_SetSX(TMB_Ptr->sx-num);
            }
        break;

        case TTY_CURSOR_RIGHT:
            if (TMB_Ptr->sx+num > C_XMAX)
            {
                if (bWrapAround) 
                {
                    TMB_ClearLine(TMB_Ptr->sy+1, 1);
                    TMB_Ptr->sy++;

                    if (TMB_Ptr->sy > (C_YMAX + (TMB_Ptr->VScroll >> 3)))
                    {
                        //TMB_ClearLine(TMB_Ptr->sy, 1);
                        TMB_Ptr->VScroll += 8;
                        TMB_Ptr->VScroll %= 256;
                    }
                }
                TMB_SetSX((TMB_Ptr->sx+num)-C_XMAX-2);
            }
            else
            {
                TMB_SetSX(TMB_Ptr->sx+num);
            }
        break;

        default:
        break;
    }
}

inline void TMB_SetSX(s32 x)
{
    if (TMB_Ptr == NULL) return;

    TMB_Ptr->sx = x<0?0:x;                                  // sx less than 0? set to 0
    TMB_Ptr->sx = TMB_Ptr->sx>C_XMAX?C_XMAX:TMB_Ptr->sx;    // sx greater than max_x? set to max_x
}

inline s32 TMB_GetSX()
{
    if (TMB_Ptr == NULL) return 0;
    
    return TMB_Ptr->sx;
}

inline void TMB_SetSY_A(s32 y)
{
    if (TMB_Ptr == NULL) return;
    
    TMB_Ptr->sy = y<0?0:y;                                  // sy less than 0? set to 0
    TMB_Ptr->sy = TMB_Ptr->sy>C_YMAX?C_YMAX:TMB_Ptr->sy;    // sy greater than max_y? set to max_y
    TMB_Ptr->sy += ((TMB_Ptr->VScroll >> 3) + C_YSTART);
}

inline s32 TMB_GetSY_A()
{
    if (TMB_Ptr == NULL) return 0;
    
    return TMB_Ptr->sy - ((TMB_Ptr->VScroll >> 3) + C_YSTART);
}

inline void TMB_SetSY(s32 y)
{
    if (TMB_Ptr == NULL) return;
    
    TMB_Ptr->sy = y<0?0:y;
}

inline s32 TMB_GetSY()
{
    if (TMB_Ptr == NULL) return 0;
    
    return TMB_Ptr->sy;
}

void TMB_SetVScroll(s16 v)
{
    if (TMB_Ptr == NULL) return;

    TMB_Ptr->VScroll = v;
}

void TMB_SetHScroll(s16 h)
{
    if (TMB_Ptr == NULL) return;

    TMB_Ptr->HScroll = h;
}

void TMB_ClearBuffer()
{
    if (TMB_Ptr == NULL) return;

    memset(TMB_Ptr->BufferA, 0, TMB_BUFFER_SIZE);
    memset(TMB_Ptr->BufferB, 0, TMB_BUFFER_SIZE);
}
