#include "Screensaver.h"
#include "Utils.h"
#include "StateCtrl.h"

bool sv_bScreensaver = TRUE;    // Screensaver enable
s16 InactiveCounter = 0;
s16 InactiveUpper = 18000;      // NTSC = 18000 - PAL = 15000
static s16 ScrCx = 0;
static s16 ScrCy = 0;
static s16 ScrDx = 1;
static s16 ScrDy = 1;


void ScreensaverInit()
{
    State cs = getState();
    InactiveCounter = 0;
    if (bPALSystem) InactiveUpper = 15000;   // NTSC = 18000 - PAL = 15000

    // Set up screensaver sprite here
    SetSprite_Y(SPRITE_ID_SCRSAV, 0);
    SetSprite_X(SPRITE_ID_SCRSAV, 0);
    SetSprite_TILE(SPRITE_ID_SCRSAV, (0xE000 | AVR_SCRSAV));

    // Point screensaver sprite link back to 0 or next sprite depending on which state smdt is in
    if (cs == PS_Gopher) {SetSprite_SIZELINK(SPRITE_ID_SCRSAV, SPR_HEIGHT_1x4 | SPR_WIDTH_4x1, SPRITE_ID_POINTER);} // Gopher.   ScrSaveSprite -> Mouse pointer
    else if (cs != PS_IRC) {SetSprite_SIZELINK(SPRITE_ID_SCRSAV, SPR_HEIGHT_1x4 | SPR_WIDTH_4x1, SPRITE_ID_CURSOR);}// Terminal. ScrSaveSprite -> Cursor
    else {SetSprite_SIZELINK(SPRITE_ID_SCRSAV, SPR_HEIGHT_1x4 | SPR_WIDTH_4x1, 2);}                                 // IRC. ScrSaveSprite -> First text input box

    // Update cursor sprite link to point to screensaver
    SetSprite_SIZELINK(SPRITE_ID_CURSOR, SPR_SIZE_1x1, SPRITE_ID_SCRSAV);
}

void ScreensaverTick()
{
    if (sv_bScreensaver == FALSE) return;

    if (InactiveCounter >= InactiveUpper)   // Screensaver is active, tick the sprite movement
    {        
        if ((ScrCy < 0) || (ScrCy > (bPALSystem?208:192)))
        {
            ScrDy *= -1;
            *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_CRAM_ADDR((u32)110);
            *((vu16*) VDP_DATA_PORT) = random();
        }

        if ((ScrCx < 0) || (ScrCx > 288))
        {
            ScrDx *= -1;
            *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_CRAM_ADDR((u32)110);
            *((vu16*) VDP_DATA_PORT) = random();
        }

        ScrCy += ScrDy;
        ScrCx += ScrDx;

        SetSprite_Y(SPRITE_ID_SCRSAV, ScrCy+128);
        SetSprite_X(SPRITE_ID_SCRSAV, ScrCx+128);
    }
    else if (InactiveCounter <= -1) // Turn off screensaver
    {
        SetSprite_Y(SPRITE_ID_SCRSAV, 0);
        SetSprite_X(SPRITE_ID_SCRSAV, 0);

        InactiveCounter = 0;

        //kprintf("Screensaver not active");
    }
    else // Screensaver is inactive, tick the inactive counter instead
    {
        InactiveCounter++;

        //if (InactiveCounter == InactiveUpper) kprintf("Screensaver active");
    }
}
