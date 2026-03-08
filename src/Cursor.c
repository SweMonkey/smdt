#include "Cursor.h"
#include "system/Sprite.h"
#include "Utils.h"  // bPALSystem
#include "Terminal.h"

static u8 CursorBlink = 0;  // Cursor blink counter
u8 bDoCursorBlink = TRUE;
u16 sv_CursorCL = 0x0E0;    // Cursor colour
u16 LastCursor = 0x10;      // Last cursor tile used


void CR_SetupSprite(u16 x, u16 y)
{
    // Clamp position
    u16 sprx = x >= 504 ? 504 : x;
    u16 spry = y >= 504 ? 504 : y;

    SetSprite_Y(SPRITE_CURSOR, spry);
    SetSprite_X(SPRITE_CURSOR, sprx);
    SetSprite_TILE(SPRITE_CURSOR, LastCursor);
}

void CR_Blink()
{
    if (!bDoCursorBlink) return;

    // Cursor blink
    if (CursorBlink == (bPALSystem?16:20))
    {
        SetSprite_TILE(SPRITE_CURSOR, LastCursor);
        CursorBlink++;
    }
    else if (CursorBlink == (bPALSystem?32:40))
    {
        SetSprite_TILE(SPRITE_CURSOR, 0x16);
        CursorBlink = 0;
    }
    else CursorBlink++;
}
