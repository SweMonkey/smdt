#include "Cursor.h"
#include "Utils.h"  // bPALSystem
#include "Terminal.h"

static u8 CursorBlink = 0;  // Cursor blink counter
u8 bDoCursorBlink = TRUE;
u16 sv_CursorCL = 0x0E0;    // Cursor colour
u16 LastCursor = 0x10;      // Last cursor tile used


void CR_Blink()
{
    if (!bDoCursorBlink) return;

    // Cursor blink
    if (CursorBlink == (bPALSystem?16:20))
    {
        SetSprite_TILE(SPRITE_ID_CURSOR, LastCursor);
        CursorBlink++;
    }
    else if (CursorBlink == (bPALSystem?32:40))
    {
        SetSprite_TILE(SPRITE_ID_CURSOR, 0x16);
        CursorBlink = 0;
    }
    else CursorBlink++;
}
