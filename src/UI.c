#include "UI.h"
#include "Utils.h"      // TRM_DrawChar()
#include "Network.h"    // TxBuffer

// -- Window --
static const u8 Frame[30][40] = 
{
    {0xC0, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC1, 0xC2},
    {0xC3, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC5},
    {0xC6, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC7, 0xC8},
    {0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCB},
    {0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCB},
    {0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCB},
    {0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCB},
    {0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCB},
    {0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCB},
    {0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCB},
    {0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCB},
    {0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCB},
    {0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCB},
    {0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCB},
    {0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCB},
    {0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCB},
    {0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCB},
    {0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCB},
    {0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCB},
    {0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCB},
    {0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCB},
    {0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCB},
    {0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCB},
    {0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCB},
    {0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCB},
    {0xC9, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCB},
    {0xCC, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCD, 0xCE},

    {0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF},
    {0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF},
    {0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF, 0xDF},
};

static SM_Window *Target = NULL;
u8 sv_ThemeUI = 0;
bool bModalWindowActive = FALSE;


/// @brief Apply UI theme
void UI_ApplyTheme()
{
    switch (sv_ThemeUI)
    {
        case 0: // Dark blue
            SetColor( 2, 0xEEE);    // Window title FG (+Selected text colour)
            SetColor( 3, 0xA40);    // Window title background
            SetColor( 5, 0x222);    // Icon background

            SetColor(19, 0x222);    // Window inner BG
            SetColor(20, 0xA40);    // Window title BG

            SetColor(21, 0x820);    // Window shadow
            SetColor(22, 0xE80);    // Window border
            SetColor(49, 0xE80);    // Cursor outline
        break;
        case 1: // Dark lime
            SetColor( 2, 0x000);    // Window title FG (+Selected text colour)
            SetColor( 3, 0x084);    // Window title background
            SetColor( 5, 0x222);    // Icon background
            
            SetColor(19, 0x222);    // Window inner BG
            SetColor(20, 0x084);    // Window title BG

            SetColor(21, 0x062);    // Window shadow
            SetColor(22, 0x0C8);    // Window border
            SetColor(49, 0x0C8);    // Cursor outline
        break;
        case 2: // Dark amber
            SetColor( 2, 0xEEE);    // Window title FG (+Selected text colour)
            SetColor( 3, 0x048);    // Window title background
            SetColor( 5, 0x222);    // Icon background
            
            SetColor(19, 0x222);    // Window inner BG
            SetColor(20, 0x048);    // Window title BG

            SetColor(21, 0x028);    // Window shadow
            SetColor(22, 0x08E);    // Window border
            SetColor(49, 0x08E);    // Cursor outline
        break;
        case 3: // High contrast
            SetColor( 2, 0xEEE);    // Window title FG (+Selected text colour)
            SetColor( 3, 0x444);    // Window title background
            SetColor( 5, 0x000);    // Icon background
            
            SetColor(19, 0x000);    // Window inner BG
            SetColor(20, 0x444);    // Window title BG

            SetColor(21, 0x000);    // Window shadow
            SetColor(22, 0xEEE);    // Window border
            SetColor(49, 0xEEE);    // Cursor outline
        break;
        case 4: // Aqua
            SetColor( 2, 0xEEE);    // Window title FG (+Selected text colour)
            SetColor( 3, 0x664);    // Window title background
            SetColor( 5, 0x442);    // Icon background

            SetColor(19, 0x442);    // Window inner BG
            SetColor(20, 0x664);    // Window title BG

            SetColor(21, 0x660);    // Window shadow
            SetColor(22, 0xCC0);    // Window border
            SetColor(49, 0x880);    // Cursor outline
        break;
        case 5: // Hot pink
            SetColor( 2, 0xEEE);    // Window title FG (+Selected text colour)
            SetColor( 3, 0x646);    // Window title background
            SetColor( 5, 0x424);    // Icon background

            SetColor(19, 0x424);    // Window inner BG
            SetColor(20, 0x646);    // Window title BG

            SetColor(21, 0x606);    // Window shadow
            SetColor(22, 0xC0C);    // Window border
            SetColor(49, 0xC0C);    // Cursor outline
        break;
    
        default:
        break;
    }
}

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

    for (u8 y = 0; y < 30; y++)
    {
    for (u8 x = 0; x < 40; x++)
    {
        if (Target->WinBuffer[y][x] == 0) continue;

        TRM_DrawChar(Target->WinBuffer[y][x], x, y+1 - ((Target->Flags & WF_NoBorder) ? 3 : 0), (Target->WinAttribute[y][x] & 0x3));
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

    if (!(Target->Flags & WF_NoBorder))
    {
        const char *c = Target->Title;
        while (*c) 
        {
            Target->WinAttribute[1][x] = PAL0;
            Target->WinBuffer[1][x++] = *c++;
        }
    }
}

/// @brief Create a new window
/// @param w Empty Window pointer to create
/// @param title Window title
/// @param flags Window flags; UC_NONE = No flags, UC_NOBORDER = Do not draw a window frame
void UI_CreateWindow(SM_Window *w, const char *title, UI_WindowFlags flags)
{
    if (w == NULL) return;

    w->Flags = flags;

    memset(w->WinAttribute, PAL1, 1200);

    if (w->Flags & WF_NoBorder) memset(w->WinBuffer, 0, 1200);   // prev: filled with 0
    else memcpy(w->WinBuffer, Frame, 1200);

    memcpy(w->Title, title, 34);
    w->Title[34] = '\0';

    if (!(w->Flags & WF_NoBorder))
    {
        const char *c = w->Title;
        u8 x = 1;
        while (*c) 
        {
            w->WinAttribute[1][x] = PAL0;
            w->WinBuffer[1][x++] = *c++;
        }
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
        Target->WinAttribute[_y+y+3][_x+x+1] = PAL1;
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
void UI_DrawVLine(u8 x, u8 y, u8 height)
{
    if (Target == NULL) return;

    height = (height > 23 ? 23 : height);

    for (u8 i = 0; i < height; i++)
    {
        if (Target->WinBuffer[y+i+3][x+1] == 0xD0)
        {
            Target->WinBuffer[y+i+3][x+1] = 0xD9;
        }
        else Target->WinBuffer[y+i+3][x+1] = 0xCF;
    }

    if (y == 0)
    {
        Target->WinBuffer[2][x+1] = 0xDC;
    }

    if (y+height >= 23)
    {
        Target->WinBuffer[26][x+1] = 0xDD;
    }
}

/// @brief Draw horizontal line
/// @param x X position
/// @param y Y position
/// @param height Width of line
void UI_DrawHLine(u8 x, u8 y, u8 width)
{
    if (Target == NULL) return;

    width = (width > 38 ? 38 : width);

    for (u8 i = 0; i < width; i++)
    {
        if (Target->WinBuffer[y+3][x+i+1] == 0xCF)
        {
            Target->WinBuffer[y+3][x+i+1] = 0xD9;
        }
        else Target->WinBuffer[y+3][x+i+1] = 0xD0;
    }

    if (x == 0)
    {
        Target->WinBuffer[y+3][0] = 0xDA;
    }

    if (x+width >= 38)
    {
        Target->WinBuffer[y+3][39] = 0xDB;
    }
}

/// @brief Draw panel
/// @param x X position
/// @param y Y position
/// @param width Width of panel
/// @param height Height of panel
void UI_DrawPanel(u8 x, u8 y, u8 width, u8 height)
{
    if (Target == NULL) return;

    for (u8 i = 0; i < width-2; i++)
    {
        Target->WinBuffer[y+3][x+2+i] = 0xD0;
        Target->WinBuffer[y+3+height-1][x+2+i] = 0xD0;
    }

    for (u8 i = 0; i < height-2; i++)
    {
        Target->WinBuffer[y+4+i][x+1] = 0xCF;
        Target->WinBuffer[y+4+i][x+1+width-1] = 0xCF;
    }

    Target->WinBuffer[y+3][x+1] = 0xD3;
    Target->WinBuffer[y+3][x+1+width-1] = 0xD4;
    Target->WinBuffer[y+3+height-1][x+1] = 0xD1;
    Target->WinBuffer[y+3+height-1][x+1+width-1] = 0xD2;

    UI_ClearRect(x+1, y+1, width-2, height-2);
}

/// @brief Draw groupbox
/// @param x X position
/// @param y Y position
/// @param width Width of groupbox
/// @param height Height of groupbox
/// @param caption Groupbox caption
void UI_DrawGroupBox(u8 x, u8 y, u8 width, u8 height, const char *caption)
{
    if (Target == NULL) return;

    for (u8 i = 0; i < width-2; i++)
    {
        Target->WinBuffer[y+3][x+2+i] = 0xD0;
        Target->WinBuffer[y+3+height-1][x+2+i] = 0xD0;
    }

    for (u8 i = 0; i < height-2; i++)
    {
        Target->WinBuffer[y+4+i][x+1] = 0xCF;
        Target->WinBuffer[y+4+i][x+1+width-1] = 0xCF;
    }

    Target->WinBuffer[y+3][x+1] = 0xD3;
    Target->WinBuffer[y+3][x+1+width-1] = 0xD4;
    Target->WinBuffer[y+3+height-1][x+1] = 0xD1;
    Target->WinBuffer[y+3+height-1][x+1+width-1] = 0xD2;

    UI_DrawText(x+1, y, PAL1, caption);

    UI_ClearRect(x+1, y+1, width-2, height-2);
}

/// @brief Draw floating window
/// @param x X position
/// @param y Y position
/// @param width Width of window
/// @param height Height of window
/// @param bChild Draw window inside another window
/// @param title Window title
void UI_DrawWindow(u8 x, u8 y, u8 width, u8 height, bool bChild, const char *title)
{
    if (Target == NULL) return;

    u8 m = (bChild ? 64 : 0);

    // Reset tiles and attributes manually for the entire rect covered by the window (using UI_ClearRect won't work since attributes will be offset)
    for (u8 ay = 0; ay < height; ay++)
    {
    for (u8 ax = 0; ax < width; ax++)
    {
        Target->WinBuffer[y+ay+3][x+ax+1] = 0x20;   // 0xCA ?
        Target->WinAttribute[y+ay+3][x+ax+1] = PAL1;
    }
    }

    // Horizontal lines
    for (u8 i = 0; i < width-2; i++)
    {
        Target->WinBuffer[y+3][x+2+i] = 0xC1 - m;
        Target->WinBuffer[y+4][x+2+i] = 0xC4 - m;
        Target->WinBuffer[y+5][x+2+i] = 0xC7 - m;

        Target->WinBuffer[y+3+height-1][x+2+i] = 0xCD - m;
    }

    Target->WinBuffer[y+4][x+1] = 0xC3 - m;
    Target->WinBuffer[y+4][x+1+width-1] = 0xC5 - m;

    Target->WinBuffer[y+5][x+1] = 0xC6 - m;
    Target->WinBuffer[y+5][x+1+width-1] = 0xC8 - m;

    // Vertical lines
    for (u8 i = 0; i < height-4; i++)
    {
        Target->WinBuffer[y+6+i][x+1] = 0xC9 - m;
        Target->WinBuffer[y+6+i][x+1+width-1] = 0xCB - m;
    }

    Target->WinBuffer[y+3][x+1] = 0xC0 - m;
    Target->WinBuffer[y+3][x+1+width-1] = 0xC2 - m;

    Target->WinBuffer[y+3+height-1][x+1] = 0xCC - m;
    Target->WinBuffer[y+3+height-1][x+1+width-1] = 0xCE - m;

    const char *c = title;
    u8 tx = 0;
    while (*c) 
    {
        Target->WinAttribute[y+4][x+2 + tx] = PAL0;
        Target->WinBuffer[y+4][x+2 + tx++] = *c++;
    }
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
    if (max <= min) return;

    y += 3;

    u8 scrollArea = height-2;
    if (scrollArea < 1) return;

    UI_ClearRect(x, y-3, 1, height);

    Target->WinBuffer[y][x+1] = 0xBD;           // Up arrow
    Target->WinBuffer[y+height-1][x+1] = 0xBE;  // Down arrow

    if (pos < min) pos = min;
    if (pos > max) pos = max;

    u16 range = max - min;

    // Max slider height
    u8 sliderHeight = scrollArea * scrollArea / (range + scrollArea);
    if (sliderHeight < 1) sliderHeight = 1;
    if (sliderHeight > scrollArea) sliderHeight = scrollArea;

    // Slider position
    u8 travelArea = scrollArea - sliderHeight;
    u8 sliderPos = (u32)(pos - min) * travelArea / range;

    for (u8 i = 0; i < sliderHeight; i++)
    {
        Target->WinBuffer[y + 1 + sliderPos + i][x + 1] = 0xBF;
    }
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
void UI_DrawItemList(u8 x, u8 y, u8 width, u8 height, char *list[], u16 item_count, u16 scroll)
{
    if (Target == NULL) return;

    u8 max_visible = height;
    u16 max = (item_count <= max_visible) ? 0 : (item_count-max_visible);
    u16 scroll_ = (scroll < max) ? scroll : max;

    if (max != 0) UI_DrawVScrollbar(x+width-1, y, height, 0, max, scroll_);
    UI_ClearRect(x, y, width-1, height);

    char tmp[width-1];

    for (u16 i = 0; i < (item_count < max_visible ? item_count : max_visible); i++)
    {
        strncpy(tmp, list[i+scroll_], width-1);
        UI_DrawText(x, y+i, PAL1, tmp);
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
    
    UI_DrawPanel(x, y, width, 3);
    UI_DrawText(x+1, y, PAL1, caption);

    size_t len = strlen(str);
    u8 nwidth = width-3;

    // Point to the start of the last `width` characters, or start of str if shorter
    const char *visibleStr = str;
    if (len > nwidth) 
    {
        visibleStr = str + (len - nwidth);
    }

    UI_DrawText(x+1, y+1, PAL1, visibleStr);

    if (bShowCaret) 
    {
        size_t caretPos = len < nwidth ? len : nwidth;
        Target->WinBuffer[y+4][x+2+caretPos] = 0x7C;
    }
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
/// @param flags ItemList flags (IL_None, IL_NoBorder)
void UI_DrawItemListSelect(u8 x, u8 y, u8 width, u8 height, const char *caption, char *list[], u8 item_count, u8 selected_item, UI_ItemListFlags flags)
{
    if (Target == NULL) return;

    u8 min_height = 0;
    u8 max_visible = 0;
    u8 x_ = x;
    u8 y_ = y+2;
    u8 w_ = width;
    char tmp[40];

    if (flags & IL_NoBorder)
    {
        min_height = (item_count < height) ? item_count : height;
        max_visible = height;
        w_ -= 1;
    }
    else// TEST ME
    {
        min_height = (item_count < height+2) ? item_count : height; 
        max_visible = height-2;

        x_ += 1;
        w_ -= 3;

        UI_DrawPanel(x_, y_, w_+3, min_height);
        UI_DrawText(x_+1, y_, PAL1, caption);
    }

    s16 pos = (s8)selected_item;
    pos = pos < 0 ? 0 : pos;

    if (item_count > max_visible)
    {
        UI_DrawVScrollbar(x_+w_, y_, height, 0, item_count-1, pos);
    }

    pos -= (max_visible-1);
    pos = pos < 0 ? 0 : pos;

    for (u16 i = 0; i < (item_count < max_visible ? item_count : max_visible); i++)
    {
        u8 n = i+pos;
        snprintf(tmp, width+1, "%-*s", width-1, list[n]);
        
        if (selected_item == n) UI_DrawText(x_, y_+i, PAL0, tmp);
        else UI_DrawText(x_, y_+i, PAL1, tmp);
    }
}

/// @brief Draw a colour picker
/// @param x X position
/// @param y Y position
/// @param rgb Pointer to an 12 bit RGB value (RRR0 GGG0 BBB0)
/// @param selected Selected channel (R / G / B)
void UI_DrawColourPicker(u8 x, u8 y, u16 *rgb, u8 selected)
{
    if (Target == NULL) return;
    
    u8 c;
    u8 r = (*rgb & 0xE)   >> 1;
    u8 g = (*rgb & 0xE0)  >> 5;
    u8 b = (*rgb & 0xE00) >> 9;

    UI_DrawText(x, y  , (selected == 0 ? PAL0 : PAL1), "Red   [       ]");
    UI_DrawText(x, y+1, (selected == 1 ? PAL0 : PAL1), "Green [       ]");
    UI_DrawText(x, y+2, (selected == 2 ? PAL0 : PAL1), "Blue  [       ]");

    c = 7; while (c--) Target->WinBuffer[y+3][x+8+c] = (c >= r ? ' ' : 0xAA);
    c = 7; while (c--) Target->WinBuffer[y+4][x+8+c] = (c >= g ? ' ' : 0xAA);
    c = 7; while (c--) Target->WinBuffer[y+5][x+8+c] = (c >= b ? ' ' : 0xAA);

    Target->WinBuffer[y+3][x+17] = 0xB0;    // Top Left
    Target->WinBuffer[y+3][x+19] = 0xB1;    // Top Right
    Target->WinBuffer[y+5][x+17] = 0xB2;    // Bottom Left
    Target->WinBuffer[y+5][x+19] = 0xB3;    // Bottom Right
    
    Target->WinBuffer[y+3][x+18] = 0xAE;    // Middle Top
    Target->WinBuffer[y+5][x+18] = 0xAF;    // Middle Bottom
    Target->WinBuffer[y+4][x+17] = 0xAC;    // Middle Left
    Target->WinBuffer[y+4][x+19] = 0xAD;    // Middle Right

    Target->WinBuffer[y+4][x+18] = 0xAB;    // Center

    SetColor(23, *rgb);
}

/// @brief Draw a confirmation selector box
/// @param x X position
/// @param y Y position
/// @param model Type of confirm box; UC_MODEL_OK_CANCEL, UC_MODEL_YES_NO, UC_MODEL_APPLY_CANCEL
/// @param selected Selected choice
void UI_DrawConfirmBox(u8 x, u8 y, UI_ConfirmModel model, u8 selected)
{
    if (Target == NULL) return;
    
    switch (model)
    {
        case CM_Ok_Cancel:
            UI_DrawText(x    , y, (selected == 0 ? PAL0 : PAL1), " Ok ");
            UI_DrawText(x + 6, y, (selected == 1 ? PAL0 : PAL1), " Cancel ");
        break;
        case CM_Yes_No:
            UI_DrawText(x    , y, (selected == 0 ? PAL0 : PAL1), " Yes ");
            UI_DrawText(x + 6, y, (selected == 1 ? PAL0 : PAL1), " No ");
        break;
        case CM_Apply_Cancel:
            UI_DrawText(x    , y, (selected == 0 ? PAL0 : PAL1), " Apply ");
            UI_DrawText(x + 8, y, (selected == 1 ? PAL0 : PAL1), " Cancel ");
        break;
        case CM_Add_Remove:
            UI_DrawText(x    , y, (selected == 0 ? PAL0 : PAL1), " Add ");
            UI_DrawText(x + 8, y, (selected == 1 ? PAL0 : PAL1), " Remove ");
        break;
    
        default:
        break;
    }
}

/// @brief Draw tab widget
/// @param x X position
/// @param y Y position
/// @param num_tabs Number of tabs
/// @param active_tab Active tab
/// @param selected Selected tab
void UI_DrawTabs(u8 x, u8 y, u8 w, u8 num_tabs, u8 active_tab, u8 selected, const char * const tab_text[])
{
    if (Target == NULL) return;

    u8 c;       // Down counter
    u8 len;     // Current tab title strlen
    u8 o = 0;   // Offset

    c = 1; while (c++ <= 38) Target->WinBuffer[y+4][x-1+c] = 0xA3;    // Bottom line spanning the entire width (will be cleared underneath the active tab)

    for (u8 i = 0; i < num_tabs; i++)
    {
        len = strlen(tab_text[i]);

        Target->WinBuffer[y+3][x+o] = ((x+o) == 0 ? 0xA4 : 0xA7);    // Left tab side

        c = len; while (c--) Target->WinBuffer[y+2][x+1+c+o] = (y == 0) ? 0xA5 : 0xA8;    // Top tab side

        Target->WinBuffer[y+3][x+len+1+o] = ((x+len+1+o) == 39 ? 0xA6 : 0xA9);    // Right tab side

        //c = 1; while (c++ <= 38) Target->WinBuffer[y+4][x-1+c] = 0xA8-5;    // Bottom

        if (active_tab != i) 
        {
            //c = len; while (c--) Target->WinBuffer[y+4][x+1+c+o] = 0xA3;    // Inactive tab bottom border
        
            UI_DrawText(x+o, y, PAL1, tab_text[i]);
        }
        else 
        {
            c = len; while (c--) Target->WinBuffer[y+4][x+1+c+o] = 0x20;    // Active tab bottom border (None, clear the line under it)
            
            UI_DrawText(x+o, y, (selected ? PAL1 : PAL0), tab_text[i]);
        }

        o += len+2;
    }
    
    //c = o; while (c++ <= 38) Target->WinBuffer[y+4][x-1+c] = 0xA8-5;    // Bottom

    if (x ==  0) Target->WinBuffer[y+4][0]    = 0xA2;   // Point where the left window border meets the tab bar
    if (w >= 38) Target->WinBuffer[y+4][x+39] = 0xA1;   // Point where the right window border meets the tab bar
}

/// @brief Draw a horizontal progress bar
/// @param x X position
/// @param y Y position
/// @param max Max value of progress bar
/// @param value Current value of progress bar
void UI_DrawHProgressBar(u8 x, u8 y, u8 max, u8 value)
{
    if (Target == NULL) return;

    u8 max_v = max == 0 ? 1 : max;
    
    u8  c = 15;
    f32 v = fix32Div(FIX32(value), FIX32(max_v));
        v = fix32Mul(v, FIX32(16));
    u8  r = fix32ToInt(v);

    UI_DrawText(x, y, PAL1, "[               ]");

    while (c--) Target->WinBuffer[y+3][x+2+c] = (c >= r ? ' ' : 0xAA);
}
