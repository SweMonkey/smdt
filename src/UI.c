
#include "UI.h"
#include "Utils.h" // TRM_drawChar()

// -- Window --
static const u8 Frame[28][40] = 
{
    {201, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 187},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {204, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 202, 205, 205, 205, 187},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {186, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 186},
    {200, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 205, 188},
};

SM_Window *Target = NULL;


void UI_Begin(SM_Window *w)
{
    Target = w;
}

void UI_End()
{
    UI_RepaintWindow();
    Target = NULL;
}

void UI_SetVisible(SM_Window *w, bool v)
{
    w->bVisible = v;
}

void UI_ToggleVisible(SM_Window *w)
{
    w->bVisible = !w->bVisible;
}

bool UI_GetVisible(SM_Window *w)
{
    return w->bVisible;
}

void UI_RepaintWindow()
{
    if (Target == NULL) return;

    if (!(Target->Flags & UC_NOBORDER))
    {
        const char *c = Target->Title;
        u8 x = 1;
        while (*c) Target->WinBuffer[1][x++] = *c++;
    }

    for (u8 y = (Target->Flags & UC_NOBORDER)?1:0; y < 28; y++)
    {
    for (u8 x = 0; x < 40; x++)
    {
        if ((y == 0) && (x > 35)) continue; // Don't clear status icons area
        if (Target->WinBuffer[y][x] == 0) continue;

        TRM_drawChar(Target->WinBuffer[y][x], x, y, PAL1);
    }
    }
}

void UI_SetWindowTitle(const char *title)
{
    if (Target == NULL) return;

    memcpy(Target->Title, title, 34);
    Target->Title[34] = '\0';

    // Redraw title here?
}

void UI_CreateWindow(SM_Window *w, const char *title, u8 flags)
{
    if (w == NULL) return;

    w->Flags = flags;

    if (w->Flags & UC_NOBORDER) memset(w->WinBuffer, 0, 1120);   // prev: filled with 0
    else memcpy(w->WinBuffer, Frame, 1120);

    memcpy(w->Title, title, 34);
    w->Title[34] = '\0';

    if (!(w->Flags & UC_NOBORDER))
    {
        const char *c = w->Title;
        u8 x = 1;
        while (*c) w->WinBuffer[1][x++] = *c++;
    }
}

// -- Clear / Print text --
void UI_DrawText(u8 x, u8 y, const char *text)
{
    if (Target == NULL) return;

    const char *c = text;
    u8 _x = x+1;

    while (*c) Target->WinBuffer[y+3][_x++] = *c++;
}

void UI_ClearRect(u8 x, u8 y, u8 width, u8 height)
{
    if (Target == NULL) return;

    for (u8 _y = 0; _y < height; _y++)
    {
    for (u8 _x = 0; _x < width; _x++)
    {
        Target->WinBuffer[_y+y+3][_x+x+1] = 0x20;
    }
    }
}

void UI_FillRect(u8 x, u8 y, u8 width, u8 height, u8 fillbyte)
{
    if (Target == NULL) return;

    for (u8 _y = 0; _y < height; _y++)
    {
    for (u8 _x = 0; _x < width; _x++)
    {
        Target->WinBuffer[_y+y][_x+x] = fillbyte;
    }
    }
}

// -- VLine / HLine --
void UI_DrawVLine(u8 x, u8 y, u8 height, u8 linechar)
{
    if (Target == NULL) return;

    height = (height > 24 ? 24 : height);

    for (u8 i = 0; i < height; i++)
    {
        if (Target->WinBuffer[y+i+3][x+1] == UC_HLINE_SINGLE)
        {
            if (linechar == UC_VLINE_SINGLE)
            {
                Target->WinBuffer[y+i+3][x+1] = 0xC5;
            }
            else if (linechar == UC_VLINE_DOUBLE)
            {
                Target->WinBuffer[y+i+3][x+1] = 0xD7;
            }
        }
        else if (Target->WinBuffer[y+i+3][x+1] == UC_HLINE_DOUBLE)
        {
            if (linechar == UC_VLINE_SINGLE)
            {
                Target->WinBuffer[y+i+3][x+1] = 0xD8;
            }
            else if (linechar == UC_VLINE_DOUBLE)
            {
                Target->WinBuffer[y+i+3][x+1] = 0xCE;
            }
        }
        else Target->WinBuffer[y+i+3][x+1] = linechar;
    }

    if (y == 0)
    {
        if (linechar == UC_VLINE_SINGLE)
        {
            Target->WinBuffer[2][x+1] = 0xD1;
        }
        else if (linechar == UC_VLINE_DOUBLE)
        {
            Target->WinBuffer[2][x+1] = 0xCB;
        }
    }

    if (y+height >= 24)
    {
        if (linechar == UC_VLINE_SINGLE)
        {
            Target->WinBuffer[27][x+1] = 0xCF;
        }
        else if (linechar == UC_VLINE_DOUBLE)
        {
            Target->WinBuffer[27][x+1] = 0xCA;
        }
    }
}

void UI_DrawHLine(u8 x, u8 y, u8 width, u8 linechar)
{
    if (Target == NULL) return;

    width = (width > 38 ? 38 : width);

    for (u8 i = 0; i < width; i++)
    {
        if (Target->WinBuffer[y+3][x+i+1] == UC_VLINE_SINGLE)
        {
            if (linechar == UC_HLINE_SINGLE)
            {
                Target->WinBuffer[y+3][x+i+1] = 0xC5;
            }
            else if (linechar == UC_HLINE_DOUBLE)
            {
                Target->WinBuffer[y+3][x+i+1] = 0xD8;
            }
        }
        else if (Target->WinBuffer[y+3][x+i+1] == UC_VLINE_DOUBLE)
        {
            if (linechar == UC_HLINE_SINGLE)
            {
                Target->WinBuffer[y+3][x+i+1] = 0xD7;
            }
            else if (linechar == UC_HLINE_DOUBLE)
            {
                Target->WinBuffer[y+3][x+i+1] = 0xCE;
            }
        }
        else Target->WinBuffer[y+3][x+i+1] = linechar;
    }

    if (x == 0)
    {
        if (linechar == UC_HLINE_SINGLE)
        {
            Target->WinBuffer[y+3][0] = 0xC7;
        }
        else if (linechar == UC_HLINE_DOUBLE)
        {
            Target->WinBuffer[y+3][0] = 0xCC;
        }
    }

    if (x+width >= 38)
    {
        if (linechar == UC_HLINE_SINGLE)
        {
            Target->WinBuffer[y+3][39] = 0xB6;
        }
        else if (linechar == UC_HLINE_DOUBLE)
        {
            Target->WinBuffer[y+3][39] = 0xB9;
        }
    }
}

// -- Box / Panel --
void UI_DrawPanel(u8 x, u8 y, u8 width, u8 height, u8 linetype)
{
    if (Target == NULL) return;

    u8 lt_w, lt_h, lt_ul, lt_ur, lt_bl, lt_br;

    if (linetype == UC_PANEL_SINGLE)
    {
        lt_w  = 0xC4;
        lt_h  = 0xB3;
        lt_ul = 0xDA;
        lt_ur = 0xBF;
        lt_bl = 0xC0;
        lt_br = 0xD9;
    }
    else if (linetype == UC_PANEL_DOUBLE)
    {
        lt_w  = 0xCD;
        lt_h  = 0xBA;
        lt_ul = 0xC9;
        lt_ur = 0xBB;
        lt_bl = 0xC8;
        lt_br = 0xBC;
    }
    else
    {
        lt_w  = linetype;
        lt_h  = linetype;
        lt_ul = linetype;
        lt_ur = linetype;
        lt_bl = linetype;
        lt_br = linetype;
    }

    for (u8 i = 0; i < width-2; i++)
    {
        Target->WinBuffer[y+3][x+2+i] = lt_w;
        Target->WinBuffer[y+3+height-1][x+2+i] = lt_w;
    }

    for (u8 i = 0; i < height-2; i++)
    {
        Target->WinBuffer[y+4+i][x+1] = lt_h;
        Target->WinBuffer[y+4+i][x+1+width-1] = lt_h;
    }

    Target->WinBuffer[y+3][x+1] = lt_ul;
    Target->WinBuffer[y+3][x+1+width-1] = lt_ur;
    Target->WinBuffer[y+3+height-1][x+1] = lt_bl;
    Target->WinBuffer[y+3+height-1][x+1+width-1] = lt_br;

    /*for (u8 i = 0; i < width-2; i++)
    {
        if (linetype == UC_PANEL_SINGLE) 
        {
            if (Target->WinBuffer[y+3][x+2+i] == UC_VLINE_SINGLE) Target->WinBuffer[y+3][x+2+i] = 0xC1;
            else if (Target->WinBuffer[y+3][x+2+i] == UC_VLINE_DOUBLE) Target->WinBuffer[y+3][x+2+i] = 0xD0;
            else Target->WinBuffer[y+3][x+2+i] = lt_w;


            if (Target->WinBuffer[y+3+height-1][x+2+i] == UC_VLINE_SINGLE) Target->WinBuffer[y+3+height-1][x+2+i] = 0xC2;
            else if (Target->WinBuffer[y+3+height-1][x+2+i] == UC_VLINE_DOUBLE) Target->WinBuffer[y+3+height-1][x+2+i] = 0xD2;
            else Target->WinBuffer[y+3+height-1][x+2+i] = lt_w;
        }
        else if (linetype == UC_PANEL_DOUBLE) 
        {

        }
        else 
        {
            Target->WinBuffer[y+3][x+2+i] = lt_w;
            Target->WinBuffer[y+3+height-1][x+2+i] = lt_w;
        }
    }

    for (u8 i = 0; i < height-2; i++)
    {
        if (linetype == UC_PANEL_SINGLE) 
        {
            if (Target->WinBuffer[y+4+i][x+1] == UC_HLINE_SINGLE) Target->WinBuffer[y+4+i][x+1] = 0xB4;//
            else if (Target->WinBuffer[y+4+i][x+1] == UC_HLINE_DOUBLE) Target->WinBuffer[y+4+i][x+1] = 0xB5;//
            else if (Target->WinBuffer[y+4+i][x+1] == 0xD2) Target->WinBuffer[y+4+i][x+1] = 0xB4;//
            else if (Target->WinBuffer[y+4+i][x+1] == 0xC1) Target->WinBuffer[y+4+i][x+1] = 0xB4;//
            else Target->WinBuffer[y+4+i][x+1] = lt_h;


            if (Target->WinBuffer[y+4+i][x+1+width-1] == UC_HLINE_SINGLE) Target->WinBuffer[y+4+i][x+1+width-1] = 0xC3;//
            else if (Target->WinBuffer[y+4+i][x+1+width-1] == UC_HLINE_DOUBLE) Target->WinBuffer[y+4+i][x+1+width-1] = 0xC6;
            else Target->WinBuffer[y+4+i][x+1+width-1] = lt_h;
        }
        else if (linetype == UC_PANEL_DOUBLE) 
        {

        }
        else 
        {
            Target->WinBuffer[y+4+i][x+1] = lt_h;
            Target->WinBuffer[y+4+i][x+1+width-1] = lt_h;
        }

        //Target->WinBuffer[y+4+i][x+1] = lt_h;
        //Target->WinBuffer[y+4+i][x+1+width-1] = lt_h;
    }


    / *if (linetype == UC_PANEL_SINGLE) 
    {
        if (Target->WinBuffer[y+3][x+1] == lt_ul) Target->WinBuffer[y+3][x+1] == 
        else if (Target->WinBuffer[y+3][x+1+width-1] == lt_ur)
        else if (Target->WinBuffer[y+3+height-1][x+1] == lt_bl)
        else if (Target->WinBuffer[y+3+height-1][x+1+width-1] == lt_br)
        else
        {

        }
    }* /

    // UL
    switch (Target->WinBuffer[y+3][x+1])
    {
        case UC_HLINE_SINGLE:
            Target->WinBuffer[y+3][x+1] = 0xC2;
        break;
        
        case 0x20:
        default:
            Target->WinBuffer[y+3][x+1] = lt_ul;
        break;
    }

    // UR
    switch (Target->WinBuffer[y+3][x+1+width-1])
    {
        case UC_HLINE_SINGLE:
            Target->WinBuffer[y+3][x+1+width-1] = 0xC2;
        break;
        
        case 0x20:
        default:
            Target->WinBuffer[y+3][x+1+width-1] = lt_ur;
        break;
    }

    // BL
    switch (Target->WinBuffer[y+3+height-1][x+1])
    {
        case UC_HLINE_SINGLE:
            Target->WinBuffer[y+3+height-1][x+1] = 0xC1;
        break;

        case 0xBF:
            Target->WinBuffer[y+3+height-1][x+1] = 0xC5;
        break;

        case UC_VLINE_DOUBLE:
            Target->WinBuffer[y+3+height-1][x+1] = lt_bl;
        break;
        
        case 0x20:
        default:
            Target->WinBuffer[y+3+height-1][x+1] = lt_bl;
        break;
    }

    // BR
    switch (Target->WinBuffer[y+3+height-1][x+1+width-1])
    {
        case UC_HLINE_SINGLE:
            Target->WinBuffer[y+3+height-1][x+1+width-1] = 0xC2;
        break;
        
        case 0x20:
        default:
            Target->WinBuffer[y+3+height-1][x+1+width-1] = lt_br;
        break;
    }*/

    UI_ClearRect(x+1, y+1, width-2, height-2);
}

// -- Scrollbar --
void UI_DrawVScrollbar(u8 x, u8 y, u8 height, u16 min, u16 max, u16 pos)
{
    if (Target == NULL) return;

    fix32 pfd = fix32Div(FIX32(pos), FIX32((max-min)));
    fix32 pfm = fix32Mul(pfd, FIX32(height-y-2));
    u8 pos_ = fix32ToInt(pfm);

    UI_ClearRect(x, y+1, 1, height-2);

    Target->WinBuffer[y+3][x+1] = 0x1E;         // Up arrow
    Target->WinBuffer[y+height+2][x+1] = 0x1F;  // Down arrow
    Target->WinBuffer[y+pos_+4][x+1] = 0xDB;    // Slider
}

// -- ItemList --
void UI_DrawItemList(u8 x, u8 y, u8 width, u8 height, const char *caption, char list[][16], u16 item_count, u16 scroll)
{
    if (Target == NULL) return;

    u8 max_visible = height-4;
    u16 max = (item_count <= max_visible) ? 0 : (item_count-max_visible);
    u16 scroll_ = (scroll < max) ? scroll : max;

    UI_DrawPanel(x, y, width, height, UC_PANEL_SINGLE);
    UI_DrawVScrollbar(x+width-2, y+1, height-2, 0, max, scroll_);
    UI_DrawText(x+1, y, caption);

    char tmp[width-3];

    for (u16 i = 0; i < (item_count < max_visible ? item_count : max_visible); i++)
    {
        strncpy(tmp, list[i+scroll_], width-3);
        UI_DrawText(x+1, y+2+i, tmp);
    }
}
