#include "Cursor.h"
#include "Utils.h"  // bPALSystem
#include "Terminal.h"

static u8 CursorBlink = 0; // Cursor blink counter
u8 bDoCursorBlink = TRUE;
u16 Cursor_CL = 0x0E0;
u16 LastCursor = 0x10;      // Last cursor tile used


void CR_Blink()
{
    if (!bDoCursorBlink) return;

    // Cursor blink
    if (CursorBlink == (bPALSystem?12:15))
    {
        SetSprite_TILE(CURSOR_SPRITE_NUM, LastCursor);
        CursorBlink++;
    }
    else if (CursorBlink == (bPALSystem?24:30))
    {
        SetSprite_TILE(CURSOR_SPRITE_NUM, 0x16);
        CursorBlink = 0;
    }
    else CursorBlink++;
}
