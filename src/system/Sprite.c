#include "Sprite.h"
#include "Utils.h"

u8 LastSprite = SPRITE_POINTER;
u8 LastSpriteSize = SPR_SIZE_1x1;

// Extern
void ScrSetupSprite();
void Mouse_SetupSprite();


void SP_Setup()
{
    ScrSetupSprite();   // Screensaver sprite setup
    Mouse_SetupSprite();// Mouse cursor sprite setup

    // Setup permanent sprite links
    SetSprite_SIZELINK(SPRITE_CURSOR, SPR_SIZE_1x1, SPRITE_SCRSAV);
    SetSprite_SIZELINK(SPRITE_SCRSAV, SPR_WIDTH_4x1 | SPR_HEIGHT_1x4, SPRITE_POINTER);
    SetSprite_SIZELINK(SPRITE_POINTER, SPR_SIZE_1x1, 0);

    // Save the last sprite and its size. Used to later to determine which sprite is at the end of the list (variable).
    LastSprite = SPRITE_POINTER;
    LastSpriteSize = SPR_SIZE_1x1;
}
