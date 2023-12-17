
#include "UI.h"
#include "main.h" // TRM_drawChar()

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
SM_Menu *TargetMenu = NULL;


void UI_Begin(SM_Window *w)
{
    Target = w;
}

void UI_End()
{
    UI_RepaintWindow();
    Target = NULL;
}

void UI_RepaintWindow()
{
    if (Target == NULL) return;

    const char *c = Target->Title;
    u8 x = 1;
    while (*c) Target->WinBuffer[1][x++] = *c++;

    for (u8 y = 0; y < 28; y++)
    {
    for (u8 x = 0; x < 40; x++)
    {
        if ((y == 0) && (x > 35)) continue;

        TRM_drawChar(Target->WinBuffer[y][x], x, y, Target->WinAttr[y][x]);
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

void UI_CreateWindow(SM_Window *w, const char *title)
{
    if (w == NULL) return;

    memcpy(w->WinBuffer, Frame, 1120);
    memset(w->WinAttr, PAL1, 1120);
    memcpy(w->Title, title, 34);
    w->Title[34] = '\0';

    const char *c = w->Title;
    u8 x = 1;
    while (*c) w->WinBuffer[1][x++] = *c++;
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
        Target->WinAttr[_y+y+3][_x+x+1] = PAL1;
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

    //kprintf("pos: %u - pfm: %u", *pos, fix32ToInt(pfm));

    UI_ClearRect(x, y+1, 1, height-2);

    Target->WinBuffer[y+3][x+1] = 0x1E;         // Up arrow
    Target->WinBuffer[y+height+2][x+1] = 0x1F;  // Down arrow
    Target->WinBuffer[y+pos_+4][x+1] = 0xDB;    // Slider
}


// -- Menu test 1/2394932 --
#include "Input.h"

void UI_BeginMenu(SM_Menu *m)
{
    TargetMenu = m;
    TargetMenu->EntryCnt = 0;
    //TargetMenu->SubParent = 255;
    
    if (is_KeyDown(KEY_RETURN))
    {
        //TargetMenu->SubLevel++;
        //TargetMenu->SubParent = TargetMenu->SelectedIdx;
        TargetMenu->Level[TargetMenu->CurLevel] = TargetMenu->SelectedIdx;
    }
    
    if (is_KeyDown(KEY_ESCAPE))
    {
        //if (TargetMenu->SubLevel > 0) TargetMenu->SubLevel--;
        //TargetMenu->Action = TargetMenu->SelectedIdx * -1;
    }
}

void UI_EndMenu()
{
    if (is_KeyDown(KEY_UP))
    {
        if (TargetMenu->SelectedIdx > 0) TargetMenu->SelectedIdx--;
        else TargetMenu->SelectedIdx = TargetMenu->EntryCnt-1;
    }

    if (is_KeyDown(KEY_DOWN))
    {
        if (TargetMenu->SelectedIdx < TargetMenu->EntryCnt-1) TargetMenu->SelectedIdx++;
        else TargetMenu->SelectedIdx = 0;
    }

    TargetMenu = NULL;       
}

void UI_AddMenuEntry(const char *text, VoidCallback *cb)
{
}

bool UI_MenuItem(const char *text, u8 x, u8 y/*, u8 level*/)
{
    if ((Target == NULL)/* || (level != TargetMenu->SubLevel) || ((y != TargetMenu->SubParent) && (TargetMenu->SubParent != 255))*/) return FALSE;

    //TargetMenu->SubParent = y;

    //bool r = FALSE;
    const char *c = text;
    u8 _x = x+1;


    if (TargetMenu->Level[TargetMenu->EntryCnt] != TargetMenu->CurLevel)
    {
        //TargetMenu->Action = 0;
        return 0;   
    }

    if (TargetMenu->SelectedIdx == TargetMenu->EntryCnt)
    {
        while (*c)
        {
            Target->WinBuffer[y+3][_x] = *c++;
            Target->WinAttr[y+3][_x++] = PAL3;
        }

        //r = TRUE;
    }
    else
    {
        while (*c) Target->WinBuffer[y+3][_x++] = *c++;

        //r = FALSE;
    }

    TargetMenu->EntryCnt++;

    return 1;//r;
}

void UI_MenuSelect(SM_Menu *m, s8 idx)
{
    /*if (idx >= m->EntryCnt)
    {
        idx = 0;
    }
    else if (idx < 0)
    {
        idx = m->EntryCnt-1;
    }*/

    m->SelectedIdx = idx;

    kprintf("SelectedIdx: %u - Cnt: %u", m->SelectedIdx, m->EntryCnt);
}

void UI_MenuEnter()
{
}