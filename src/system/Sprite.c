#include "Sprite.h"
#include "Utils.h"

// Extern
void ScrSetupSprite();
void Mouse_SetupSprite();


void SP_Setup()
{
    ScrSetupSprite();
    Mouse_SetupSprite();

    // Setup permanent sprite links
    SetSprite_SIZELINK(SPRITE_CURSOR, SPR_SIZE_1x1, SPRITE_SCRSAV);
    SetSprite_SIZELINK(SPRITE_SCRSAV, SPR_WIDTH_4x1 | SPR_HEIGHT_1x4, SPRITE_POINTER);
    SetSprite_SIZELINK(SPRITE_POINTER, SPR_SIZE_1x1, 0);
}
