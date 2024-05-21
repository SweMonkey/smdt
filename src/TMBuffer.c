#include "TMBuffer.h"
#include "Terminal.h"
#include "Utils.h"

TMBuffer *TMB_Ptr = NULL;   // Work buffer

#define TMBATTR_BGA (sv_Font?(0x4100 + AVR_FONT0):(0x2100 + AVR_FONT0))
#define TMBATTR_BGB (sv_Font?(0x4100 + AVR_FONT0):(0x4000 + AVR_FONT0))


void TMB_UploadBuffer(TMBuffer *tptr)
{
    if ((tptr == NULL) || (tptr->Updates == 0)) return;
    
    *((vu16*) VDP_CTRL_PORT) = 0x8F02;  // Set VDP autoinc to 2

    // Upload/update dirty BGA tilemap in VRAM
    u16 Updates = tptr->Updates;
    u16 Addr = tptr->LastAddr;

    *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR(AVR_PLANE_A + (Addr<<1));
    while (Updates--)
    {
        *((vu16*) VDP_DATA_PORT) = TMBATTR_BGA + tptr->BufferA[Addr++];

        if (Addr > 0xFFF)
        {
            Addr = 0;
            *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR(AVR_PLANE_A);
        }
    }

    // Upload/update dirty BGB tilemap in VRAM
    Updates = tptr->Updates;
    Addr = tptr->LastAddr;

    *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR(AVR_PLANE_B + (Addr<<1));
    while (Updates--)
    {
        *((vu16*) VDP_DATA_PORT) = TMBATTR_BGB + tptr->BufferB[Addr++];

        if (Addr > 0xFFF)
        {
            Addr = 0;
            *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR(AVR_PLANE_B);
        }
    }

    tptr->Updates = 0;

    // Update vertical scroll
    *((vu32*) VDP_CTRL_PORT) = 0x40000010;
    *((vu16*) VDP_DATA_PORT) = tptr->VScroll;
    *((vu32*) VDP_CTRL_PORT) = 0x40020010;
    *((vu16*) VDP_DATA_PORT) = tptr->VScroll;
}

void TMB_UploadBufferFull(TMBuffer *tptr)
{
    if (tptr == NULL) return;
    
    *((vu16*) VDP_CTRL_PORT) = 0x8F02;  // Set VDP autoinc to 2
    
    // Upload/update full BGA tilemap in VRAM
    u16 Updates = TMB_BUFFER_SIZE;
    u16 Addr = 0;

    *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR(AVR_PLANE_A + (Addr<<1));
    while (Updates--)
    {
        *((vu16*) VDP_DATA_PORT) = TMBATTR_BGA + tptr->BufferA[Addr++];
    }

    // Upload/update full BGB tilemap in VRAM
    Updates = TMB_BUFFER_SIZE;
    Addr = 0;

    *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR(AVR_PLANE_B + (Addr<<1));
    while (Updates--)
    {
        *((vu16*) VDP_DATA_PORT) = TMBATTR_BGB + tptr->BufferB[Addr++];
    }

    tptr->Updates = 0;

    // Update vertical scroll
    *((vu32*) VDP_CTRL_PORT) = 0x40000010;
    *((vu16*) VDP_DATA_PORT) = tptr->VScroll;
    *((vu32*) VDP_CTRL_PORT) = 0x40020010;
    *((vu16*) VDP_DATA_PORT) = tptr->VScroll;
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

    if (sv_Font)   // 4x8 font
    {
        addr = ((TMB_Ptr->sy & 31) << 7) + (TMB_Ptr->sx >> 1);

        switch (TMB_Ptr->sx & 1)
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
    else    // 8x8 font
    {
        addr = ((TMB_Ptr->sy & 31) << 7) + TMB_Ptr->sx;
        TMB_Ptr->BufferA[addr] = c;
        TMB_Ptr->BufferB[addr] = TMB_Ptr->ColorFG;
    }

    if (TMB_Ptr->Updates == 0) TMB_Ptr->LastAddr = addr;    

    TMB_Ptr->Updates++;

    TMB_MoveCursor(TTY_CURSOR_RIGHT, 1);
}

void TMB_PrintString(const char *str)
{
    u16 i = 0;
    while (str[i] != '\0') TMB_PrintChar(str[i++]);
}

void TMB_ClearLine(u16 y, u16 line_count)
{
    if (TMB_Ptr == NULL) return;

    u16 addr = (y & 31) << 7;
    u16 count = line_count << 7;

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
                TMB_Ptr->VScroll &= 0xFF;
            }
        break;

        case TTY_CURSOR_LEFT:
            if (TMB_Ptr->sx-num < 0)
            {
                TMB_SetSX(C_XMAX-(TMB_Ptr->sx-num));
                if (sv_bWrapAround && (TMB_Ptr->sy > 0)) TMB_Ptr->sy--;
            }
            else
            {
                TMB_SetSX(TMB_Ptr->sx-num);
            }
        break;

        case TTY_CURSOR_RIGHT:
            if (TMB_Ptr->sx+num > C_XMAX)
            {
                if (sv_bWrapAround) 
                {
                    TMB_ClearLine(TMB_Ptr->sy+1, 1);
                    TMB_Ptr->sy++;

                    if (TMB_Ptr->sy > (C_YMAX + (TMB_Ptr->VScroll >> 3)))
                    {
                        //TMB_ClearLine(TMB_Ptr->sy, 1);
                        TMB_Ptr->VScroll += 8;
                        TMB_Ptr->VScroll &= 0xFF;
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
