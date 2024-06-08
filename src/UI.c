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


/// @brief Start window composition
/// @param w Pointer to window to composite in
void UI_Begin(SM_Window *w)
{
    Target = w;
}

/// @brief Stop window composition and repaint it
void UI_End()
{
    UI_RepaintWindow();
    Target = NULL;
}

/// @brief Stop window composition
void UI_EndNoPaint()
{
    Target = NULL;
}

/// @brief Set window visibility
/// @param w Pointer to window
/// @param v Visibility (boolean)
void UI_SetVisible(SM_Window *w, bool v)
{
    w->bVisible = v;
}

/// @brief Toggle window visibility
/// @param w Pointer to window
void UI_ToggleVisible(SM_Window *w)
{
    w->bVisible = !w->bVisible;
}

/// @brief Get window visibility
/// @param w Pointer to window
/// @return TRUE if visible, FALSE if not visible
bool UI_GetVisible(SM_Window *w)
{
    return w->bVisible;
}

/// @brief Repaint window on screen
void UI_RepaintWindow()
{
    if (Target == NULL) return;

    for (u8 y = (Target->Flags & UC_NOBORDER)?1:0; y < 28; y++)
    {
    for (u8 x = 0; x < 40; x++)
    {
        if ((y == 0) && (x > 35)) continue; // Don't clear status icons area
        if (Target->WinBuffer[y][x] == 0) continue;

        TRM_DrawChar(Target->WinBuffer[y][x], x, y, (Target->WinAttribute[y][x] & 0x3));//PAL1);
    }
    }
}

/// @brief Set window title
/// @param title Title text
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

/// @brief Create a new window
/// @param w Empty Window pointer to create
/// @param title Window title
/// @param flags Window flags; UC_NONE = No flags, UC_NOBORDER = Do not draw a window frame
void UI_CreateWindow(SM_Window *w, const char *title, u8 flags)
{
    if (w == NULL) return;

    w->Flags = flags;

    memset(w->WinAttribute, 1, 1120);

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

/// @brief Draw text string
/// @param x X position
/// @param y Y position
/// @param text Text string
void UI_DrawText(u8 x, u8 y, u8 attribute, const char *text)
{
    if (Target == NULL) return;

    const char *c = text;
    u8 _x = x+1;

    while (*c)
    {
        Target->WinAttribute[y+3][_x] = attribute;
        Target->WinBuffer[y+3][_x++] = *c++;
    }
}

/// @brief Clear rectangle
/// @param x X position of rectangle
/// @param y Y position of rectangle
/// @param width Width of rectangle
/// @param height Height of rectangle
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

/// @brief Fill rectangle
/// @param x X position of rectangle
/// @param y Y position of rectangle
/// @param width Width of rectangle
/// @param height Height of rectangle
/// @param fillbyte Byte to fill the rectangle with (Extended ASCII)
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

/// @brief Draw vertical line
/// @param x X position
/// @param y Y position
/// @param height Height of line
/// @param linechar UC_VLINE_SINGLE = Draw single line, UC_VLINE_DOUBLE = Draw double line
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

/// @brief Draw horizontal line
/// @param x X position
/// @param y Y position
/// @param height Width of line
/// @param linechar UC_HLINE_SINGLE = Draw single line, UC_HLINE_DOUBLE = Draw double line
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

/// @brief Draw panel
/// @param x X position
/// @param y Y position
/// @param width Width of panel
/// @param height Height of panel
/// @param linetype UC_PANEL_SINGLE = Draw panel with single lines, UC_PANEL_DOUBLE = Draw panel with double lines
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

/// @brief Draw a simple panel
/// @param x X position
/// @param y Y position
/// @param width Width of panel
/// @param height Height of panel
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
}

/// @brief Draw a vertical scrollbar
/// @param x X position
/// @param y Y position
/// @param height Height of scrollbar
/// @param min Min value of scrollbar
/// @param max Max value of scrollbar
/// @param pos Position of slider / Value of scrollbar
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

/// @brief Draw an item list widget
/// @param x X position
/// @param y Y position
/// @param width Width of item list
/// @param height Height of item list
/// @param caption Item list caption 
/// @param list Array of 16 character strings to be drawn in list
/// @param item_count Number of items (in list)
/// @param scroll Scroll value of item list
void UI_DrawItemList(u8 x, u8 y, u8 width, u8 height, const char *caption, char *list[], u16 item_count, u16 scroll)
{
    if (Target == NULL) return;

    u8 max_visible = height-4;
    u16 max = (item_count <= max_visible) ? 0 : (item_count-max_visible);
    u16 scroll_ = (scroll < max) ? scroll : max;

    UI_DrawPanel(x, y, width, height, UC_PANEL_SINGLE);
    UI_DrawVScrollbar(x+width-2, y+1, height-2, 0, max, scroll_);
    UI_DrawText(x+1, y, PAL1, caption);

    char tmp[width-3];

    for (u16 i = 0; i < (item_count < max_visible ? item_count : max_visible); i++)
    {
        strncpy(tmp, list[i+scroll_], width-3);
        UI_DrawText(x+1, y+2+i, PAL1, tmp);
    }
}

/// @brief Draw text input box
/// @param x X position
/// @param y Y position
/// @param width Width of text input box
/// @param caption Text input box caption
/// @param str String to output/input into
/// @param bShowCaret Show caret in input box
void UI_DrawTextInput(u8 x, u8 y, u8 width, const char *caption, char str[], bool bShowCaret)
{
    if (Target == NULL) return;
    
    UI_DrawPanelSimple(x, y, width, 3);
    UI_DrawText(x+1, y, PAL1, caption);

    UI_DrawText(x+1, y+1, PAL1, str);

    if (bShowCaret) Target->WinBuffer[y+4][x+2+strlen(str)] = 0xBD;
}

/// @brief Draw item list with selector
/// @param x X position
/// @param y Y position
/// @param width Width of item list
/// @param height Height of item list
/// @param caption Item list caption
/// @param list Array of items to draw in list
/// @param item_count Number of items in list
/// @param selected_item Selected item in list
void UI_DrawItemListSelect(u8 x, u8 y, u8 width, u8 height, const char *caption, char *list[], u8 item_count, u8 selected_item)
{
    if (Target == NULL) return;

    u8 min_height = (height < item_count+4) ? item_count+4 : height;//(item_count < height) ? 0 : (item_count-max_visible);
    u8 max_visible = min_height-4;

    UI_DrawPanel(x, y, width, min_height, UC_PANEL_SINGLE);
    UI_DrawText(x+1, y, PAL1, caption);

    char tmp[width-3];

    for (u16 i = 0; i < (item_count < max_visible ? item_count : max_visible); i++)
    {
        strncpy(tmp, list[i], width-3);
        
        if (selected_item == i) UI_DrawText(x+1, y+2+i, PAL3, tmp);
        else UI_DrawText(x+1, y+2+i, PAL1, tmp);
    }
}