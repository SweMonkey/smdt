#include "UI.h"
#include "Utils.h"      // TRM_DrawChar()
#include "Network.h"    // TxBuffer

// -- Window --
static const u8 Frame[28][40] = 
{
    {0xA9, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0x9B},
    {0x9A, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x9A},
    {0xAC, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAA, 0xAD, 0xAD, 0xAD, 0x9B},
    {0x9A, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x9A},
    {0x9A, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x9A},
    {0x9A, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x9A},
    {0x9A, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x9A},
    {0x9A, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x9A},
    {0x9A, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x9A},
    {0x9A, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x9A},
    {0x9A, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x9A},
    {0x9A, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x9A},
    {0x9A, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x9A},
    {0x9A, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x9A},
    {0x9A, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x9A},
    {0x9A, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x9A},
    {0x9A, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x9A},
    {0x9A, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x9A},
    {0x9A, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x9A},
    {0x9A, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x9A},
    {0x9A, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x9A},
    {0x9A, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x9A},
    {0x9A, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x9A},
    {0x9A, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x9A},
    {0x9A, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x9A},
    {0x9A, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x9A},
    {0x9A, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 0x9A},
    {0xA8, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0xAD, 0x9C},
};

static SM_Window *Target = NULL;


void UI_Begin(SM_Window *w)
{
    Target = w;
}

void UI_End()
{
    UI_RepaintWindow();
    Target = NULL;
}

void UI_EndNoPaint()
{
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

    for (u8 y = (Target->Flags & UC_NOBORDER)?1:0; y < 28; y++)
    {
    for (u8 x = 0; x < 40; x++)
    {
        if ((y == 0) && (x > 35)) continue; // Don't clear status icons area
        if (Target->WinBuffer[y][x] == 0) continue;

        TRM_DrawChar(Target->WinBuffer[y][x], x, y, PAL1);
    }
    }
}

void UI_SetWindowTitle(const char *title)
{
    if (Target == NULL) return;
    
    u8 x = 1;

    memcpy(Target->Title, title, 34);
    Target->Title[34] = '\0';

    if (!(Target->Flags & UC_NOBORDER))
    {
        const char *c = Target->Title;
        while (*c) Target->WinBuffer[1][x++] = *c++;
    }
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
                Target->WinBuffer[y+i+3][x+1] = 0xA5;
            }
            else if (linechar == UC_VLINE_DOUBLE)
            {
                Target->WinBuffer[y+i+3][x+1] = 0xB7;
            }
        }
        else if (Target->WinBuffer[y+i+3][x+1] == UC_HLINE_DOUBLE)
        {
            if (linechar == UC_VLINE_SINGLE)
            {
                Target->WinBuffer[y+i+3][x+1] = 0xB8;
            }
            else if (linechar == UC_VLINE_DOUBLE)
            {
                Target->WinBuffer[y+i+3][x+1] = 0xAE;
            }
        }
        else Target->WinBuffer[y+i+3][x+1] = linechar;
    }

    if (y == 0)
    {
        if (linechar == UC_VLINE_SINGLE)
        {
            Target->WinBuffer[2][x+1] = 0xB1;
        }
        else if (linechar == UC_VLINE_DOUBLE)
        {
            Target->WinBuffer[2][x+1] = 0xAB;
        }
    }

    if (y+height >= 24)
    {
        if (linechar == UC_VLINE_SINGLE)
        {
            Target->WinBuffer[27][x+1] = 0xAF;
        }
        else if (linechar == UC_VLINE_DOUBLE)
        {
            Target->WinBuffer[27][x+1] = 0xAA;
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
                Target->WinBuffer[y+3][x+i+1] = 0xA5;
            }
            else if (linechar == UC_HLINE_DOUBLE)
            {
                Target->WinBuffer[y+3][x+i+1] = 0xB8;
            }
        }
        else if (Target->WinBuffer[y+3][x+i+1] == UC_VLINE_DOUBLE)
        {
            if (linechar == UC_HLINE_SINGLE)
            {
                Target->WinBuffer[y+3][x+i+1] = 0xB7;
            }
            else if (linechar == UC_HLINE_DOUBLE)
            {
                Target->WinBuffer[y+3][x+i+1] = 0xAE;
            }
        }
        else Target->WinBuffer[y+3][x+i+1] = linechar;
    }

    if (x == 0)
    {
        if (linechar == UC_HLINE_SINGLE)
        {
            Target->WinBuffer[y+3][0] = 0xA7;
        }
        else if (linechar == UC_HLINE_DOUBLE)
        {
            Target->WinBuffer[y+3][0] = 0xAC;
        }
    }

    if (x+width >= 38)
    {
        if (linechar == UC_HLINE_SINGLE)
        {
            Target->WinBuffer[y+3][39] = 0x96;
        }
        else if (linechar == UC_HLINE_DOUBLE)
        {
            Target->WinBuffer[y+3][39] = 0x99;
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
        lt_w  = 0xA4;
        lt_h  = 0x93;
        lt_ul = 0xBA;
        lt_ur = 0x9F;
        lt_bl = 0xA0;
        lt_br = 0xB9;
    }
    else if (linetype == UC_PANEL_DOUBLE)
    {
        lt_w  = 0xAD;
        lt_h  = 0x9A;
        lt_ul = 0xA9;
        lt_ur = 0x9B;
        lt_bl = 0xA8;
        lt_br = 0x9C;
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

    UI_ClearRect(x+1, y+1, width-2, height-2);
}
void UI_DrawPanelSimple(u8 x, u8 y, u8 width, u8 height)
{
    if (Target == NULL) return;

    for (u8 i = 0; i < width-2; i++)
    {
        Target->WinBuffer[y+3][x+2+i] = 0xA4;
        Target->WinBuffer[y+3+height-1][x+2+i] = 0xA4;
        
        Target->WinBuffer[y+4][x+2+i] = 0x20;  // Clear a single line
    }

    for (u8 i = 0; i < height-2; i++)
    {
        Target->WinBuffer[y+4+i][x+1] = 0x93;
        Target->WinBuffer[y+4+i][x+1+width-1] = 0x93;
    }

    Target->WinBuffer[y+3][x+1] = 0xBA;
    Target->WinBuffer[y+3][x+1+width-1] = 0x9F;
    Target->WinBuffer[y+3+height-1][x+1] = 0xA0;
    Target->WinBuffer[y+3+height-1][x+1+width-1] = 0xB9;

    //UI_ClearRect(x+1, y+1, width-2, height-2);
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
    Target->WinBuffer[y+pos_+4][x+1] = 0xBB;    // Slider
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

// -- Text Input --
void UI_DrawTextInput(u8 x, u8 y, u8 width, const char *caption, char str[], bool bShowCaret)
{
    if (Target == NULL) return;
    
    UI_DrawPanelSimple(x, y, width, 3);
    UI_DrawText(x+1, y, caption);

    UI_DrawText(x+1, y+1, str);

    if (bShowCaret) Target->WinBuffer[y+4][x+2+strlen(str)] = 0xBD;
}
