#include "Screensaver.h"
#include "Utils.h"
#include "StateCtrl.h"

bool bScreensaver = FALSE;
s16 InactiveCounter = 0;
s16 InactiveUpper = 18000;  // NTSC = 18000 - PAL = 15000
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
    SetSprite_TILE(SPRITE_ID_SCRSAV, (0x8000 | AVR_SCRSAV));
    SetSprite_SIZELINK(SPRITE_ID_SCRSAV, SPR_HEIGHT_1x4 | SPR_WIDTH_4x1, 0);

    // Update cursor sprite link to point to screensaver
    // IRC client sets this link itself, from last IRC sprite to screensaver sprite
    if (cs != PS_IRC) SetSprite_SIZELINK(SPRITE_ID_CURSOR, SPR_SIZE_1x1, SPRITE_ID_SCRSAV);
}

void ScreensaverTick()
{
    if (bScreensaver == FALSE) return;

    if (InactiveCounter >= InactiveUpper)   // Screensaver is active, tick the sprite movement
    {        
        if ((ScrCy < 0) || (ScrCy > (bPALSystem?208:184)))
        {
            ScrDy *= -1;
            *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_CRAM_ADDR((u32)22);
            *((vu16*) VDP_DATA_PORT) = random();
        }

        if ((ScrCx < 0) || (ScrCx > 288))
        {
            ScrDx *= -1;
            *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_CRAM_ADDR((u32)22);
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
